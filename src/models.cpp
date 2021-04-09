#include <whiteboard_pal/main.hpp>
#include "cppflow/ops.h"
#include "cppflow/model.h"

// frame: Matrix of the current frame in BGR24 format, that is, the mat entries are 3-bytes deep, each byte representing the B, G, R respectively
// idx: index of the frame, if that's useful for some reason
finger_output_t finger_tracking(Mat frame, int idx) {
    return finger_output_t {
        200,
        200,
        idx
    };
}

// frame: Matrix of the current frame in BGR24 format, that is, the mat entries are 3-bytes deep, each byte representing the B, G, R respectively
// idx: index of the frame, if that's useful for some reason
gesture_output_t gesture_detection(cppflow::model model, Mat frame, int idx) {
    return gesture_output_t {
        true,
        idx
    };
}
