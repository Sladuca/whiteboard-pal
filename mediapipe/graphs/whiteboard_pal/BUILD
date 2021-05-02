load(
    "//mediapipe/framework/tool:mediapipe_graph.bzl",
    "mediapipe_binary_graph",
    "mediapipe_simple_subgraph",
)

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

exports_files(glob([
    "*.pbtxt",
]))

cc_library(
    name = "desktop_offline_calculators",
    deps = [
        "//mediapipe/calculators/core:flow_limiter_calculator",
        "//mediapipe/calculators/core:gate_calculator",
        "//mediapipe/calculators/core:immediate_mux_calculator",
        "//mediapipe/calculators/core:packet_inner_join_calculator",
        "//mediapipe/calculators/core:previous_loopback_calculator",
        "//mediapipe/calculators/video:opencv_video_decoder_calculator",
        "//mediapipe/calculators/video:opencv_video_encoder_calculator",
    ],
)

cc_library(
    name = "desktop_tflite_calculators",
    deps = [
        ":desktop_offline_calculators",
        "//mediapipe/calculators/core:constant_side_packet_calculator",
        "//mediapipe/calculators/core:merge_calculator",
        "//mediapipe/modules/hand_landmark:hand_landmark_tracking_cpu",
    ],
)

cc_library(
    name = "whiteboard_pal_calculators",
    deps = [
        ":desktop_tflite_calculators",
        "//mediapipe/calculators/core:constant_side_packet_calculator",
        "//mediapipe/calculators/core:merge_calculator",
        "//mediapipe/framework/stream_handler:sync_set_input_stream_handler",
        "//mediapipe/modules/hand_landmark:hand_landmark_tracking_cpu",
        "//mediapipe/graphs/whiteboard_pal/calculators:whiteboard_pal_gesture_calculator",
        "//mediapipe/graphs/whiteboard_pal/calculators:whiteboard_pal_canvas_calculator",
    ],
)

mediapipe_binary_graph(
    name = "whiteboard_pal_binary_graph",
    graph = "whiteboard_pal.pbtxt",
    output_name = "whiteboard_pal_binary_graph.binarypb",
    visibility = ["//visibility:public"],
    deps = [":whiteboard_pal_calculators"],
)