cd tensorflow
bazel build --jobs=6 --verbose_failures -c opt --copt=-mavx --copt=-mfpmath=both --copt=-msse4.2 //tensorflow:libtensorflow_cc.so
1	
bazel build --jobs=6 --verbose_failures -c opt --copt=-mavx --copt=-mfpmath=both --copt=-msse4.2 //tensorflow:libtensorflow_cc.so