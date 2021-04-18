#include <whiteboard_pal/main.hpp>

#define MORPH_SIZE     2
#define EROSION_SIZE   2
#define DIALATION_SIZE 2

// frame: Matrix of the current frame in BGR24 format, that is, the mat entries are 3-bytes deep, each byte representing the B, G, R respectively
// idx: index of the frame, if that's useful for some reason
finger_output_t finger_tracking(Mat frame, int idx/*, std::deque<point> points*/) {
    Mat hsv, hsv_range, erosion, morph, dialation, bw; /*bw bw bw bw bw bw*/
    Mat erosion_ele, morph_ele, dialation_ele;
    cvtColor(frame,hsv, COLOR_BGR2HSV);
    
    // vector<Mat> contours;
    std::vector<std::vector<cv::Point> > contours;

    inRange(hsv, Scalar(60, 90, 50), Scalar(140, 255, 255), hsv_range); // Used Jenny's filtering constants
    // These were just 2d arrays of ones before
    erosion_ele    = getStructuringElement(0/*Morph Rect*/,
		                           Size(2*EROSION_SIZE+1,2*EROSION_SIZE+1),
					   Point(EROSION_SIZE,EROSION_SIZE)); //Default point maybe should be 5,5
    morph_ele      = getStructuringElement(0/*Morph Rect*/,
		                           Size(2*MORPH_SIZE+1,2*MORPH_SIZE+1),
					   Point(MORPH_SIZE,MORPH_SIZE));
    dialation_ele  = getStructuringElement(0/*Morph Rect*/,
		                           Size(2*DIALATION_SIZE+1,2*DIALATION_SIZE+1),
					   Point(DIALATION_SIZE,DIALATION_SIZE));
    erode(hsv,erosion,erosion_ele);
    morphologyEx(erosion, morph, 2 /*MORPH_OPEN*/, morph_ele);
    dilate(morph, dialation,dialation_ele);
    //Dialation mask should be the noiseless binary output of hand position
    // These two defines are located in cv::traits
    findContours(bw.clone(), contours, 0/*CV_RETR_EXTERNAL*/, 2/*CV_CHAIN_APPROX_SIMPLE*/);
    vector<Point> max_cont;
    float max_cont_area = 0.0;
    for(int i = 0; i< contours.size; i++){
        float area = fabs(contourArea(contours[i]));
	if( area > max_cont_area){
        max_cont_area = area;
	max_cont = contours[i];
      }
    }
    
    //Farthest point
    //New way to find max centroid
   
    Moments m = moments(max_cont, true);
    cv::Point farthest_point;
    Point centroid(m.m10/m.m00, m.m01/m.m00);
    for(int k = 0; k < max_cont.size; k++){
      float new_dist = dist(centroid, max_cont[k]);
      if(new_dist > old_dist){
	old_dist = new_dist;
        farthest_point = max_cont[k];
      }
    }
    
    
    
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
    Mat frame_HSV, frame_threshold, frame_blur, final, final_resized;
    cvtColor(frame, frame_HSV, COLOR_BGR2HSV);
    inRange(frame_HSV, Scalar(60, 90, 50), Scalar(140, 255, 255), frame_threshold);
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
