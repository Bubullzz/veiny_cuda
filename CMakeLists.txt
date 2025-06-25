cmake_minimum_required(VERSION 3.10)
project(MyITKVTKProject)

find_package(ITK REQUIRED)
find_package(VTK REQUIRED)

include(${ITK_USE_FILE})
include(${VTK_USE_FILE})

add_executable(MyApp main.cpp)

target_link_libraries(MyApp ${ITK_LIBRARIES} ${VTK_LIBRARIES})
