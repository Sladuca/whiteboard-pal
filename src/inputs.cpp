#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <whiteboard_pal/main.hpp>
#include <time.h>

#define WHITEBOARD_WIDTH 640
#define WHITEBOARD_HEIGHT 480


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

// https://stackoverflow.com/questions/24988164/c-fast-screenshots-in-linux-for-use-with-opencv
struct ScreenShot{
    ScreenShot() {
        ScreenShot::setup();
    }

    void setup() {
        display = XOpenDisplay(nullptr);
        root = DefaultRootWindow(display);

        XGetWindowAttributes(display, root, &window_attributes);
        screen = window_attributes.screen;
        width = window_attributes.width;
        height = window_attributes.height;
        x = window_attributes.x;
        y = window_attributes.y;
        ximg = XShmCreateImage(display, DefaultVisualOfScreen(screen), DefaultDepthOfScreen(screen), ZPixmap, NULL, &shminfo, width, height);

        shminfo.shmid = shmget(IPC_PRIVATE, ximg->bytes_per_line * ximg->height, IPC_CREAT|0777);
        shminfo.shmaddr = ximg->data = (char*)shmat(shminfo.shmid, 0, 0);
        shminfo.readOnly = False;
        if(shminfo.shmid < 0)
            puts("Fatal shminfo error!");;
        Status s1 = XShmAttach(display, &shminfo);
        printf("XShmAttach() %s\n", s1 ? "success!" : "failure!");

        init = true;
    }

    void operator() (cv::Mat& cv_img) {
        XEvent e;
        if (XCheckMaskEvent(display, -1, &e) && e.type == ResizeRequest) {
            // TODO: free existing stuff
            ScreenShot::setup();
        }

        if (init) {
            init = false;
        }

        XShmGetImage(display, root, ximg, 0, 0, 0x00ffffff);
        // TODO: see if we can get this in BGR instead
        cv_img = Mat(height, width, CV_8UC4, ximg->data);
        flip(cv_img, cv_img, 0);
        cvtColor(cv_img, cv_img, COLOR_BGRA2BGR);
    }

    ~ScreenShot(){
        if(!init)
            XDestroyImage(ximg);

        XShmDetach(display, &shminfo);
        shmdt(shminfo.shmaddr);
        XCloseDisplay(display);
    }

    Display* display;
    Window root;
    XWindowAttributes window_attributes;
    Screen* screen;
    XImage* ximg;
    XShmSegmentInfo shminfo;

    int x, y, width, height;

    bool init;
};

int input_screen(frame_chan_t &to_finger, frame_chan_t &to_gesture, cap_size_chan_t &broadcast_size) {
    ScreenShot cap;
    broadcast_size.push(capture_size_t { cap.width, cap.height });
    Mat frame;
    int i = 0;
    while (true) {
        cap(frame);
        if (frame.empty()) {
            cerr << "ERROR: failed to read frame!";
            return -1;
        }
        Mat frame2 = frame;
        to_finger.push(frame_with_idx_t { frame, i });
        to_gesture.push(frame_with_idx_t { frame2, i});
        i++;
    }
    to_finger.close();
    to_gesture.close();
    return 0;
}

int input_camera(frame_chan_t &to_finger, frame_chan_t &to_gesture, cap_size_chan_t &broadcast_size) {            
    VideoCapture cap(0, CAP_V4L);
    
    if (not cap.isOpened()) {
        cerr << "ERROR: failed to open camera!\n";
        return -2;
    }

    int fourcc = cap.get(CAP_PROP_FOURCC);
    int format = cap.get(CAP_PROP_FORMAT);
    print_fourcc(fourcc);
    print_mat_type(format);

    size_t width = cap.get(CAP_PROP_FRAME_WIDTH);
    size_t height = cap.get(CAP_PROP_FRAME_HEIGHT);

    broadcast_size.push(capture_size_t { width, height });
    Mat frame;
    int i = 0;
    while (true) {
        cap.read(frame);
        if (frame.empty()) {
            cerr << "ERROR: failed to read frame!";
            return -1;
        }
       Mat frame2 = frame;
        to_finger.push(frame_with_idx_t { frame, i });
        to_gesture.push(frame_with_idx_t { frame2, i});
        i++;
    }
    to_finger.close();
    to_gesture.close();
    return 0;
}

int input_white(frame_chan_t &to_finger, frame_chan_t &to_gesture, cap_size_chan_t &broadcast_size) {
    Mat white = Mat(cv::Size(640, 480), CV_8UC3, Scalar(255, 255, 255));
    broadcast_size.push(capture_size_t { WHITEBOARD_WIDTH, WHITEBOARD_HEIGHT });
    Mat frame;
    int i = 0;
    while (true) {
        frame = white;
        if (frame.empty()) {
            cerr << "ERROR: failed to read frame!";
            return -1;
        }
        Mat frame2 = frame;
        to_finger.push(frame_with_idx_t { frame, i });
        to_gesture.push(frame_with_idx_t { frame2, i});
        i++;
    }
    to_finger.close();
    to_gesture.close();
    return 0;
}

int input(frame_chan_t &to_finger, frame_chan_t &to_gesture, cap_size_chan_t &broadcast_size, capture_source_t source) {
    switch (source) {
        case WEBCAM:
            return input_camera(to_finger, to_gesture, broadcast_size);

        case SCREEN:
            return input_screen(to_finger, to_gesture, broadcast_size);

        case WHITEBOARD:
            return input_white(to_finger, to_gesture, broadcast_size);
    }
}