cmake_minimum_required(VERSION 3.16)
project(JMNeXt4QtDesktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 平台无关的核心（先只放算法，之后才是 Qt 界面）
add_library(jmnext_core STATIC
    src/core/ImageUnscramble.cpp
    src/core/Md5.cpp
    src/core/AesEcb.cpp
    src/core/Base64.cpp
    src/core/JmCrypto.cpp
    src/core/UpdateCheck.cpp
    src/core/UnscrambleApply.cpp
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
add_executable(test_paths tests/test_paths.cpp)
target_link_libraries(test_paths PRIVATE jmnext_core)
add_test(NAME paths COMMAND test_paths)
add_executable(test_crypto tests/test_crypto.cpp)
target_link_libraries(test_crypto PRIVATE jmnext_core)
add_test(NAME crypto COMMAND test_crypto)
add_executable(test_base64_crypto tests/test_base64_crypto.cpp)
target_link_libraries(test_base64_crypto PRIVATE jmnext_core)
add_test(NAME base64_crypto COMMAND test_base64_crypto)
add_executable(test_update_check tests/test_update_check.cpp)
target_link_libraries(test_update_check PRIVATE jmnext_core)
add_test(NAME update_check COMMAND test_update_check)
add_executable(test_unscramble_apply tests/test_unscramble_apply.cpp)
target_link_libraries(test_unscramble_apply PRIVATE jmnext_core)
add_test(NAME unscramble_apply COMMAND test_unscramble_apply)

# 无头 CLI（纯计算链路，不需要 Qt 与显示）
add_executable(jmnext4cli src/cli/main.cpp)
target_link_libraries(jmnext4cli PRIVATE jmnext_core)
