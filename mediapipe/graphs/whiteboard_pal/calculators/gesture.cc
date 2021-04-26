
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/calculator_options.pb.h"
#include "mediapipe/framework/port/opencv_core_inc.h"
#include "mediapipe/framework/port/opencv_imgproc_inc.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"

namespace mediapipe {

    constexpr char DETECTIONS_TAG[] = "DETECTIONS";
    constexpr char LANDMARKS_TAG[] = "LANDMARKS";
    constexpr char HANDEDNESS_TAG[] = "HANDEDNESS";
    constexpr char NORM_RECTS_TAG[] = "NORM_RECTS";
    constexpr char COORDS_TAG[] = "COORDS";
    constexpr char HAS_GESTURE_TAG[] = "HAS_GESTURE";

    // for stub usage
    constexpr std::pair<int, int> stub_coords = std::make_pair(100, 100);
    constexpr bool stub_has_gesture = true;

    class WhiteboadPalGestureDetectionCalculator: public CalculatorBase {
        public: 
            static absl::Status GetContract(CalculatorContract* cc) {
                return absl::OkStatus();
            }

            // TODO: get rid of this and actually set the timestamps properly
            absl::Status Open(CalculatorContext* cc) {
                cc->SetOffset(TimestampDiff(0));
                return absl::OkStatus();
            }

            // ! expects the substrate mat to be in RGB format
            absl::Status Process(CalculatorContext* cc) {
                cc->Outputs().Tag(COORDS_TAG).Add(&stub_coords, cc->InputTimestamp());
                cc->Outputs().Tag(HAS_GESTURE_TAG).Add(&stub_has_gesture, cc->InputTimestamp());
                return absl::OkStatus();
            }

    };
    REGISTER_CALCULATOR(WhiteboadPalGestureDetectionCalculator);
}
