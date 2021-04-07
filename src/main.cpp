#include <whiteboard_pal/main.hpp>
#include <iostream>
#include <stdio.h>
#include <sys/ioctl.h>
#include <boost/fiber/all.hpp>
#include <linux/videodev2.h>
#include "cppflow/cppflow.h"
#include "cppflow/ops.h"
#include "cppflow/model.h"

#define FRAME_BUF_SIZE 30
#define VID_WIDTH  640
#define VID_HEIGHT 480
#define DOT_WIDTH 2
#define VIDEO_OUT "/dev/video6"

typedef struct frame_with_idx {
    Mat frame;
    int i;
} frame_with_idx_t;

typedef struct loopback_info {
    size_t width;
    size_t height;
    size_t framesize;
    int fd;
} loopback_info_t;

typedef struct finger_output_with_frame {
    Mat frame;
    finger_output_t finger;
} finger_output_with_frame_t;

typedef boost::fibers::buffered_channel<frame_with_idx_t> frame_chan_t;
typedef boost::fibers::buffered_channel<gesture_output_t> gesture_chan_t;
typedef boost::fibers::buffered_channel<finger_output_with_frame_t> finger_chan_t;

int input(frame_chan_t &to_finger, frame_chan_t &to_gesture, VideoCapture cap) {

    Mat frame;
    while (true) {
        cap.read(frame);
        if (frame.empty()) {
            cerr << "ERROR: failed to read frame!";
            return -1;
        }
        int i = cap.get(CAP_PROP_POS_FRAMES);
        Mat frame2 = frame;
        to_finger.push(frame_with_idx_t { frame, i });
        to_gesture.push(frame_with_idx_t { frame2, i});
    }
    to_finger.close();
    // to_gesture.close();
    return 0;
}

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

int output(gesture_chan_t &from_gesture, finger_chan_t &from_finger, size_t width, size_t height) {

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
            return -1;
        }

        gesture_output_t gesture;
        if (boost::fibers::channel_op_status::success != from_gesture.pop(gesture)) {
            cerr << "ERROR: failed to recv result from gesture detection!\n";
            close(lb.fd);
            return -1;
        }

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
        finger.frame.setTo(Scalar(0, 0, 255), canvas);

        // convert back to yuv420
        cvtColor(finger.frame, finger.frame, COLOR_BGR2YUV_I420);
        size_t bytes_written = write(lb.fd, finger.frame.data, lb.framesize);
        if (bytes_written < 0) {
            cerr << "ERROR: failed to write frame to loopback device!";
            close(lb.fd);
            return -2;
        }
    }

    close(lb.fd);
    return 0;
}

int gesture(frame_chan_t &to_gesture, gesture_chan_t &from_gesture) {
    frame_with_idx_t frame;
    //cppflow::model model("../model_10_30");
    while (boost::fibers::channel_op_status::success == to_gesture.pop(frame)) {
        gesture_output_t g = gesture_detection(frame.frame, frame.i);
        from_gesture.push(g);
    }
    from_gesture.close();
    return 0;
}

int finger(frame_chan_t &to_finger, finger_chan_t &from_finger) {
    frame_with_idx_t frame;
    while (boost::fibers::channel_op_status::success == to_finger.pop(frame)) {
        finger_output_t f = finger_tracking(frame.frame, frame.i);
        from_finger.push(finger_output_with_frame_t { frame.frame, f });
    }
    from_finger.close();
    return 0;
}

void print_fourcc(int fourcc) {
    string fourcc_str = format("%c%c%c%c", fourcc & 255, (fourcc >> 8) & 255, (fourcc >> 16) & 255, (fourcc >> 24) & 255);
    cout << "CAP_PROP_FOURCC: " << fourcc_str << endl;
}

void print_mat_type(int type) {
  string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  cout << "CAP_PROP_FORMAT: " << r << "\n";
}



int main() {
    frame_chan_t to_finger { 2 };
    frame_chan_t to_gesture { 2 };
    gesture_chan_t from_gesture { 2 };
    finger_chan_t from_finger { 2 };

    VideoCapture cap(0, CAP_V4L);

    if (not cap.isOpened()) {
        cerr << "ERROR: failed to open camera!\n";
        return -2;
    }

    int fourcc = cap.get(CAP_PROP_FOURCC);
    int format = cap.get(CAP_PROP_FORMAT);
    print_fourcc(fourcc);
    print_mat_type(format);

    size_t width = cap.get(CAP_PROP_FRAME_WIDTH);
    size_t height = cap.get(CAP_PROP_FRAME_HEIGHT);

    boost::fibers::fiber input_fiber(bind(input, ref(to_finger), ref(to_gesture), ref(cap)));
    boost::fibers::fiber finger_fiber(bind(finger, ref(to_finger), ref(from_finger)));
    boost::fibers::fiber gesture_fiber(bind(gesture, ref(to_gesture), ref(from_gesture)));
    boost::fibers::fiber output_fiber(bind(output, ref(from_gesture), ref(from_finger), width, height));

    output_fiber.join();
    gesture_fiber.join();
    finger_fiber.join();
    input_fiber.join();
}
