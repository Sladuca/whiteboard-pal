## Setup

### Prerequisites

install bazelisk. Easiest way is to have node installed (do yourself a favor and use [`nvm`](https://github.com/nvm-sh/nvm)) and do `npm i -g @bazel/bazelisk`.
install boost using `sudo apt install libboost-all-dev`.

No CUDA or Opencv necessary, as this runs entirely on CPU and is already configured to build a local version of OpenCV. No worries if you already have either installed, it shouldn't interfere with everything.

### Repo setup (should only need to do this once)

setup opencv: `./setup_opencv`

`python3 -m venv env`
`source ./env/bin/activate` (run this every time you start a new shell, otherwise you'll get a weird `python: command not found` error when trying to build)


### Init loopback device (do every time you reboot)

run `./init_device`. 


## Build

`bazel build -c opt --define MEDIAPIPE_DISABLE_GPU=1 src:whiteboard_pal`

## Run
`GLOG_logtostderr=1 bazel-bin/src/whiteboad_pal \
  --calculator_graph_config_file=mediapipe/graphs/whiteboad_pal/whiteboard_pal.pbtxt`