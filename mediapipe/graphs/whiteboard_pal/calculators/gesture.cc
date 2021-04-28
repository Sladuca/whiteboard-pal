
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
                cc->Outputs().Tag(COORDS_TAG).Set<std::pair<int, int>>();
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
                std::cout << "got landmark list" << "\n";

                //get finger coordinates
                //index finger
                float index_x = landmark_list.landmark(8).x();
                float index_y = landmark_list.landmark(8).y();
                float index_dip = landmark_list.landmark(7).y();
                //thumb
                float thumb_x = landmark_list.landmark(4).x();
                float thumb_y = landmark_list.landmark(4).y();
                //middle finger
                float middle_x = landmark_list.landmark(12).x();
                float middle_y = landmark_list.landmark(12).y();
                float knuckle_x = landmark_list.landmark(9).x();
                float knuckle_y = landmark_list.landmark(9).y();
                //ring and pinky
                float ring_y = landmark_list.landmark(16).y();
                float pinky_y = landmark_list.landmark(20).y();

                float key_x = std::min(knuckle_x, middle_x);

                bool clenched = (pinky_y > knuckle_y) && (middle_y > knuckle_y) && (pinky_y > knuckle_y);
                bool is_gesture = (std::abs(key_x - thumb_x) > std::abs(index_y - knuckle_y)/1.7f) && \
                  clenched && (index_dip > index_y);


                
                auto coords = absl::make_unique<std::pair<float, float>>();
                auto has_gesture = absl::make_unique<bool>();

                coords.get()->first = index_x;
                coords.get()->second = index_y;
                *(has_gesture.get()) = is_gesture;

                cc->Outputs().Tag(COORDS_TAG).Add(coords.release(), cc->InputTimestamp()); //need to potentially turn into floats
                cc->Outputs().Tag(HAS_GESTURE_TAG).Add(has_gesture.release(), cc->InputTimestamp());
                return absl::OkStatus();
            }

    };
    REGISTER_CALCULATOR(WhiteboardPalGestureDetectionCalculator);
}
