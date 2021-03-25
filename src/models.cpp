#include <whiteboard_pal/main.hpp>

// frame: Matrix of the current frame in BGR24 format, that is, the mat entries are 3-bytes deep, each byte representing the B, G, R respectively
// idx: index of the frame, if that's useful for some reason
FingerOutput finger_tracking(Mat frame, int idx) {
    return FingerOutput {
        0,
        0,
        idx
    };
}

// frame: Matrix of the current frame in BGR24 format, that is, the mat entries are 3-bytes deep, each byte representing the B, G, R respectively
// idx: index of the frame, if that's useful for some reason
GestureOutput gesture_detection(Mat frame, int idx) {
    return GestureOutput {
        false,
        idx
    };
}