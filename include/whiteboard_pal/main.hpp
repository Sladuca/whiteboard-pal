#ifndef GLOBRL_HDRS
#define GLOBRL_HDRS

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <boost/fiber/all.hpp>
#include "cppflow/cppflow.h"
#include "cppflow/ops.h"
#include "cppflow/model.h"

#endif

using namespace cv;
using namespace std;

typedef struct capture_size {
    int width;
    int height;
} capture_size_t;

typedef struct finger_output {
    int x;
    int y;
    int i;
} finger_output_t;

typedef struct gesture_output {
    bool gesture;
    int i;
} gesture_output_t;

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

typedef enum capture_source {
    WEBCAM,
    SCREEN,
    WHITEBOARD
} capture_source_t;

typedef boost::fibers::buffered_channel<frame_with_idx_t> frame_chan_t;
typedef boost::fibers::buffered_channel<gesture_output_t> gesture_chan_t;
typedef boost::fibers::buffered_channel<finger_output_with_frame_t> finger_chan_t;
typedef boost::fibers::unbuffered_channel<capture_size_t> cap_size_chan_t;

gesture_output_t gesture_detection(cppflow::model model, Mat frame, int i);
finger_output_t finger_tracking(Mat frame, int i);
int input(frame_chan_t &to_finger, frame_chan_t &to_gesture, cap_size_chan_t &broadcast_size, capture_source_t source);