#include <whiteboard_pal/main.hpp>


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