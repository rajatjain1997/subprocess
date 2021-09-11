cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    subprocess
    VERSION 0.1.0
    DESCRIPTION "A C++ high level library for running shell processes"
    HOMEPAGE_URL "https://github.com/rajatjain1997/subprocess"
    LANGUAGES CXX
)

docs_early_return()

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    subprocess_subprocess
    source/subprocess.cpp
    source/detail/command.cpp
    source/detail/file_descriptor.cpp
    source/detail/popen.cpp
)
add_library(subprocess::subprocess ALIAS subprocess_subprocess)

include(GenerateExportHeader)
generate_export_header(
    subprocess_subprocess
    BASE_NAME subprocess
    EXPORT_FILE_NAME export/subprocess/subprocess_export.h
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(subprocess_subprocess PUBLIC SUBPROCESS_STATIC_DEFINE)
endif()

set_target_properties(
    subprocess_subprocess PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME subprocess
    OUTPUT_NAME subprocess
)

target_include_directories(
    subprocess_subprocess ${subprocess_warning_guard}
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
    subprocess_subprocess SYSTEM
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

target_compile_features(subprocess_subprocess PUBLIC cxx_std_17)

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
