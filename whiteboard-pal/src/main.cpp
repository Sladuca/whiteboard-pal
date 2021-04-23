#include <whiteboard_pal/main.hpp>
#include <iostream>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#define PERF_WINDOW_SIZE 30
#define VID_WIDTH  640
#define VID_HEIGHT 480
#define DOT_WIDTH 20
#define VIDEO_OUT "/dev/video6"

int configure_loopback(loopback_info_t *lb, size_t width, size_t height) {
    int fd = open(VIDEO_OUT, O_RDWR);
    if(fd < 0) {
        cerr << "ERROR: failed to open output device!\n" << strerror(errno);
        return -2;
    }

    // CONFIGURE VIDEO FORMAT

    struct v4l2_format vid_format;
    memset(&vid_format, 0, sizeof(vid_format));
    vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    if (ioctl(fd, VIDIOC_G_FMT, &vid_format) < 0) {
        cerr << "ERROR: failed to get default loopback video format!\n" << strerror(errno);
        return -1;
    }

    cout << "output dim: (" << width << ", " << height << ")\n";
    cout << "output colorspace: " << vid_format.fmt.pix.colorspace << "\n";

    // YUYV encodes every 2 pixels with a 4 byte "YUYV", where each pixel has its own Y (luma)
    // but the two pixels share the same U and V (chroma) values.
    size_t num_pixels = width * height;
    // size_t framesize = ((num_pixels + 1) / 2) * 4;
    size_t framesize = num_pixels * 1.5;
    // size_t framesize = num_pixels * 3;
    vid_format.fmt.pix.width = width;
    vid_format.fmt.pix.height = height;
    vid_format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    vid_format.fmt.pix.sizeimage = framesize;
    vid_format.fmt.pix.field = V4L2_FIELD_NONE;
    vid_format.fmt.pix.xfer_func = V4L2_XFER_FUNC_709;

    if (ioctl(fd, VIDIOC_S_FMT, &vid_format) < 0) {
        cerr << "ERROR: failed to set loopback video format!\n" << strerror(errno);
        return -1;
    }

    *lb = loopback_info_t {
        width,
        height,
        framesize,
        fd
    };
    return 0;
}

