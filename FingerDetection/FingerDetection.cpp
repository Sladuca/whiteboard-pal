
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/hal/intrin.hpp>


#include <iostream>
#include <stdio.h>
#include <deque>

#define MORPH_SIZE 2
#define EROSION_SIZE 2
#define DIALATION_SIZE 2

typedef std::tuple<int, int> point;

using namespace cv;
using namespace std;

void print_point_vec(vector<Point> &vec){
  //if(vec == NULL) return;
  if(vec.size() == 0) return;
  for(int i = 0; i < vec.size(); i++){
    printf("(%d, %d) ",vec[i].x,vec[i].y);
  }
  printf("Vector Size: %ld\n",vec.size());
}
/*
void print_mat(Mat &mt){
  for(int i = 0; i < mt.rows; i++){
    for(int j = 0; j < mt.cols; j++){
      printf("%d ",mt[i*mt.cols+j]);
    }
    printf("\n");
  }
  
}
*/

string type2str(int type) {
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

  return r;
}

void finger_tracking(Mat frame, int idx, std::deque<Point> &points){
  Mat hsv, hsv_range, erosion, morph, dialation, bw, gray; /*bw bw bw bw bw bw*/
  Mat erosion_ele, morph_ele, dialation_ele;
  cvtColor(frame,hsv, COLOR_BGR2HSV);
  printf("First\n");
  //vector<Mat> contours;
  std::vector<std::vector<cv::Point> > contours;
  inRange(hsv, Scalar(107, 72, 49), Scalar(153, 255, 255), hsv_range); // Used Jenny's filtering constants
  //These were just 2d arrays of ones before
  erosion_ele    = getStructuringElement(0/*Morph Rect*/,
                           Size(2*EROSION_SIZE+1,2*EROSION_SIZE+1),
			   Point(EROSION_SIZE,EROSION_SIZE)); //Default point maybe should be 5,5
  morph_ele      = getStructuringElement(0/*Morph Rect*/,
                           Size(2*MORPH_SIZE+1,2*MORPH_SIZE+1),
			   Point(MORPH_SIZE,MORPH_SIZE));
  dialation_ele  = getStructuringElement(0/*Morph Rect*/,
                           Size(2*DIALATION_SIZE+1,2*DIALATION_SIZE+1),
			   Point(DIALATION_SIZE,DIALATION_SIZE));
 
  //prin_mat(ref(erosion_ele));
  //cout << "Erosion = " << endl << " "  << erosion_ele << endl << endl;
  //cout << "Morph = " << endl << " "  << erosion_ele << endl << endl;
  //cout << "Dialation = " << endl << " "  << erosion_ele << endl << endl;
  //printf("second\n");
  //printf("Erosion Size: %dx%d",erosion_ele.rows,erosion_ele.cols);
  //imshow("1: hsv_image", hsv_range);
  erode(hsv_range,erosion,erosion_ele);
  //imshow("2: erpsion", erosion);
  morphologyEx(erosion, morph, 2 /*morph_open*/, morph_ele);
  //imshow("3: morph cleanup", morph);
  dilate(morph, dialation,dialation_ele);
  imshow("4: dialation", dialation);
  //dialation mask should be the noiseless binary output of hand position
  
  //string ty =  type2str( dialation.type() );
  //printf("matrix: %s %dx%d \n", ty.c_str(), dialation.cols, dialation.rows );
  
  //printf("third\n");
  // these two defines are located in cv::traits
  //cvstartfindcontours_impl(frame, contours, 0/*cv_retr_external*/, 2/*cv_chain_approx_simple*/);
  //cvtcolor(dialation, gray, imgproc.cv_bgr2gray);
  //printf("this is the type of matrix it is %s \n", cv_mat_type(dialation));
  //if(cv_mat_type(dialation) == cv_32sc1) printf("radical re\n\n");
  
  Mat hsv_channels[3];
  split(dialation,hsv_channels);
  
  findContours(hsv_channels[0], contours, 0 /*CV_RETR_EXTERNAL*/, 2 /*CV_CHAIN_APPROX_SIMPLE*/);
  vector<Point> max_cont;
  double max_cont_area = 0;
  int max_cont_idx     = 0;

  for(int i = 0; i< contours.size(); i++){

      double area = fabs(contourArea(contours[i]));
      if( area > max_cont_area){
        max_cont_area = area;
        max_cont      = contours[i];
        max_cont_idx  = i;
    }
  }
  printf("Contour List Size: %ld, Index of Maximum Area Contour: %d\n",contours.size(), max_cont_idx);
  //print_point_vec(ref(contours[max_cont_idx]));
  //Farthest point
  //New way to find max centroid
  printf("Fourth\n");
  Moments m = moments(max_cont, true);
  cv::Point farthest_point;
  Point centroid(m.m10/m.m00, m.m01/m.m00);
  


  
  
  Mat contour_mat(frame.size(), CV_8UC3, Scalar(0,0,0));
  Scalar colors[3];
  colors[0] = Scalar(255,0,0);
  colors[1] = Scalar(0,255,0);
  colors[2] = Scalar(0,0,255);
  drawContours(contour_mat,contours, max_cont_idx,colors[0]);
  

  printf("Fifth\n");
  imshow("5: Contour Display", contour_mat);
  //Slow method
  
  int max_dist = 0;
  int max_idx  = 0;
  for(int i = 0; i< max_cont.size(); i++){
    int tmp1 = (max_cont[i].x - centroid.x);
    int tmp2 = (max_cont[i].y - centroid.y);

    int tmp = tmp1 * tmp1 + tmp2 * tmp2;
    if(tmp > max_dist){
      max_idx = i;
      max_dist = tmp;
    }
  }
  printf("Sixth\n");
  //print_point_vec(ref(max_cont));
  points.push_back(max_cont[max_idx]);
  printf("Seventh\n");
  if(points.size() > 1){
    for(int i = 0; i < points.size()-1; i++){
      line(frame,points[i], points[i+1], Scalar(0,0,255), 2, LINE_4);
    }
  }

  printf("Vec size %d\n", points.size());
  printf("Clearly We're making it to this line of code" );
  imshow("Original Frame", frame);  
  return;
}


