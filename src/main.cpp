#include "main.hpp"
#include <iostream>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "mediapipe/framework/port/file_helpers.h"
#include "mediapipe/framework/port/opencv_highgui_inc.h"
#include "mediapipe/framework/port/opencv_imgproc_inc.h"
#include "mediapipe/framework/port/opencv_video_inc.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/status.h"

#define PERF_WINDOW_SIZE 30
#define VID_WIDTH  640
#define VID_HEIGHT 480
#define DOT_WIDTH 20
#define VIDEO_OUT "/dev/video6"

constexpr char GRAPH_INPUT_STREAM_NAME[] = "input_video";
constexpr char GRAPH_OUTPUT_STREAM_NAME[] = "output_video";

int configure_loopback(loopback_info_t *lb, size_t width, size_t height) {
    int fd = open(VIDEO_OUT, O_RDWR);
    if(fd < 0) {
        cerr << "ERROR: failed to open output device!\n" << strerror(errno);
        return -2;
    }

    // CONFIGURE VIDEO FORMAT

    struct v4l2_format vid_format;
    memset(&vid_format, 0, sizeof(vid_format));
    vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    if (ioctl(fd, VIDIOC_G_FMT, &vid_format) < 0) {
        cerr << "ERROR: failed to get default loopback video format!\n" << strerror(errno);
        return -1;
    }

    cout << "output dim: (" << width << ", " << height << ")\n";
    cout << "output colorspace: " << vid_format.fmt.pix.colorspace << "\n";

    // YUYV encodes every 2 pixels with a 4 byte "YUYV", where each pixel has its own Y (luma)
    // but the two pixels share the same U and V (chroma) values.
    size_t num_pixels = width * height;
    // size_t framesize = ((num_pixels + 1) / 2) * 4;
    size_t framesize = num_pixels * 1.5;
    // size_t framesize = num_pixels * 3;
    vid_format.fmt.pix.width = width;
    vid_format.fmt.pix.height = height;
    vid_format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    vid_format.fmt.pix.sizeimage = framesize;
    vid_format.fmt.pix.field = V4L2_FIELD_NONE;
    vid_format.fmt.pix.xfer_func = V4L2_XFER_FUNC_709;

    if (ioctl(fd, VIDIOC_S_FMT, &vid_format) < 0) {
        cerr << "ERROR: failed to set loopback video format!\n" << strerror(errno);
        return -1;
    }

    *lb = loopback_info_t {
        width,
        height,
        framesize,
        fd
    };
    return 0;
}

int output_inner(frame_chan_t &frame_chan, loopback_info_t &lb) {
    while (true) {
        Mat frame;
        if (boost::fibers::channel_op_status::success != frame_chan.pop(frame)) {
            cerr << "ERROR: failed to recv frame from main loop!";
            close(lb.fd);
            frame_chan.close();
            return -1;
        }

        // convert back to yuv420
        cvtColor(frame, frame, COLOR_BGR2YUV_I420);
        size_t bytes_written = write(lb.fd, frame.data, lb.framesize);

        if (bytes_written < 0) {
            cerr << "ERROR: failed to write frame to loopback device!";
            close(lb.fd);
            frame_chan.close();
            return -2;
        }
    }
    close(lb.fd);
    frame_chan.close();
}

int output_inner_fiber(frame_chan_t &frame_chan, loopback_info_t lb) {
    boost::fibers::fiber f(bind(output_inner, ref(frame_chan), ref(lb)));
    f.join();
    return 0;
}

std::optional<thread> output(frame_chan_t &frame_chan, int width, int height) {
    loopback_info_t lb;
    int res = configure_loopback(&lb, width, height);
    if (res < 0) {
        return {};
    }

    thread t(output_inner_fiber, ref(frame_chan), ref(lb));
    return t;
}

void print_fourcc(int fourcc) {
    string fourcc_str = format("%c%c%c%c", fourcc & 255, (fourcc >> 8) & 255, (fourcc >> 16) & 255, (fourcc >> 24) & 255);
    cout << "CAP_PROP_FOURCC: " << fourcc_str << endl;
}

void print_mat_type(int type) {
  string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  cout << "CAP_PROP_FORMAT: " << r << "\n";
}

int input_inner(frame_chan_t &frame_chan, VideoCapture cap, int width, int height) {
    Mat frame;
    while (true) {
        cap.read(frame);

        if (frame.empty()) {
            cerr << "ERROR: failed to read frame!";
            frame_chan.close();
            return -1;
        }

        resize(frame, frame, Size(width, height), 0, 0, CV_INTER_LINEAR);

        frame_chan.push(frame);
    }
    frame_chan.close();
}

int input_inner_fiber(frame_chan_t &frame_chan, VideoCapture cap) {
    boost::fibers::fiber f(bind(input_inner, ref(frame_chan), ref(cap)));
    f.join();
    return 0;
}