int output(
    gesture_chan_t &from_gesture,
    finger_chan_t &from_finger,
    cap_size_chan_t &broadcast_size,
    frame_chan_t &substrate_chan,
    bool use_substrate,
    perf_chan_t &perf_chan
    ) {
    capture_size_t cap_size;
    if (boost::fibers::channel_op_status::success != broadcast_size.pop(cap_size)) {
        cerr << "ERROR: failed to get capture size from input!\n";
        from_gesture.close();
        from_finger.close();
        return -1;
    }
    int width = cap_size.width;
    int height = cap_size.height;

    loopback_info_t lb;
    int res = configure_loopback(&lb, width, height);
    if (res < 0) {
        return res;
    }

    Mat canvas = Mat::zeros(lb.height, lb.width, CV_8UC1);

    while (true) {

        finger_output_with_frame_t finger;
        if (boost::fibers::channel_op_status::success != from_finger.pop(finger)) {
            cerr << "ERROR: failed to recv result from finger tracking!\n";
            close(lb.fd);
            from_gesture.close();
            from_finger.close();
            return -1;
        }
        finger.perf->output_recv_finger = chrono::steady_clock::now();

        gesture_output_t gesture;
        if (boost::fibers::channel_op_status::success != from_gesture.pop(gesture)) {
            cerr << "ERROR: failed to recv result from gesture detection!\n";
            close(lb.fd);
            from_gesture.close();
            from_finger.close();
            return -1;
        }
        finger.perf->output_recv_gesture = chrono::steady_clock::now();

        frame_with_idx_t sub;
        if (!use_substrate) {
            sub.frame = finger.frame;
            sub.i = finger.finger.i;
        } else if (boost::fibers::channel_op_status::success != substrate_chan.pop(sub)) {
            cerr << "ERROR: failed to recv substrate frame from substrate!\n";
            close(lb.fd);
            from_gesture.close();
            from_finger.close();
            return -1;
        }
        finger.perf->output_start = chrono::steady_clock::now();

        // draw onto canvas
        if (gesture.gesture) {

            if (finger.finger.x < DOT_WIDTH / 2) {
                finger.finger.x = DOT_WIDTH / 2;
            }
            if (finger.finger.y < DOT_WIDTH / 2) {
                finger.finger.y = DOT_WIDTH / 2;
            }
            if (lb.width - finger.finger.x < DOT_WIDTH / 2) {
                finger.finger.x = lb.width - DOT_WIDTH / 2;
            }
            if (lb.height - finger.finger.y < DOT_WIDTH / 2) {
                finger.finger.y = lb.height - DOT_WIDTH / 2;
            }

            // cout << "x: " << finger.finger.x << " y: " << finger.finger.y << "\n";

            Mat roi = canvas(Rect(finger.finger.x-(DOT_WIDTH / 2), finger.finger.y-(DOT_WIDTH/2), DOT_WIDTH, DOT_WIDTH));
            roi.setTo(1);
        }

        // apply canvas to frame
        sub.frame.setTo(Scalar(0, 0, 255), canvas);

        // convert back to yuv420
        cvtColor(sub.frame, sub.frame, COLOR_BGR2YUV_I420);
        size_t bytes_written = write(lb.fd, sub.frame.data, lb.framesize);
        finger.perf->output_end = chrono::steady_clock::now();

        if (bytes_written < 0) {
            cerr << "ERROR: failed to write frame to loopback device!";
            close(lb.fd);
            from_gesture.close();
            from_finger.close();
            return -2;
        }

        perf_chan.push(finger.perf);
    }

    close(lb.fd);
    from_gesture.close();
    from_finger.close();
    return 0;
}

int gesture(frame_chan_t &to_gesture, gesture_chan_t &from_gesture) {
    frame_with_idx_t frame;
    cppflow::model model("./src/model");
    while (boost::fibers::channel_op_status::success == to_gesture.pop(frame)) {
        frame.perf->gesture_start = chrono::steady_clock::now();
        gesture_output_t g = gesture_detection(model, frame.frame, frame.i);
        frame.perf->gesture_end = chrono::steady_clock::now();
        from_gesture.push(g);
    }
    from_gesture.close();
    to_gesture.close();
    return 0;
}

int finger(frame_chan_t &to_finger, finger_chan_t &from_finger) {
    frame_with_idx_t frame;
    while (boost::fibers::channel_op_status::success == to_finger.pop(frame)) {
        frame.perf->finger_start = chrono::steady_clock::now();
        finger_output_t f = finger_tracking(frame.frame, frame.i);
        frame.perf->finger_end = chrono::steady_clock::now();
        from_finger.push(finger_output_with_frame_t { frame.frame, frame.perf, f });
    }
    from_finger.close();
    return 0;
}

