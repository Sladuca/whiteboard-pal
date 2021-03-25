
cmake_minimum_required( VERSION 3.0 )

project( whiteboard_pal )

find_package(PkgConfig REQUIRED) 

# opencv

pkg_check_modules(OPENCV REQUIRED IMPORTED_TARGET opencv4)

# boost
find_package(Boost REQUIRED COMPONENTS fiber)
if(Boost_FOUND)
    message(STATUS "Boost version: ${Boost_VERSION}")
    include_directories(${Boost_INCLUDE_DIRS})
endif()



# include files
include_directories( ./include ./src )

# target
add_executable( whiteboard_pal ./src/main.cpp ./src/models.cpp )

target_link_libraries( whiteboard_pal PUBLIC PkgConfig::OPENCV ${Boost_LIBRARIES})