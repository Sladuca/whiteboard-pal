## Setup

### Prerequisites

install bazelisk. Easiest way is to have node installed (do yourself a favor and use [`nvm`](https://github.com/nvm-sh/nvm)) and do `npm i -g @bazel/bazelisk`.
install boost using `sudo apt install libboost-all-dev`.

No CUDA or Opencv necessary, as this runs entirely on CPU and is already configured to build a local version of OpenCV. No worries if you already have either installed, it shouldn't interfere with everything.

Also need to install ncurses by doing `sudo apt-get install libncurses5-dev libncursesw5-dev`.

### Repo setup (should only need to do this once)

setup opencv: `./setup_opencv`

`python3 -m venv env`
`source ./env/bin/activate` (run this every time you start a new shell, otherwise you'll get a weird `python: command not found` error when trying to build)


### Init loopback device (do every time you reboot)

run `./init_device`.


## Build

`bazel build -c opt --define MEDIAPIPE_DISABLE_GPU=1 src:whiteboard_pal`

## Run

`GLOG_logtostderr=1 ./bazel-bin/src/whiteboard_pal --calculator_graph_config_file=mediapipe/graphs/whiteboard_pal/whiteboard_pal.pbtxt`

Currently, there are three different modes for Whiteboard Pal: free drawing mode, line mode, and eraser mode. To toggle between the different modes, simply press "m" on the keyboard when not making the drawing gesture.

To "deposit ink" when drawing, you will need to form the gesture of making an L shape with your fingers (think of "L on your forehead" from All Star by Smash Mouth). Otherwise, to "lift the pen" and not draw, simply close your thumb inwards and only have your index finger pointing. 

Note: To use Whiteboard Pal on Zoom, you will have to run the application BEFORE opening up Zoom on Linux, otherwise Zoom will not recognize Whiteboard Pal as an input device.