int record_metrics(perf_chan_t &perf_chan) {
    ofstream f;
    f.open("perf.csv", ios::out);
    f << "idx: frame index\n";
    f << "fps: current throughput over the last PERF_WINDO_SIZE frames in frames per second\n";
    f << "t_input: time to read a frame from webcam\n";
    f << "t_gesture: time gesture detector takes\n";
    f << "t_finger: time finger tracker takes\n";
    f << "t_sync_gesture: time from when gesture detector is done to when output thread recv's the result\n";
    f << "t_sync_finger: time from when finger tracker is done to when output thread recv's the result\n";
    f << "t_output: time from when output thread recv's both results to when output frame is written to loopback device\n";
    f << "t_total: total time to get a single frame through the system\n\n";
    f << "idx, fps, t_input, t_gesture, t_finger, t_sync_gesture, t_sync_finger, t_output, t_total\n";

    instant_t window[PERF_WINDOW_SIZE];
    perf_info_t *perf;
    int start = 0;
    int i = 0;
    while (boost::fibers::channel_op_status::success == perf_chan.pop(perf)) {
        if (perf == NULL) {
            f.close();
            perf_chan.close();
            return -1;
        }
        
        int end = (start > 0) ? start - 1 : PERF_WINDOW_SIZE - 1;
        window[start] = chrono::steady_clock::now();
        start = (start + 1) % PERF_WINDOW_SIZE;

        auto window_duration = chrono::duration_cast<chrono::milliseconds>(window[end] - window[start]).count();
        cout << "average latency: " << window_duration / 30 << "ms \n";
        
        f << ++i << ", "
          << (window_duration == 0 ? -1 : (PERF_WINDOW_SIZE * 1000 / window_duration)) << ", "
          << (perf->in_end - perf->in_start) / 1ms << ", "
          << (perf->gesture_end - perf->gesture_start) / 1ms << ", "
          << (perf->finger_end - perf->finger_start) / 1ms << ", "
          << (perf->output_recv_gesture - perf->gesture_end) / 1ms << ", "
          << (perf->output_recv_finger - perf->finger_end) / 1ms << ", "
          << (perf->output_end - perf->output_start) / 1ms << ", "
          << (perf->output_end - perf->in_start) / 1ms << "\n";

        free(perf);
    }
    f.close();
}

int input_wrapped(frame_chan_t &to_finger, frame_chan_t &to_gesture, cap_size_chan_t &broadcast_size) {
    boost::fibers::fiber fiber(bind(input, ref(to_finger), ref(to_gesture), ref(broadcast_size)));
    fiber.join();
    return 0;
}

int finger_wrapped(frame_chan_t &to_finger, finger_chan_t &from_finger) {
    boost::fibers::fiber fiber(bind(finger, ref(to_finger), ref(from_finger)));
    fiber.join();
    return 0;
}

int gesture_wrapped(frame_chan_t &to_gesture, gesture_chan_t &from_gesture) {
    boost::fibers::fiber fiber(bind(gesture, ref(to_gesture), ref(from_gesture)));
    fiber.join();
    return 0;
}

int output_wrapped(gesture_chan_t &from_gesture, finger_chan_t &from_finger, cap_size_chan_t &broadcast_size, substrate_source_t source) {
    frame_chan_t substrate_chan { 2 };
    perf_chan_t perf_chan { 2 };

    boost::fibers::fiber perf_fiber(bind(record_metrics, ref(perf_chan)));

    if (source == WEBCAM) {
        boost::fibers::fiber fiber(bind(output, ref(from_gesture), ref(from_finger), ref(broadcast_size), ref(substrate_chan), false, ref(perf_chan)));
        perf_fiber.detach();
        fiber.join();
        return 0;
    }
    
    thread substrate_thread(substrate, ref(substrate_chan), ref(broadcast_size), source);
    boost::fibers::fiber fiber(bind(output, ref(from_gesture), ref(from_finger), ref(broadcast_size), ref(substrate_chan), true, ref(perf_chan)));
    
    perf_fiber.detach();
    fiber.join();
    substrate_chan.close();
    substrate_thread.join();
    return 0;
}


int run_whiteboard_pal() {
    frame_chan_t to_finger { 2 };
    frame_chan_t to_gesture { 2 };
    gesture_chan_t from_gesture { 2 };
    finger_chan_t from_finger { 2 };
    cap_size_chan_t broadcast_size { 2 };

    thread threads[] = {
        thread(input_wrapped, ref(to_finger), ref(to_gesture), ref(broadcast_size)),
        thread(finger_wrapped, ref(to_finger), ref(from_finger)),
        thread(gesture_wrapped, ref(to_gesture), ref(from_gesture)),
        thread(output_wrapped, ref(from_gesture), ref(from_finger), ref(broadcast_size), WEBCAM)
    };
    
    for (int i = 0; i < 5; i++) {
        threads[i].join();
    }
}
