cmake_minimum_required(VERSION 3.16)
project(JMNeXt4QtDesktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 平台无关的核心（先只放算法，之后才是 Qt 界面）
add_library(jmnext_core STATIC
    src/core/ImageUnscramble.cpp
)
target_include_directories(jmnext_core PUBLIC src)

find_package(Qt6 COMPONENTS Widgets)

# 界面骨架（在没有 Qt 的环境里也能只编核心与测试）
if(Qt6_FOUND)
    add_executable(jmnext4desktop src/main.cpp)
    target_link_libraries(jmnext4desktop PRIVATE Qt6::Widgets jmnext_core)
else()
    message(STATUS "未找到 Qt6：跳过界面目标，只构建核心与测试")
endif()

# 测试（不依赖 Qt，也不依赖任何测试框架 —— 用断言即可，便于在没有包管理的环境里跑）
enable_testing()
add_executable(test_image_unscramble tests/test_image_unscramble.cpp)
target_link_libraries(test_image_unscramble PRIVATE jmnext_core)
add_test(NAME image_unscramble COMMAND test_image_unscramble)
