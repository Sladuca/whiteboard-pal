
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/calculator_options.pb.h"
#include "mediapipe/framework/port/opencv_core_inc.h"
#include "mediapipe/framework/port/opencv_imgproc_inc.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/formats/landmark.pb.h"

namespace mediapipe {

    constexpr char LANDMARKS_TAG[] = "LANDMARKS";
    // don't care about handedness for now
    // constexpr char HANDEDNESS_TAG[] = "HANDEDNESS";
    constexpr char COORDS_TAG[] = "COORDS";
    constexpr char HAS_GESTURE_TAG[] = "HAS_GESTURE";

    // for stub usage
    constexpr std::pair<int, int> stub_coords = std::make_pair(100, 100);
    constexpr bool stub_has_gesture = true;

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
                cc->Outputs().Tag(COORDS_TAG).Add(&stub_coords, cc->InputTimestamp());
                cc->Outputs().Tag(HAS_GESTURE_TAG).Add(&stub_has_gesture, cc->InputTimestamp());
                return absl::OkStatus();
            }

    };
    REGISTER_CALCULATOR(WhiteboardPalGestureDetectionCalculator);
}
