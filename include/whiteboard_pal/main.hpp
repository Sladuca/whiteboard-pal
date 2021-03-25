#ifndef OPENCV_HDRS
#define OPENCV_HDRS

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>

#endif

using namespace cv;
using namespace std;

typedef struct FingerOutput {
    int x;
    int y;
    int idx;
} FingerOutput;

typedef struct GestureOutput {
    bool gesture;
    int idx;
} GestureOutput;