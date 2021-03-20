use crossbeam::{
    channel,
    channel::{
        Receiver, 
        Sender, 
        select,
        TryRecvError
    },
    sync::WaitGroup
};

use opencv::core::Mat;
use opencv::videoio::{
    VideoCapture,
    VideoCaptureTrait
};
use std::thread;
use anyhow::{
    Error,
    anyhow
};
use core::ffi::c_void;
use cty::c_int;

#[repr(C)]
struct FingerOutput {
    x: c_int,
    y: c_int,
    idx: c_int,
}

#[repr(C)]
struct GestureOutput {
    gesture: bool,
    idx: c_int,
}

extern "C" {
    fn finger_tracking(frame:  *const c_void, idx: c_int) -> FingerOutput;
    fn gesture_detection(frame: *const c_void, idx: c_int) -> GestureOutput;
}

const FRAME_BUF_SIZE: usize = 30;

fn main() {
    let (gesture_frame_tx, gesture_frame_rx) = channel::bounded::<(Mat, i32)>(FRAME_BUF_SIZE);
    let (finger_frame_tx, finger_frame_rx) = channel::bounded::<(Mat, i32)>(FRAME_BUF_SIZE);
    let (gesture_tx, gesture_rx) = channel::bounded::<(bool, i32)>(1);
    let (finger_tx, finger_rx) = channel::bounded::<(Mat, (i32, i32), i32)>(FRAME_BUF_SIZE);
    let (result_tx, result_rx) = channel::bounded::<(Mat, (i32, i32), bool, i32)>(FRAME_BUF_SIZE);

    let wg = WaitGroup::new();

    let wg0 = wg.clone();
    thread::spawn(move || {
        let res = input(gesture_frame_tx, finger_frame_tx);
        if let Err(e) = res {
            eprintln!("in input thread: {}", e);
        };
        drop(wg0)
    });

    let wg1 = wg.clone();
    thread::spawn(move || {
        let res = gesture(gesture_frame_rx, gesture_tx);
        if let Err(e) = res {
            eprintln!("in gesture thread: {}", e);
        };
        drop(wg1)
    });

    let wg2 = wg.clone();
    thread::spawn(move || {
        let res = finger(finger_frame_rx, finger_tx);
        if let Err(e) = res {
            eprintln!("in finger thread: {}", e);
        };
        drop(wg2)
    });

    let wg3 = wg.clone();
    thread::spawn(move || {
        let res = sync(finger_rx, gesture_rx, result_tx);
        if let Err(e) = res {
            eprintln!("in sync thread: {}", e);
        };
        drop(wg3)
    });

    let res = canvas(result_rx);
    if let Err(e) = res {
        eprintln!("in canvas thread: {}", e);
    };
    
    wg.wait();
}

fn input(gesture_frame_tx: Sender<(Mat, i32)>, finger_frame_tx: Sender<(Mat, i32)>) -> Result<(), Error> {
    let mut cap = VideoCapture::default()?;
    let mut cnt = 0;
    loop {
        let mut frame = Mat::default()?;
        cap.read(&mut frame)?;
        // TODO: see if there's some way to avoid clone
        gesture_frame_tx.send((Mat::copy(&frame)?, cnt)).map_err(|_e| anyhow!("failed to send frame {} from input to gesture thread", cnt))?;
        finger_frame_tx.send((frame, cnt)).map_err(|_e| anyhow!("failed to send frame {} from input to finger thread", cnt))?;
        cnt += 1;
    }
}

fn gesture(frame_in_chan: Receiver<(Mat, i32)>, out_chan: Sender<(bool, i32)>) -> Result<(), Error> {
    loop {
        select! {
            recv(frame_in_chan) -> msg => match msg {
                Ok((frame, i)) => {
                    
                    // ! Safety: this is safe because model_fns guarantees to not free the matrix
                    let (gesture, i) = unsafe {   
                        let frame_raw = frame.as_raw_Mat();
                        let output = gesture_detection(frame_raw, i);
                        (output.gesture, output.idx)
                    };

                    out_chan.send((gesture, i)).map_err(|e| anyhow!("gesture thread failed to send result to sync: {}", e))?;

                }
                Err(e) => break Err(anyhow!("gesture thread failed to recv frame from input: {}", e))
            }
        }
    }
}

fn finger(frame_in_chan: Receiver<(Mat, i32)>, out_chan: Sender<(Mat, (i32, i32), i32)>) -> Result<(), Error> {
    loop {
        select! {
            recv(frame_in_chan) -> msg => match msg {
                Ok((frame, i)) => {
                    // ! Safety: this is safe because model_fns guarantees to not free the matrix
                    let (coords, i) = unsafe {
                        let frame_raw = frame.as_raw_Mat();
                        let output = finger_tracking(frame_raw, i);
                        ((output.x, output.y), output.idx)
                    };
                   
                    out_chan.send((frame, coords, i)).map_err(|e| anyhow!("finger thread failed to send result to sync: {}", e))?;
                }
                Err(e) => break Err(anyhow!("finger thread failed to recv frame from input: {}", e))
            }
        }
    }
}

fn sync(finger_chan: Receiver<(Mat, (i32, i32), i32)>, gesture_chan: Receiver<(bool, i32)>, result_chan: Sender<(Mat, (i32, i32), bool, i32)>) -> Result<(), Error> {
    let mut finger_buf = Vec::<(Mat, (i32, i32), i32)>::new();
    let mut gesture_buf = Vec::<(bool, i32)>::new();
    loop {
        select! {
            recv(finger_chan) -> msg => match msg {
                Ok((frame, coords, i)) => {
                    if gesture_buf.len() > 0 && gesture_buf[0].1 == i {
                        result_chan.send((frame, coords, gesture_buf.remove(0).0, i)).map_err(|_e| anyhow!("failed to send finger result {} from sync to canvas", i))?;
                    } else {
                        finger_buf.push((frame, coords, i));
                    }
                }
                Err(e) => break Err(anyhow!("sync thread failed to recv finger output: {}", e))
            },
            recv(gesture_chan) -> msg => match msg {
                Ok((gesture_state, i)) => {
                    if finger_buf.len() > 0 && finger_buf[0].2 == i {
                        let (frame, coords, _) = finger_buf.remove(0);
                        result_chan.send((frame, coords, gesture_state, i)).map_err(|_e| anyhow!("failed to send gesture result {} from sync to canvas", i))?;
                    } else {
                        gesture_buf.push((gesture_state, i));
                    }
                }
                Err(e) => break Err(anyhow!("sync thread failed to recv gesture output: {}", e))
            }
        }
    }
}

fn canvas(result_chan: Receiver<(Mat, (i32, i32), bool, i32)>) -> Result<(), Error> {
    loop {
        select! {
            recv(result_chan) -> msg => match msg {
                Ok((frame, coords, gesture_state, i)) => {
                    println!("i: {}, x: {}, y: {}, gesture_state: {}", gesture_state, coords.0, coords.1, gesture_state);
                    // TODO: draw on canvas
                    // TODO: output to v4l2
                },
                Err(e) => break Err(anyhow!("canvas thread failed to frame and model output: {}", e))
            }
        }
    }
}
