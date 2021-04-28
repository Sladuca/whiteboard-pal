#include <whiteboard_pal/main.hpp>

#define MORPH_SIZE     2
#define EROSION_SIZE   2
#define DIALATION_SIZE 2

// frame: Matrix of the current frame in BGR24 format, that is, the mat entries are 3-bytes deep, each byte representing the B, G, R respectively
// idx: index of the frame, if that's useful for some reason
finger_output_t finger_tracking(Mat frame, int idx, deque<Point> &points) {
    Mat hsv, hsv_range, erosion, morph, dialation, bw, gray; /*bw bw bw bw bw bw*/
    Mat erosion_ele, morph_ele, dialation_ele;
    cvtColor(frame,hsv, COLOR_BGR2HSV);
    
    printf("First\n");
    //vector<Mat> contours;
    std::vector<std::vector<cv::Point> > contours;
    inRange(hsv, Scalar(60, 90, 50), Scalar(140, 255, 255), hsv_range); // Used Jenny's filtering constants
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
    printf("second\n");
    
    //imshow("1: hsv_image", hsv);
    erode(hsv_range,erosion,erosion_ele);
    //imshow("2: erpsion", erosion);
    morphologyEx(erosion, morph, 2 /*morph_open*/, morph_ele);
    //imshow("3: morph cleanup", morph);
    dilate(morph, dialation,dialation_ele);
    //imshow("4: dialation", dialation);
    //dialation mask should be the noiseless binary output of hand position
    
    //string ty =  type2str( dialation.type() );
    //printf("matrix: %s %dx%d \n", ty.c_str(), dialation.cols, dialation.rows );
    
    printf("third\n");
    // these two defines are located in cv::traits
    //cvstartfindcontours_impl(frame, contours, 0/*cv_retr_external*/, 2/*cv_chain_approx_simple*/);
    //cvtcolor(dialation, gray, imgproc.cv_bgr2gray);
    //printf("this is the type of matrix it is %s \n", cv_mat_type(dialation));
    //if(cv_mat_type(dialation) == cv_32sc1) printf("radical re\n\n");
   
    Mat hsv_channels[3];
    split(dialation,hsv_channels);
    
    findContours(hsv_channels[0] , contours, 0 /*CV_RETR_EXTERNAL*/, 2 /*CV_CHAIN_APPROX_SIMPLE*/);
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
    //imshow("5: Contour Display", contour_mat);
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
    //printf("Sixth\n");
    //print_point_vec(ref(max_cont));
    points.push_back(max_cont[max_idx]);
    if(points.size() > 1){
      for(int i = 0; i < points.size()-1; i++){
        line(frame, points[i], points[i+1], Scalar(0,0,255), 2, LINE_4);
      }
    }
    printf("Seventh\n");
    return finger_output_t {
        200,
        200,
        idx
    };
}

// frame: Matrix of the current frame in BGR24 format, that is, the mat entries are 3-bytes deep, each byte representing the B, G, R respectively
// idx: index of the frame, if that's useful for some reason
gesture_output_t gesture_detection(cppflow::model model, Mat frame, int idx) {
    flip(frame, frame, 1);
    //convert to HSV color space, then threshold
    Mat frame_threshold, frame_blur, final, final_resized;
    cvtColor(frame, frame, COLOR_BGR2HSV);
    inRange(frame, Scalar(60, 90, 50), Scalar(140, 255, 255), frame_threshold);
    blur(frame_threshold, frame_blur, Size(2, 2));
    threshold(frame_blur, final, 0, 255, THRESH_BINARY);
    resize(final, final_resized, Size(250, 250));

    //resize matrix to be flat so we can put it into tensor
    Mat flat = final_resized.reshape(1, final_resized.total() * final_resized.channels());
    std::vector<float> img_data(250*250);

    //shape of the image (rows, cols, channels)
    std::vector<int64_t> shape(3);
    shape[0] = 250;
    shape[1] = 250;
    shape[2] = 1;
    img_data = flat.clone();

    //create input tensor
    cppflow::tensor input(img_data, shape);
    input = cppflow::expand_dims(input, 0);

    //get output
    std::vector<cppflow::tensor> output = model({{"serving_default_conv2d_input:0", input}},
      {"StatefulPartitionedCall:0"});
    cppflow::tensor res = output[0];
    std::vector<float> prediction = res.get_data<float>();
    bool gesture = false;

    if (prediction[0] == 0) {
      gesture = true;
    }
    // if (gesture)
    //   std::cout << "gesture" << std::endl;
    // else
    //   std::cout << "no gesture" << std::endl;

    return gesture_output_t {
        gesture,
        idx
    };
}
