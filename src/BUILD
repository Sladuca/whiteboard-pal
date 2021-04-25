package(default_visibility = [
    "//visibility:public",
])

cc_library(
    name = "whiteboard_pal_run_graph",
    srcs = [
        "main.cpp",
        "main.hpp",
    ],
    deps = [
        "@boost//:fiber",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework/formats:image_frame",
        "@mediapipe//mediapipe/framework/formats:image_frame_opencv",
        "@mediapipe//mediapipe/framework/port:file_helpers",
        "@mediapipe//mediapipe/framework/port:opencv_highgui",
        "@mediapipe//mediapipe/framework/port:opencv_imgproc",
        "@mediapipe//mediapipe/framework/port:opencv_video",
        "@mediapipe//mediapipe/framework/port:parse_text_proto",
        "@mediapipe//mediapipe/framework/port:status",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
    ]
)

cc_binary(
    name = "whiteboard_pal",
    deps = [
        "@mediapipe//mediapipe/graphs/whiteboard_pal:whiteboard_pal_calculators",
        ":whiteboard_pal_run_graph"
    ]
)