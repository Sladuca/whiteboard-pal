#ifndef GLOBRL_HDRS
#define GLOBRL_HDRS

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/mat.hpp>
#include <boost/fiber/all.hpp>
#include <chrono>
#endif

using namespace cv;
using namespace std;
using namespace std::literals;


typedef struct loopback_info {
    size_t width;
    size_t height;
    size_t framesize;
    int fd;
} loopback_info_t;

typedef enum substrate_source {
    WEBCAM,
    SCREEN_CAPTURE,
    WHITEBOARD,
} substrate_source_t;


typedef boost::fibers::buffered_channel<cv::Mat> frame_chan_t;
typedef boost::fibers::buffered_channel<int> int_char_chan_t;

std::pair<std::pair<int, int>, thread> input(frame_chan_t &camera_chan);
int output(frame_chan_t &output_frames_chan);