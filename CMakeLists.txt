cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    subprocess
    VERSION 0.1.0
    DESCRIPTION "A C++ high level library for running shell processes"
    HOMEPAGE_URL "https://github.com/rajatjain1997/subprocess"
    LANGUAGES CXX
)

execute_process(
    COMMAND conan install ${CMAKE_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

docs_early_return()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)

conan_basic_setup(TARGETS)


include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    subprocess_subprocess INTERFACE)
add_library(subprocess::subprocess ALIAS subprocess_subprocess)

target_include_directories(
    subprocess_subprocess ${subprocess_warning_guard}
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(subprocess_subprocess INTERFACE cxx_std_17)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT subprocess_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of subprocess"
  )
endif()

include(cmake/dev-mode.cmake)
