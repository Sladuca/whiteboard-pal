
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/calculator_options.pb.h"
#include "mediapipe/framework/port/opencv_core_inc.h"
#include "mediapipe/framework/port/opencv_imgproc_inc.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"

namespace mediapipe {

    constexpr char DRAW_COORDS_TAG[] = "DRAW_COORDS";
    constexpr char HAS_GESTURE_TAG[] = "HAS_GESTURE";
    constexpr char FRAME_HEIGHT_TAG[] = "FRAME_HEIGHT";
    constexpr char FRAME_WIDTH_TAG[] = "FRAME_WIDTH";
    constexpr char SUBSTRATE_TAG[] = "SUBSTRATE";
    constexpr char DRAWN_FRAME_TAG[] = "DRAWN_FRAME";

    class WhiteboadPalGestureDetectionCalculator: public CalculatorBase {
        cv::Mat canvas;
        cv::Scalar color;
        std::pair<int, int> size;
        int dot_width;

        public: 
            static absl::Status GetContract(CalculatorContract* cc) {
                cc->Inputs().Tag(DRAW_COORDS_TAG).Set<std::pair<int, int>>();
                cc->Inputs().Tag(SUBSTRATE_TAG).Set<cv::Mat>();
                cc->Outputs().Tag(DRAWN_FRAME_TAG).Set<cv::Mat>();
                return absl::OkStatus();
            }

            // TODO: get rid of this and actually set the timestamps properly
            absl::Status Open(CalculatorContext* cc) {
                cc->SetOffset(TimestampDiff(0));
                int width = cc->InputSidePackets().Tag(FRAME_WIDTH_TAG).Get<int>();
                int height = cc->InputSidePackets().Tag(FRAME_HEIGHT_TAG).Get<int>();

                this->size = std::make_pair(height, width);

                // TODO: add a packet for this so you can change the color and/or pen width at any time
                this->color = cv::Scalar(255, 0, 0);
                this->dot_width = 5;
                this->canvas = cv::Mat::zeros(size.first, size.second, CV_8UC1);
                return absl::OkStatus();
            }

            // ! expects the substrate mat to be in RGB format
            absl::Status Process(CalculatorContext* cc) {
                // throw an error if no coords
                RET_CHECK(cc->Inputs().Tag(HAS_GESTURE_TAG).IsEmpty());

                auto output_frame = absl::make_unique<cv::Mat>();

                // if no substrate, substrate is blank white
                if (cc->Inputs().Tag(SUBSTRATE_TAG).IsEmpty()) {
                    *output_frame.get() = cv::Mat(
                        this->size.first,
                        this->size.second,
                        CV_8UC3, 
                        cv::Scalar(255, 255, 255)
                    );
                } else {
                    *output_frame.get() = cc->Inputs().Tag(SUBSTRATE_TAG).Get<cv::Mat>();
                }


                // only update the canvas if gesture is detected
                if (!cc->Inputs().Tag(HAS_GESTURE_TAG).Get<bool>()) {
                    std::pair<int, int> coords = cc->Inputs().Tag(DRAW_COORDS_TAG).Get<std::pair<int, int>>();
                    this->update_canvas(coords);
                    return absl::OkStatus();
                }

                // apply substrate to the image and send
                output_frame.get()->setTo(this->color, this->canvas);
                cc->Outputs().Tag(DRAWN_FRAME_TAG).Add(output_frame.release(), cc->InputTimestamp());
            }

        private:
            void update_canvas(std::pair<int, int> coords) {
                
                if (coords.first < this->dot_width / 2) {
                coords.first = this->dot_width / 2;
                }
                if (coords.second < this->dot_width / 2) {
                    coords.second = this->dot_width / 2;
                }
                if (this->size.first - coords.first < this->dot_width / 2) {
                    coords.first = this->size.first - this->dot_width / 2;
                }
                if (this->size.second - coords.second < this->dot_width / 2) {
                    coords.second = this->size.second - this->dot_width / 2;
                }

                // cout << "x: " << coords.first << " y: " << coords.second << "\n";

                // get a reference to the part of the canvas matrix where dot is to be drawn
                cv::Mat roi = this->canvas(
                    cv::Rect(
                        coords.first-(this->dot_width / 2),
                        coords.second-(this->dot_width / 2),
                        this->dot_width,
                        this->dot_width
                    )
                );

                roi.setTo(1);
            }

    };

    REGISTER_CALCULATOR(WhiteboadPalGestureDetectionCalculator);
}
