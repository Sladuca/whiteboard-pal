#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/calculator_options.pb.h"
#include "mediapipe/framework/port/opencv_core_inc.h"
#include "mediapipe/framework/port/opencv_imgproc_inc.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/formats/image.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"

namespace mediapipe {

    constexpr char DRAW_COORDS_TAG[] = "DRAW_COORDS";
    constexpr char HAS_GESTURE_TAG[] = "HAS_GESTURE";
    constexpr char FRAME_HEIGHT_TAG[] = "FRAME_HEIGHT";
    constexpr char FRAME_WIDTH_TAG[] = "FRAME_WIDTH";
    constexpr char SUBSTRATE_TAG[] = "SUBSTRATE";
    constexpr char DRAWN_FRAME_TAG[] = "DRAWN_FRAME";

    class WhiteboardPalCanvasCalculator: public CalculatorBase {
        cv::Mat canvas;
        cv::Scalar color;
        std::pair<int, int> size;
        int dot_width;

        public:
            static absl::Status GetContract(CalculatorContract* cc) {
                LOG(INFO) << "canvas calculator get contract\n";
                cc->Inputs().Tag(DRAW_COORDS_TAG).Set<std::pair<int, int>>();
                cc->Inputs().Tag(HAS_GESTURE_TAG).Set<bool>();
                cc->Inputs().Tag(SUBSTRATE_TAG).Set<ImageFrame>();
                cc->InputSidePackets().Tag(FRAME_WIDTH_TAG).Set<int>();
                cc->InputSidePackets().Tag(FRAME_HEIGHT_TAG).Set<int>();
                cc->Outputs().Tag(DRAWN_FRAME_TAG).Set<cv::Mat>();
                return absl::OkStatus();
            }

            // TODO: get rid of this and actually set the timestamps properly
            absl::Status Open(CalculatorContext* cc) {
                cc->SetOffset(TimestampDiff(0));
                int width = cc->InputSidePackets().Tag(FRAME_WIDTH_TAG).Get<int>();
                int height = cc->InputSidePackets().Tag(FRAME_HEIGHT_TAG).Get<int>();

                LOG(INFO) << "frame height: " << height;
                LOG(INFO) << "frame width: " << width;

                this->size = std::make_pair(height, width);

                // TODO: add a packet for this so you can change the color and/or pen width at any time
                this->color = cv::Scalar(255, 0, 0);
                this->dot_width = 5;
                this->canvas = cv::Mat::zeros(size.first, size.second, CV_8U);

                LOG(INFO) << "returning from canvas.Open";
                return absl::OkStatus();
            }

            // ! expects the substrate mat to be in RGB format
            absl::Status Process(CalculatorContext* cc) {
                // throw an error if any inputs not present

                //RET_CHECK(!cc->Inputs().Tag(DRAW_COORDS_TAG).IsEmpty());
                //RET_CHECK(!cc->Inputs().Tag(HAS_GESTURE_TAG).IsEmpty());
                RET_CHECK(!cc->Inputs().Tag(SUBSTRATE_TAG).IsEmpty());

                auto& substrate_packet = cc->Inputs().Tag(SUBSTRATE_TAG).Get<ImageFrame>();

                // if no coords packet or gesture packet, just forward the substrate
                if (cc->Inputs().Tag(DRAW_COORDS_TAG).IsEmpty() || cc->Inputs().Tag(HAS_GESTURE_TAG).IsEmpty()) {
                    LOG(INFO) << "either draw coords or gesture packet is empty. Simply forwarding substrate";
                } else if (!cc->Inputs().Tag(HAS_GESTURE_TAG).Get<bool>()) {
                    // only update the canvas if gesture is detected
                    std::pair<float, float> coords = cc->Inputs().Tag(DRAW_COORDS_TAG).Get<std::pair<float, float>>();
                    std::pair<int, int> coords_int = std::make_pair((int)(coords.first * (float)this->size.second),
                        (int)(coords.second * (float)this->size.first));
                    this->update_canvas(coords);
                }

                // apply canvas to the image and send
                auto output_frame = absl::make_unique<cv::Mat>(formats::MatView(&substrate_packet));
                output_frame.get()->setTo(this->color, this->canvas);

                cc->Outputs().Tag(DRAWN_FRAME_TAG).Add(output_frame.release(), cc->InputTimestamp());
                return absl::OkStatus();
            }

        private:
            void update_canvas(std::pair<int, int> coords) {
                LOG(INFO) << "canvas calculator update\n";

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
    REGISTER_CALCULATOR(WhiteboardPalCanvasCalculator);
}
