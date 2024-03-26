cmake_minimum_required(VERSION 3.1)
project(TNN.Demo)
message(${CMAKE_SOURCE_DIR})
set(TNN_LIB_PATH ${CMAKE_SOURCE_DIR}/third_party/tnn_x86_lib/build_linux_native)
message(${TNN_LIB_PATH})
option(TNN_OPENVINO_ENABLE "with openvino lib" OFF)
option(TNN_OPENVINO_LIB_PATH "openvino lib path" "")
option(TNN_DEMO_WITH_OPENCV "with opencv" OFF)

set(CMAKE_CXX_STANDARD 11)
set(TNN_CPU_ENABLE ON)

if (TNN_DEMO_WITH_OPENCV)
    find_package(OpenCV 4 REQUIRED)
    include_directories(${OpenCV_INCLUDE_DIRS})
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /w")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -std=c++11 -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++11 -pthread -fPIC")
endif()


set(TNNRoot ${CMAKE_SOURCE_DIR}/)
set(TNNInclude ${TNNRoot}/include)

include_directories(${TNNInclude})
include_directories(${CMAKE_SOURCE_DIR}/base)
include_directories(${CMAKE_SOURCE_DIR}/utils)
link_directories(${TNN_LIB_PATH})
if (${TNN_OPENVINO_ENABLE})
    link_directories(${TNN_OPENVINO_LIB_PATH})
    add_definitions(-D_OPENVINO_)
endif()

link_libraries(TNN)

add_subdirectory(${TNNRoot}/third_party/gflags ${TNNRoot}/third_party/gflags)
get_target_property(GFLAGS_INCLUDE_DIRS gflags INTERFACE_INCLUDE_DIRECTORIES)
include_directories(BEFORE "${GFLAGS_INCLUDE_DIRS}")
link_libraries(gflags)

if (TNN_DEMO_WITH_OPENCV)
    add_definitions(-D_OPENCV_)
endif()


add_executable(TNN-demo main.cpp)

if (TNN_DEMO_WITH_OPENCV)
    target_link_libraries(TNN-demo ${OpenCV_LIBS} ${TNN_LIBS})
endif()