namespace {
    void help(char** av) {
        cout << "The program captures frames from a video file, image sequence (01.jpg, 02.jpg ... 10.jpg) or camera connected to your computer." << endl
             << "Usage:\n" << av[0] << " <video file, image sequence or device number>" << endl
             << "q,Q,esc -- quit" << endl
             << "space   -- save frame" << endl << endl
             << "\tTo capture from a camera pass the device number. To find the device number, try ls /dev/video*" << endl
             << "\texample: " << av[0] << " 0" << endl
             << "\tYou may also pass a video file instead of a device number" << endl
             << "\texample: " << av[0] << " video.avi" << endl
             << "\tYou can also pass the path to an image sequence and OpenCV will treat the sequence just like a video." << endl
             << "\texample: " << av[0] << " right%%02d.jpg" << endl;
    }


    int process(VideoCapture& capture) {
        int n = 0;
        char filename[200];
        string window_name = "video | q or esc to quit";
        cout << "press space to save a picture. q or esc to quit" << endl;
        namedWindow(window_name, WINDOW_KEEPRATIO); //resizable window;
        Mat frame;
        std::deque<Point> dq;
        for (;;) {
            capture >> frame;
            if (frame.empty())
                break;

            imshow(window_name, frame);
	    
	    finger_tracking(frame, 0, ref(dq));
            char key = (char)waitKey(30); //delay N millis, usually long enough to display and capture input

            switch (key) {
            case 'q':
            case 'Q':
            case 27: //escape key
                return 0;
            case ' ': //Save an image
                sprintf(filename,"filename%.3d.jpg",n++);
                imwrite(filename,frame);
                cout << "Saved " << filename << endl;
                break;
            default:
                break;
            }
        }
        return 0;
    }
}


/*
template<typename T>
static inline void push_back(const typename std::deque<T>::size_type max_size, std::deque<T>& deque, const T& value){
    
    while (deque.size() >= max_size) {
        deque.pop_front();
    }
    deque.push_back(value);
}

*/




int main(int ac, char** av) {
    cv::CommandLineParser parser(ac, av, "{help h||}{@input||}");
    if (parser.has("help"))
    {
        help(av);
        return 0;
    }
    std::string arg = parser.get<std::string>("@input");
    if (arg.empty()) {
        help(av);
        return 1;
    }
    VideoCapture capture(arg); //try to open string, this will attempt to open it as a video file or image sequence
    if (!capture.isOpened()) //if this fails, try to open as a video camera, through the use of an integer param
        capture.open(atoi(arg.c_str()));
    if (!capture.isOpened()) {
        cerr << "Failed to open the video device, video file or image sequence!\n" << endl;
        help(av);
        return 1;
    }
    return process(capture);
}



