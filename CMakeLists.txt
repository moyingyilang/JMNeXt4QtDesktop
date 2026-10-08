cmake_minimum_required(VERSION 3.16)

# 可选：检测到 ccache 就自动作为编译器启动器，加速重复的全量重建。
# 实测（容器内，Ninja，arm64）：无 ccache 全量重建 72.5 秒；有 ccache 首次 99.4 秒（填充缓存），
# 之后每次清空重建 13.2 秒（约 5.5 倍）。只在首次多付约 27 秒，后续每次都赚。
# 如需关闭：配置时显式传 -DCMAKE_CXX_COMPILER_LAUNCHER=（空值即可覆盖）。
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM AND NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "检测到 ccache，已启用（重复全量重建约 5 倍快）：${CCACHE_PROGRAM}")
endif()
project(JMNeXt4QtDesktop VERSION 0.1.1 LANGUAGES CXX)

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
    src/core/JmSession.cpp
    src/core/HostDiscovery.cpp
    src/core/BlockRules.cpp
    src/core/JmApi.cpp
    src/core/JmParse.cpp
    src/core/JmUrls.cpp
    src/core/ReadProgress.cpp
)
target_include_directories(jmnext_core PUBLIC src)

find_package(Qt6 COMPONENTS Widgets)

# 界面骨架（在没有 Qt 的环境里也能只编核心与测试）
if(Qt6_FOUND)
# 共用 Qt 层（网络客户端 / 缓存 / 解码 / 后台 worker）——widget 与 QML 两个前端都用它。
# 刻意不含 MainWindow.cpp：那是 widget 专属界面；QML 侧由 JmBackend 复用同一个 JmWorker。
add_library(jmnext_qtlayer STATIC
    src/qt/ImageCodec.cpp src/qt/JmClient.cpp src/qt/QtHttpClient.cpp
    src/qt/ImageCache.cpp src/qt/JmWorker.cpp)
target_include_directories(jmnext_qtlayer PUBLIC src)
target_link_libraries(jmnext_qtlayer PUBLIC jmnext_core Qt6::Gui Qt6::Network Qt6::Core)

add_executable(jmnext4desktop src/main.cpp src/qt/MainWindow.cpp)
    target_link_libraries(jmnext4desktop PRIVATE jmnext_qtlayer Qt6::Widgets Qt6::Gui Qt6::Network)
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
add_executable(test_session tests/test_session.cpp)
target_link_libraries(test_session PRIVATE jmnext_core)
add_test(NAME session COMMAND test_session)
add_executable(test_host_discovery tests/test_host_discovery.cpp)
target_link_libraries(test_host_discovery PRIVATE jmnext_core)
add_test(NAME host_discovery COMMAND test_host_discovery)
add_executable(test_block_rules tests/test_block_rules.cpp)
target_link_libraries(test_block_rules PRIVATE jmnext_core)
add_test(NAME block_rules COMMAND test_block_rules)
add_executable(test_jm_api tests/test_jm_api.cpp)
target_link_libraries(test_jm_api PRIVATE jmnext_core)
add_test(NAME jm_api COMMAND test_jm_api)
add_executable(test_offline_flow tests/test_offline_flow.cpp)
target_link_libraries(test_offline_flow PRIVATE jmnext_core)
add_test(NAME offline_flow COMMAND test_offline_flow)
add_executable(test_jm_parse tests/test_jm_parse.cpp)
target_link_libraries(test_jm_parse PRIVATE jmnext_core)
add_test(NAME jm_parse COMMAND test_jm_parse)
add_executable(test_jm_urls tests/test_jm_urls.cpp)
target_link_libraries(test_jm_urls PRIVATE jmnext_core)
add_test(NAME jm_urls COMMAND test_jm_urls)
add_executable(test_read_progress tests/test_read_progress.cpp)
target_link_libraries(test_read_progress PRIVATE jmnext_core)
add_test(NAME read_progress COMMAND test_read_progress)
add_executable(test_envelope tests/test_envelope.cpp)
target_link_libraries(test_envelope PRIVATE jmnext_core)
add_test(NAME envelope COMMAND test_envelope)
add_executable(test_search_parse tests/test_search_parse.cpp)
target_link_libraries(test_search_parse PRIVATE jmnext_core)
add_test(NAME search_parse COMMAND test_search_parse)

# 无头 CLI（纯计算链路，不需要 Qt 与显示）
add_executable(jmnext4cli src/cli/main.cpp)
target_link_libraries(jmnext4cli PRIVATE jmnext_core)

# ---- Qt 层：真机网络与图片读写（核心层仍然不依赖 Qt）----
if(Qt6_FOUND)
    find_package(Qt6 REQUIRED COMPONENTS Network Gui)
    add_executable(jmnext4img src/qt/main_img.cpp src/qt/ImageCodec.cpp)
    target_link_libraries(jmnext4img PRIVATE jmnext_core Qt6::Gui)
    # QtHttpClient 是给上层用的实现（没有 main），做成静态库
    add_library(jmnext_qtnet STATIC src/qt/QtHttpClient.cpp)
    target_link_libraries(jmnext_qtnet PUBLIC jmnext_core Qt6::Network)
    # 真实网络自检工具（给真机用）
    add_executable(jmnext4net src/qt/main_net.cpp)
    target_link_libraries(jmnext4net PRIVATE jmnext_qtnet Qt6::Core)
endif()

# ---------------------------------------------------------------------------
# QML 前端（P0 骨架）。与 widget 版共用 core/（Qt 无关）与 qt/Theme.h、qt/JmClient。
# 注意：容器里是 Qt 6.4.2，MultiEffect（6.5+）不存在；模糊/颗粒将来走 Qt5Compat 或自写 ShaderEffect。
# ---------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Quick Qml QuickControls2)
qt_add_executable(jmnext4qml
    src/qml/JmImageProvider.cpp
    src/qml/JmBackend.cpp
    src/qml/main_qml.cpp
    src/qt/Theme.h
    src/qt/ThemeTokens.h
)
target_include_directories(jmnext4qml PRIVATE src)
target_link_libraries(jmnext4qml PRIVATE jmnext_qtlayer Qt6::Quick Qt6::Qml Qt6::Gui Qt6::QuickControls2)
qt_add_qml_module(jmnext4qml
    URI JMNeXt
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
        src/qml/ComicCard.qml
        src/qml/StateBox.qml
        src/qml/LoadMoreFooter.qml
        src/qml/HomeScreen.qml
        src/qml/SearchScreen.qml
        src/qml/ReaderScreen.qml
        src/qml/DetailScreen.qml
        src/qml/AboutScreen.qml
        src/qml/SettingsScreen.qml
)
