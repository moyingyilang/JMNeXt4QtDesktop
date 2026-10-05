cmake_minimum_required(VERSION 3.16)
project(JMNeXt4Desktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(jmnext4desktop
    src/main.cpp
)
target_link_libraries(jmnext4desktop PRIVATE Qt6::Widgets)
