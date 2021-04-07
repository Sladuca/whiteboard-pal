#ifndef OPENCV_HDRS
#define OPENCV_HDRS

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>

#endif

using namespace cv;
using namespace std;

typedef struct finger_output {
    int x;
    int y;
    int i;
} finger_output_t;

typedef struct gesture_output {
    bool gesture;
    int i;
} gesture_output_t;

gesture_output_t gesture_detection(Mat frame, int i);
finger_output_t finger_tracking(Mat frame, int i);
