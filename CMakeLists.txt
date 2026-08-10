cmake_minimum_required(VERSION 3.10)

project(robot_behavior)

set(CMAKE_CXX_STANDARD 17)

find_package(yaml-cpp REQUIRED)
find_package(behaviortree_cpp REQUIRED)

add_library(robot_behavior SHARED
    src/robot_behavior.cpp
    src/task.cpp
    src/task_cloud.cpp
)

target_include_directories(robot_behavior
    PUBLIC
    include
)

target_link_libraries(robot_behavior
    yaml-cpp
    BT::behaviortree_cpp
)

add_executable(demo
    example/main.cpp
)

target_link_libraries(demo
    robot_behavior
)