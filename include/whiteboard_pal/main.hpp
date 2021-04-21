#ifndef GLOBRL_HDRS
#define GLOBRL_HDRS

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc.hpp>
#include <boost/fiber/all.hpp>
#include "cppflow/ops.h"
#include "cppflow/model.h"
#include "cppflow/tensor.h"
#include "cppflow/cppflow.h"
#include <chrono>;
#endif

using namespace cv;
using namespace std;
using namespace std::literals;

//using namespace cv::traits;

typedef chrono::time_point<chrono::steady_clock> instant_t;

typedef struct perf_info {
    instant_t in_start;
    instant_t in_end;
    instant_t gesture_start;
    instant_t finger_start;
    instant_t gesture_end;
    instant_t finger_end;
    instant_t output_recv_finger;
    instant_t output_recv_gesture;
    instant_t output_start;
    instant_t output_end;
} perf_info_t;

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
    int i;
    bool gesture;
} gesture_output_t;

typedef struct frame_with_idx {
    Mat frame;
    perf_info_t *perf;
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
    perf_info_t *perf;
    finger_output_t finger;
} finger_output_with_frame_t;

typedef enum substrate_source {
    WEBCAM,
    SCREEN,
    WHITEBOARD,
} substrate_source_t;


typedef boost::fibers::buffered_channel<frame_with_idx_t> frame_chan_t;
typedef boost::fibers::buffered_channel<gesture_output_t> gesture_chan_t;
typedef boost::fibers::buffered_channel<finger_output_with_frame_t> finger_chan_t;
typedef boost::fibers::buffered_channel<capture_size_t> cap_size_chan_t;
typedef boost::fibers::buffered_channel<perf_info_t *> perf_chan_t;

void print_point_vec(vector<Point> &vec);
gesture_output_t gesture_detection(cppflow::model model, Mat frame, int i);
finger_output_t finger_tracking(Mat frame, int i, deque<Point> &points);
int input(frame_chan_t &to_finger, frame_chan_t &to_gesture, cap_size_chan_t &broadcast_size);
int substrate(frame_chan_t &substrate_chan, cap_size_chan_t &broadcast_size, substrate_source_t source);
