cmake_minimum_required(VERSION 3.14)

macro(default name)
  if(NOT DEFINED "${name}")
    set("${name}" "${ARGN}")
  endif()
endmacro()

# Default options
default(FORMAT_COMMAND clang-format)
default(
  PATTERNS
  source/*.cpp source/*.hpp
  include/*.hpp
  test/source/*.cpp test/source/*.hpp
)
default(FIX NO)

set(flag --output-replacements-xml)
set(args OUTPUT_VARIABLE output)
if(FIX)
  set(flag -i)
  set(args "")
endif()

# Expand patterns to absolute paths
set(expanded_patterns "")
foreach(pattern IN LISTS PATTERNS)
  list(APPEND expanded_patterns "${CMAKE_SOURCE_DIR}/${pattern}")
endforeach()

file(GLOB_RECURSE files ${expanded_patterns})

if(files STREQUAL "")
  message(FATAL_ERROR "No files matched in lint.cmake. Check PATTERNS and CMAKE_SOURCE_DIR.")
endif()

set(badly_formatted "")
set(output "")
string(LENGTH "${CMAKE_SOURCE_DIR}/" path_prefix_length)

foreach(file IN LISTS files)
  message(STATUS "Linting file: ${file}")
  execute_process(
    COMMAND "${FORMAT_COMMAND}" --style=file "${flag}" "${file}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE result
    ${args}
  )
  if(NOT result EQUAL "0")
    message(FATAL_ERROR "'${file}': formatter returned with ${result}")
  endif()
  if(NOT FIX AND output MATCHES "\n<replacement offset")
    string(SUBSTRING "${file}" "${path_prefix_length}" -1 relative_file)
    list(APPEND badly_formatted "${relative_file}")
  endif()
  set(output "")
endforeach()

if(NOT badly_formatted STREQUAL "")
  list(JOIN badly_formatted "\n" bad_list)
  message("The following files are badly formatted:\n\n${bad_list}\n")
  message(FATAL_ERROR "Run again with FIX=YES to fix these files.")
endif()