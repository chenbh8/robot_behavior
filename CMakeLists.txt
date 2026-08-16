cmake_minimum_required(VERSION 3.10)

project(robot_behavior)

set(CMAKE_CXX_STANDARD 17)

find_package(yaml-cpp REQUIRED)
find_package(behaviortree_cpp REQUIRED)
find_package(spdlog REQUIRED)

add_library(robot_behavior SHARED
    src/robot_behavior.cpp
    src/task.cpp
    src/task_cloud.cpp
)

target_include_directories(robot_behavior
    PUBLIC
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/concurrentqueue
)

target_link_libraries(robot_behavior
    yaml-cpp
    BT::behaviortree_cpp
    spdlog::spdlog
)

add_executable(demo
    example/main.cpp
)

target_link_libraries(demo
    robot_behavior
    spdlog::spdlog
)