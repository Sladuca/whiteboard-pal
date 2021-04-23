
cmake_minimum_required( VERSION 3.1 )
project( whiteboard_pal )

set(THREADS_PREFER_PTHREAD_FLAG ON)
set( OpenCV_DIR "./opencv")

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)

find_library(TENSORFLOW_LIB tensorflow HINT $ENV{HOME}/libtensorflow2/lib)

# pkg_check_modules(OPENCV REQUIRED IMPORTED_TARGET opencv4)
pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
pkg_check_modules(XEXT REQUIRED IMPORTED_TARGET xext)

# boost
find_package(Boost REQUIRED COMPONENTS fiber)
if(Boost_FOUND)
    message(STATUS "Boost version: ${Boost_VERSION}")
    include_directories(${Boost_INCLUDE_DIRS})
endif()



# include files
include_directories( ./include ./src )
include_directories( ${OpenCV_INCLUDE_DIRS} )

link_directories( ${OpenCV_LIB_DIR} )

# target
add_executable( whiteboard_pal ./src/main.cpp ./src/models.cpp ./src/inputs.cpp )

target_link_libraries( whiteboard_pal PUBLIC PkgConfig::X11 PkgConfig::XEXT ${Boost_LIBRARIES} ${OpenCV_LIBS} "${TENSORFLOW_LIB}" PRIVATE Threads::Threads)
