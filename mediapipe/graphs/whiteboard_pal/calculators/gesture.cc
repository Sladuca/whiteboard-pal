
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/calculator_options.pb.h"
#include "mediapipe/framework/port/opencv_core_inc.h"
#include "mediapipe/framework/port/opencv_imgproc_inc.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include <algorithm> //for min function
#include <cmath> //for abs function

namespace mediapipe {

    constexpr char LANDMARKS_TAG[] = "LANDMARKS";
    // don't care about handedness for now
    // constexpr char HANDEDNESS_TAG[] = "HANDEDNESS";
    constexpr char COORDS_TAG[] = "DRAW_COORDS";
    constexpr char HAS_GESTURE_TAG[] = "HAS_GESTURE";

    class WhiteboardPalGestureDetectionCalculator: public CalculatorBase {
        public:
            static absl::Status GetContract(CalculatorContract* cc) {
                // set types for landmark inputs
                cc->Inputs().Tag(LANDMARKS_TAG).Set<std::vector<NormalizedLandmarkList, std::allocator<NormalizedLandmarkList>>>();
                // set types for output streams
                cc->Outputs().Tag(COORDS_TAG).Set<std::pair<float, float>>();
                cc->Outputs().Tag(HAS_GESTURE_TAG).Set<bool>();

                return absl::OkStatus();
            }

            // TODO: get rid of this and actually set the timestamps properly
            absl::Status Open(CalculatorContext* cc) {
                cc->SetOffset(TimestampDiff(0));
                return absl::OkStatus();
            }

            // ! expects the substrate mat to be in RGB format
            absl::Status Process(CalculatorContext* cc) {
                const auto &landmark_vec = cc->Inputs().Tag(LANDMARKS_TAG).Get<std::vector<NormalizedLandmarkList,
                                                                        std::allocator<NormalizedLandmarkList>>>();
                const auto landmark_list = landmark_vec[0];

                //get finger coordinates
                //index finger
                float index_x = landmark_list.landmark(8).x();
                float index_y = landmark_list.landmark(8).y();
                float index_dip = landmark_list.landmark(7).y();
                //thumb
                float thumb_x = landmark_list.landmark(4).x();
                float thumb_y = landmark_list.landmark(4).y();
                float thumb_knuckle_x = landmark_list.landmark(2).x();
                float thumb_knuckle_y = landmark_list.landmark(2).y();
                //middle finger
                float middle_x = landmark_list.landmark(12).x();
                float middle_y = landmark_list.landmark(12).y();
                float knuckle_x = landmark_list.landmark(9).x();
                float knuckle_y = landmark_list.landmark(9).y();
                float middle_pip_x = landmark_list.landmark(10).x();
                float middle_pip_y = landmark_list.landmark(10).y();
                //ring and pinky
                float ring_y = landmark_list.landmark(16).y();
                float pinky_y = landmark_list.landmark(20).y();

                float key_x = std::min(knuckle_x, middle_x);
                float key_y = (key_x == knuckle_x) ? knuckle_y:middle_y;

                bool clenched = (pinky_y > knuckle_y) && (middle_y > knuckle_y) && (pinky_y > knuckle_y);
                bool is_gesture = (get_dist(middle_pip_x, middle_pip_y, thumb_x, thumb_y) > get_dist(index_x, index_y, knuckle_x, knuckle_y)/1.7f) && \
                  clenched && (index_dip > index_y);
                /*
                bool is_gesture = (std::abs(key_x - thumb_x) > std::abs(index_y - knuckle_y)/1.7f) && \
                  clenched && (index_dip > index_y);
                */
                //LOG(INFO) << "b";

                auto coords = absl::make_unique<std::pair<float, float>>();
                auto has_gesture = absl::make_unique<bool>();

                coords->first = index_x;
                coords->second = index_y;
                *has_gesture = is_gesture;

                cc->Outputs().Tag(COORDS_TAG).Add(coords.release(), cc->InputTimestamp()); //need to potentially turn into floats
                cc->Outputs().Tag(HAS_GESTURE_TAG).Add(has_gesture.release(), cc->InputTimestamp());
                return absl::OkStatus();
            }

        private:
          float get_dist(float a_x, float a_y, float b_x, float b_y){
              float dist = std::pow(a_x - b_x, 2) + pow(a_y - b_y, 2);
              return std::sqrt(dist);
          }
    };
    REGISTER_CALCULATOR(WhiteboardPalGestureDetectionCalculator);
}
