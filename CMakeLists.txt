# CMakefiles.txt for X68Sound

cmake_minimum_required(VERSION 3.20)
project(x68sound LANGUAGES CXX)

# Sources
add_library(x68sound SHARED
    X68Sound/X68Sound.cpp
    X68Sound/x68sound.rc
)

# Compile
#target_compile_definitions(x68sound PRIVATE ROEMO)
target_compile_definitions(x68sound PRIVATE C86CTL)

target_compile_options(x68sound PRIVATE
    /Ox  /Ot  /Oi  /Ob2
    /EHsc
)

set_target_properties(x68sound PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded"
)

# Link
target_link_libraries(x68sound PRIVATE
    winmm
    user32
)

target_link_options(x68sound PRIVATE
    "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/X68Sound/x68sound.def"
)
