
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
                auto stub_coords = absl::make_unique<std::pair<int, int>>();
                stub_coords->first = 100;
                stub_coords->second = 100;

                auto stub_has_gesture = absl::make_unique<bool>();
                *(stub_has_gesture.get()) = true;

                cc->Outputs().Tag(COORDS_TAG).Add(stub_coords.release(), cc->InputTimestamp());
                cc->Outputs().Tag(HAS_GESTURE_TAG).Add(stub_has_gesture.release(), cc->InputTimestamp());
                return absl::OkStatus();
            }

    };
    REGISTER_CALCULATOR(WhiteboardPalGestureDetectionCalculator);
}
