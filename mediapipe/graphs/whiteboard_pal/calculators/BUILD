package(default_visibility = ["//visibility:public"])

cc_library(
    name = "whiteboard_pal_canvas_calculator",
    srcs = ["canvas.cc"],
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/framework/formats:image_frame_opencv",
        "//mediapipe/framework/formats:image_frame",
        "//mediapipe/framework/formats:image",
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework:calculator_options_cc_proto",
        "//mediapipe/framework/port:ret_check",
        "//mediapipe/framework/port:status",
        "//mediapipe/framework/port:opencv_core",
        "//mediapipe/framework/port:opencv_imgproc",
    ],
    alwayslink = 1,
)

cc_library(
    name = "whiteboard_pal_gesture_calculator",
    srcs = ["gesture.cc"],
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework/formats:landmark_cc_proto",
        "//mediapipe/framework:calculator_options_cc_proto",
        "//mediapipe/framework/port:ret_check",
        "//mediapipe/framework/port:status",
        "//mediapipe/framework/port:opencv_core",
        "//mediapipe/framework/port:opencv_imgproc",
    ],
    alwayslink = 1,
)