cmake_policy(SET CMP0048 NEW)
project(scan VERSION 0.0.0.0 LANGUAGES CXX)
cmake_minimum_required(VERSION 3.19)

find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME} ${PROJECT_NAME}.cc)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
target_link_libraries(${PROJECT_NAME} Threads::Threads)

