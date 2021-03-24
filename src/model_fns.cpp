#include <memory>
#include<opencv2/opencv.hpp>
typedef struct FingerOutput {
    int x;
    int y;
    int idx;
} FingerOutput;

typedef struct GestureOutput {
    bool gesture;
    int idx;
} GestureOutput;

extern "C" {
    FingerOutput finger_tracking(void *frame, int idx);
    GestureOutput gesture_detection(void *frame, int idx);
}

FingerOutput finger_tracking(void *frame, int idx) {
    return FingerOutput {
        0,
        0,
        idx
    };
}


GestureOutput gesture_detection(void *frame, int idx) {
    return GestureOutput {
        false,
        idx
    };
}

size_t write_output(void *frame, int fd, size_t size) {
    cv::Mat *mat = static_cast<cv::Mat*>(frame);
    return (size_t) write(fd, frame->data, size);
}