// https://stackoverflow.com/questions/24988164/c-fast-screenshots-in-linux-for-use-with-opencv
std::pair<std::pair<int, int>, thread> input(frame_chan_t &camera_chan) {
    VideoCapture cap(0, CAP_V4L);

    cap.set(CAP_PROP_CONVERT_RGB, 1);
    
    if (not cap.isOpened()) {
        cerr << "ERROR: failed to open camera!\n";
        camera_chan.close();
        exit(-2);
    }

    int fourcc = cap.get(CAP_PROP_FOURCC);
    int format = cap.get(CAP_PROP_FORMAT);
    print_fourcc(fourcc);
    print_mat_type(format);

    size_t width = 640;
    size_t height = 480; 
    cap.set(CAP_PROP_FRAME_WIDTH, width);
    cap.set(CAP_PROP_FRAME_HEIGHT, height);
    cap.set(CAP_PROP_FPS, 30);

    thread t(input_inner(camera_chan, cap, width, height));

    std::pair<int, int> size = std::make_pair(height, width);
    return make_pair(size, t);
}


ABSL_FLAG(std::string, calculator_graph_config_file, "",
          "Name of file containing text format CalculatorGraphConfig proto.");

absl::Status main_inner() {

    LOG(INFO) << "Loading calculator graph file: ";
    std::string calculator_graph_config_contents;
    MP_RETURN_IF_ERROR(mediapipe::file::GetContents(
        absl::GetFlag(FLAGS_calculator_graph_config_file),
        &calculator_graph_config_contents));


    // parse graph file
    LOG(INFO) << "Parsing calculator graph file: "
                << calculator_graph_config_contents;
    mediapipe::CalculatorGraphConfig config =
        mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig>(
            calculator_graph_config_contents);



    LOG(INFO) << "Initializing calculator graph: ";
    mediapipe::CalculatorGraph graph;
    MP_RETURN_IF_ERROR(graph.Initialize(config));


    LOG(INFO) << "Starting execution of calculator graph: ";
    ASSIGN_OR_RETURN(mediapipe::OutputStreamPoller poller,
                    graph.AddOutputStreamPoller(GRAPH_OUTPUT_STREAM_NAME));
    MP_RETURN_IF_ERROR(graph.StartRun({}));

    LOG(INFO) << "Starting input thread: ";
    frame_chan_t camera_chan { 2 };
    auto input_size_and_thread = input(camera_chan);

    int height = input_size_and_thread.first.first;
    int width = input_size_and_thread.first.second;
    thread input_thread = std::move(input_size_and_thread.second);


    LOG(INFO) << "Starting output thread: ";
    frame_chan_t output_frame_chan { 2 };
    std::optional<thread> output_thread_wrapped = output(output_frame_chan, width, height);

    LOG(INFO) << "Check to see if output initialization succeded";
    RET_CHECK(output_thread_wrapped.has_value());
    thread output_thread = std::move(output_thread_wrapped.value());


    LOG(INFO) << "Starting main loop: ";
    while (true) {
        // read incoming RGB frame from input thread
        // blocks util complete
        Mat input_frame;
        if (boost::fibers::channel_op_status::success != camera_chan.pop(input_frame)) {
            LOG(ERROR) << "ERROR: failed to recv frame from camera input thread!\n";
            break;
        }

        size_t frame_in_timestamp_us =
            (double)getTickCount() / (double)getTickFrequency() * 1e6;

        // Wrap Mat into a mediapipe "ImageFrame" packet
        auto input_frame_packet = absl::make_unique<mediapipe::ImageFrame>(
            mediapipe::ImageFormat::SRGB, input_frame.cols, input_frame.rows,
            mediapipe::ImageFrame::kDefaultAlignmentBoundary);
        Mat input_frame_packet_mat = mediapipe::formats::MatView(input_frame_packet.get());
        input_frame.copyTo(input_frame_packet_mat);

        // Feed the "ImageFrame" packet into the mediapipe graph
        // blocks until complete
        MP_RETURN_IF_ERROR(graph.AddPacketToInputStream(
            GRAPH_INPUT_STREAM_NAME, mediapipe::Adopt(input_frame_packet.release())
                            .At(mediapipe::Timestamp(frame_in_timestamp_us))));

        // Get next output packet from the mediapipe graph and immediately return if it fails
        // blocks until complete
        mediapipe::Packet packet;
        if (!poller.Next(&packet)) break;
        auto& output_frame_packet = packet.Get<mediapipe::ImageFrame>();

        // unwrap "ImageFrame" packet back into a cv::Mat and send it to the output thread]
        // blocks if output_frame_chan's internal buffer is full
        Mat output_frame = mediapipe::formats::MatView(&output_frame_packet);
        if (boost::fibers::channel_op_status::success != output_frame_chan.push(output_frame)) {
            LOG(ERROR) << "ERROR: failed to send mediapipe output frame to output thread!\n";
            break;
        }
    }

    camera_chan.close();
    output_frame_chan.close();
    input_thread.join();
    output_thread.join();
    return absl::OkStatus();
}

int main(int argc, char** argv) {
    google::InitGoogleLogging(argv[0]);
    absl::ParseCommandLine(argc, argv);
    absl::Status run_status = main_inner();
    if (!run_status.ok()) {
        LOG(ERROR) << "Execution failed: " << run_status.message();
        return EXIT_FAILURE;
    }
    LOG(INFO) << "Execution finished successfully";
    return EXIT_SUCCESS;
}