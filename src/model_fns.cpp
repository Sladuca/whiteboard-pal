#include <memory>

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