import cv2
import numpy as np
from collections import deque


typedef std::tuple<int, int> point;

template<typename T>
static inline void push_back(const typename std::deque<T>::size_type max_size, std::deque<T>& deque, const T& value){

    while (deque.size() >= max_size) {
        deque.pop_front();
    }
    deque.push_back(value);
}


int main(){
    cv2.namedWindow("Color detectors")
    cv2.createTrackbar("Upper Hue", "Color detectors", 153, 180, setValues)
    cv2.createTrackbar("Upper Saturation", "Color detectors", 255, 255, setValues)
    cv2.createTrackbar("Upper Value", "Color detectors", 255, 255, setValues)
    cv2.createTrackbar("Lower Hue", "Color detectors", 64, 180, setValues)
    cv2.createTrackbar("Lower Saturation", "Color detectors", 72, 255, setValues)
    cv2.createTrackbar("Lower Value", "Color detectors", 49, 255, setValues)

    std::deque<point> bpoints;
    std::deque<point> gpoints;
    std::deque<point> rpoints;
    std::deque<point> ypoints;


    blue_index = 0
    green_index = 0
    red_index = 0
    yellow_index = 0

    colors = [(255, 0, 0), (0, 255, 0),
              (0, 0, 255), (0, 255, 255)]
    colorIndex = 0
    kernel = numcpp::ones<uint8>(5,5)
    //paintWindow = numcpp::zeros((471, 636, 3)) + 255
    //kernel = np.ones((5, 5), np.uint8)

    //paintWindow = np.zeros((471, 636, 3)) + 255

    cv2.namedWindow('Paint', cv2.WINDOW_AUTOSIZE)

    capture = cv2.VideoCapture(0)
    while (capture.isOpened()/*Check type*/){
        pressed_key = cv2.waitKey(1)/*Check type*/
        //#Should use ret value for error detection.
        ret, frame = capture.read()
        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
        frame = cv2.flip(frame, 1)
        hsv = cv2.flip(hsv, 1)

        if(pressed_key & 0xFF == ord('z')){
            is_hand_hist_created = True
            hand_hist = hand_histogram(frame)
        }
        if(pressed_key & 0xFF == ord('d')){
            bpoints = [deque(maxlen = 512)]
            gpoints = [deque(maxlen = 512)]
            rpoints = [deque(maxlen = 512)]
            ypoints = [deque(maxlen = 512)]

            blue_index = 0
            green_index = 0
            red_index = 0
            yellow_index = 0

            //paintWindow[67:, :, :] = 255
        }
        if(pressed_key & 0xFF == ord('c')){
            draw_mode = not draw_mode
        }
        if(is_hand_hist_created){
            manage_image_opr(frame, hand_hist,bpoints,draw_mode)

            print(bpoints)
            points = [bpoints, gpoints, rpoints, ypoints]
            if(draw_mode){
                for i in range(len(points)){
                    for j in range(len(points[i])){
                        for k in range(1, len(points[i][j])){
                            if points[i][j][k - 1] is None or points[i][j][k] is None{// Fuck how do I convert this line
                                continue
                            }
                            cv2.line(frame, points[i][j][k - 1], points[i][j][k], colors[i], 2)
                            //cv2.line(paintWindow, points[i][j][k - 1], points[i][j][k], colors[i], 2)

                        }
                    }
                }
            }

        }
        else{
            frame = draw_rect(frame)
        }
    }



        cv2.imshow("Live Feed", rescale_frame(frame))

        if(pressed_key == 27){:
            break
        }
    }
    cv2.destroyAllWindows()
    capture.release()
    return 0;
    }
