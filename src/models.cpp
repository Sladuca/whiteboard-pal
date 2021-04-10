#include <whiteboard_pal/main.hpp>

// frame: Matrix of the current frame in BGR24 format, that is, the mat entries are 3-bytes deep, each byte representing the B, G, R respectively
// idx: index of the frame, if that's useful for some reason
finger_output_t finger_tracking(Mat frame, int idx) {
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

    imwrite("frame.jpg", final_resized);

    //inputting into model
    Mat out = imread("frame.jpg");
    auto input = cppflow::decode_jpeg(cppflow::read_file(std::string("frame.jpg")));
    input = cppflow::cast(input, TF_UINT8, TF_FLOAT);
    input = cppflow::expand_dims(input, 0);
    //run through model
    std::vector<cppflow::tensor> output = model({{"serving_default_conv2d_input:0", input}},{"StatefulPartitionedCall:0"});
    cppflow::tensor res = output[0];
    std::vector<float> prediction = res.get_data<float>();
    bool gesture = false;
    if (prediction[0] == 0)
      gesture = true;
    if (gesture)
      std::cout << "gesture" << std::endl;
    else
      std::cout << "no gesture" << std::endl;

    return gesture_output_t {
        gesture,
        idx
    };
}
