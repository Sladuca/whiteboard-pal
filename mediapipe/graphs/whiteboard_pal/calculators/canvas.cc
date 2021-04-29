#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/calculator_options.pb.h"
#include "mediapipe/framework/port/opencv_core_inc.h"
#include "mediapipe/framework/port/opencv_imgproc_inc.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/formats/image.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"

void printt_mat_type(int type) {
  std::string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  LOG(INFO) << "CAP_PROP_FORMAT: " << r << "\n";
}

typedef enum DrawMode {
    DEFAULT,
    LINE,
    ERASER,
} DrawMode;

namespace mediapipe {

    constexpr char DRAW_COORDS_TAG[] = "DRAW_COORDS";
    constexpr char HAS_GESTURE_TAG[] = "HAS_GESTURE";
    constexpr char FRAME_HEIGHT_TAG[] = "FRAME_HEIGHT";
    constexpr char FRAME_WIDTH_TAG[] = "FRAME_WIDTH";
    constexpr char SUBSTRATE_TAG[] = "SUBSTRATE";
    constexpr char DRAWN_FRAME_TAG[] = "DRAWN_FRAME";
    constexpr char KEY_TAG[] = "KEY";

    class WhiteboardPalCanvasCalculator: public CalculatorBase {
        DrawMode mode;
        cv::Mat canvas;
        cv::Scalar color;
        std::pair<int, int> size;
        std::optional<std::pair<int, int>> previous_point;
        int dot_width;
        bool has_gesture;
        bool line_in_progress;

        public:
            static absl::Status GetContract(CalculatorContract* cc) {
                LOG(INFO) << "canvas calculator get contract\n";
                cc->Inputs().Tag(DRAW_COORDS_TAG).Set<std::pair<float, float>>();
                cc->Inputs().Tag(HAS_GESTURE_TAG).Set<bool>();
                cc->Inputs().Tag(SUBSTRATE_TAG).Set<ImageFrame>();
                cc->Inputs().Tag(KEY_TAG).Set<int>();
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

                this->size = std::make_pair(width, height);
                this->previous_point = {};
                this->mode = DrawMode::DEFAULT;
                this->line_in_progress = false;

                // TODO: add a packet for this so you can change the color and/or pen width at any time
                this->color = cv::Scalar(255, 0, 0);
                this->dot_width = 3;
                this->canvas = cv::Mat::zeros(size.second, size.first, CV_8U);
                this->has_gesture = false;

                LOG(INFO) << "returning from canvas.Open";
                return absl::OkStatus();
            }

            // ! expects the substrate mat to be in RGB format
            absl::Status Process(CalculatorContext* cc) {

                if (!cc->Inputs().Tag(KEY_TAG).IsEmpty()) {
                    int c = cc->Inputs().Tag(KEY_TAG).Get<int>();
                    this->handle_key(c);

                    if (cc->Inputs().Tag(SUBSTRATE_TAG).IsEmpty()) {
                        return absl::OkStatus();
                    }
                }

                RET_CHECK(!cc->Inputs().Tag(SUBSTRATE_TAG).IsEmpty());
                
                auto& substrate_packet = cc->Inputs().Tag(SUBSTRATE_TAG).Get<ImageFrame>();

                bool finish_line = false;


                if (!cc->Inputs().Tag(HAS_GESTURE_TAG).IsEmpty()) {
                    bool gesture = cc->Inputs().Tag(HAS_GESTURE_TAG).Get<bool>();
                    if (this->mode == DrawMode::LINE && this->has_gesture && !gesture) {
                       this->line_in_progress = false;
                       finish_line = true;
                    } else if (this->mode == DrawMode::LINE && !this->has_gesture && gesture) {
                        this->line_in_progress = true;
                    }
                    this->has_gesture = gesture;
                }


                std::optional<std::pair<int, int>> line_preview_endpoint;

                // only update the canvas if gesture is detected
                if (!cc->Inputs().Tag(DRAW_COORDS_TAG).IsEmpty()) {
                    if (this->has_gesture) {
                        std::pair<float, float> coords = cc->Inputs().Tag(DRAW_COORDS_TAG).Get<std::pair<float, float>>();
                        std::pair<int, int> coords_int = std::make_pair((int)(coords.first * (float)this->size.first),
                            (int)(coords.second * (float)this->size.second));

                        if (this->mode == DrawMode::LINE && !finish_line) {
                            line_preview_endpoint = coords_int;
                        } else {
                            // LOG(INFO) << "pair int: " << coords_int.first << coords_int.second;
                            this->update_canvas(coords_int);
                            this->previous_point = coords_int;
                        }
                    } else {
                        this->previous_point = {};
                    }
                }

                // LOG(INFO) << "4";

                // apply canvas to the image and send
                auto output_frame = absl::make_unique<cv::Mat>(formats::MatView(&substrate_packet));
                output_frame->setTo(this->color, this->canvas);

                // show straight-line preview
                if (this->mode == DrawMode::LINE && !finish_line && line_preview_endpoint.has_value() && previous_point.has_value()) {
                    cv::Point p1 = cv::Point(this->previous_point.value().first, this->previous_point.value().second);
                    cv::Point p2 = cv::Point(line_preview_endpoint.value().first, line_preview_endpoint.value().second);
                    cv::line(*output_frame, p1, p2, this->color, this->dot_width);
                }
                
                // LOG(INFO) << "5";
                
                cc->Outputs().Tag(DRAWN_FRAME_TAG).Add(output_frame.release(), cc->InputTimestamp());
                return absl::OkStatus();
            }

        private:

            void toggle_mode() {
                switch (this->mode) {
                    case DrawMode::DEFAULT:
                        this->mode = DrawMode::LINE;
                        break;
                    case DrawMode::LINE:
                        this->mode = DrawMode::ERASER;
                        this->line_in_progress = false;
                        break;
                    case DrawMode::ERASER:
                        this->mode = DrawMode::DEFAULT;
                        break;
                    default:
                        LOG(ERROR) << "canvas draw mode in an invalid state!";
                }
                LOG(INFO) << "current draw mode: " << this->mode;
            }

            void handle_key(int c) {
                switch (c) {
                    case 'm':
                        this->toggle_mode();
                        break;
                    default:
                        LOG(INFO) << "ignoring unrecognised key '"  << (char)c << "'";
                        break;
                }
            }

            void update_canvas(std::pair<int, int> coords) {

                if (!this->previous_point.has_value()) {
                    this->previous_point = coords;
                    return;
                }

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

                auto previous_point = this->previous_point.value();

                cv::line(this->canvas, cv::Point(previous_point.first, previous_point.second), cv::Point(coords.first, coords.second), cv::Scalar(1), this->dot_width);
            }
    };
    REGISTER_CALCULATOR(WhiteboardPalCanvasCalculator);
}
