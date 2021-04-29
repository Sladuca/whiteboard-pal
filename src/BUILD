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
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework/formats:image_frame",
        "//mediapipe/framework/formats:image_frame_opencv",
        "//mediapipe/framework/port:file_helpers",
        "//mediapipe/framework/port:opencv_highgui",
        "//mediapipe/framework/port:opencv_imgproc",
        "//mediapipe/framework/port:opencv_video",
        "//mediapipe/framework/port:parse_text_proto",
        "//mediapipe/framework/port:status",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@ncurses//:main",
    ]
)

# cc_library(
#    name = "ncurses",
#    srcs = ["/usr/lib/x86_64-linux-gnu/libncurses.a"],
#    visibility = ["//visibility:public"],
# )

cc_binary(
    name = "whiteboard_pal",
    deps = [
        "//mediapipe/graphs/whiteboard_pal:whiteboard_pal_calculators",
        ":whiteboard_pal_run_graph"
    ]
)