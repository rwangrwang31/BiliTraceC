cmake_minimum_required(VERSION 3.10)
project(BiliTraceC LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Optimization flags
if(MSVC)
    add_compile_options(/O2 /W4)
else()
    add_compile_options(-O3 -Wall -Wextra -pthread)
endif()

# Include directories
include_directories(include)
include_directories(deps/curl-8.11.1_1-win64-mingw/include)

# Source files
file(GLOB SOURCES "src/*.c")

# Executable
add_executable(bilitrace ${SOURCES})

# Libraries
link_directories(deps/curl-8.11.1_1-win64-mingw/lib)
if(WIN32)
    target_link_libraries(bilitrace curl ws2_32)
else()
    target_link_libraries(bilitrace curl)
endif()
