cmake_minimum_required(VERSION 3.5)
project(mfcc)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread -g")#-g -O0

find_package(OpenCV REQUIRED)

# demo：c++的MFCC梅尔特征提取测试
add_executable(demo_mfcc_t demo_mfcc.cpp)
target_link_libraries(demo_mfcc_t ${OpenCV_LIBS}
        )

