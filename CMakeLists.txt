cmake_minimum_required(VERSION 3.19)

project(subprocess)

set_property(GLOBAL PROPERTY CMAKE_CXX_STANDARD 17)

add_library(subprocess SHARED
    detail/popen.cpp
    detail/util.cpp
    detail/command.cpp
    subprocess.hpp
)

target_compile_features(subprocess PUBLIC cxx_std_17)