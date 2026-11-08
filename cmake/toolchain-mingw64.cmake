# Windows x86_64 交叉编译工具链（MinGW-w64）
#
# 用途：在 Linux/arm64 上用 mingw 编译出 Windows 版可执行文件。
# Qt 用的是官方 6.7.3 mingw_64 构建（Qt 6.8 起官方不再提供 MinGW 包），
# 宿主机工具（moc/rcc/qmlcachegen）来自同版本的 linux_gcc_arm64 包，
# 通过 -DQT_HOST_PATH=... 传入 —— 二者版本必须一致。
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /root/qt-win/6.7.3/mingw_64)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
