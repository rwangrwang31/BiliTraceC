# ============================================================
# BiliTraceC CMakeLists.txt
# 跨平台CMake编译配置
# ============================================================

cmake_minimum_required(VERSION 3.10)
project(BiliTraceC VERSION 2.0 LANGUAGES C)

# C标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
if(MSVC)
    # Visual Studio编译器
    add_compile_options(/O2 /W4)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    # GCC/Clang
    add_compile_options(-O3 -Wall -Wextra)
endif()

# 查找依赖
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# 源文件
set(SOURCES
    main.c
    cracker.c
    network.c
)

set(HEADERS
    crc32_core.h
    utils.h
    cracker.h
    network.h
)

# 可执行文件
add_executable(bilitrace ${SOURCES} ${HEADERS})

# 链接库
target_link_libraries(bilitrace 
    PRIVATE 
    CURL::libcurl
    Threads::Threads
)

# Windows特殊处理
if(WIN32)
    target_link_libraries(bilitrace PRIVATE ws2_32)
endif()

# 安装规则
install(TARGETS bilitrace RUNTIME DESTINATION bin)

# 打印配置信息
message(STATUS "==================================")
message(STATUS "BiliTraceC v${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "CURL: ${CURL_VERSION_STRING}")
message(STATUS "==================================")
