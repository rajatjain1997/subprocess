# subprocess

[![cppver](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.wikipedia.org/wiki/C++17)
[![build](https://github.com/rajatjain1997/subprocess/actions/workflows/cmake.yml/badge.svg?branch=master)](https://github.com/rajatjain1997/subprocess/actions)
[![codecov](https://codecov.io/gh/rajatjain1997/subprocess/branch/master/graph/badge.svg?token=BQX8LHUXQ8)](https://codecov.io/gh/rajatjain1997/subprocess)
[![issues](https://img.shields.io/github/issues/rajatjain1997/subprocess)](https://github.com/rajatjain1997/subprocess/issues)
[![GitHub](https://img.shields.io/github/license/rajatjain1997/subprocess)](https://github.com/rajatjain1997/subprocess/blob/master/LICENSE)
[![online](https://img.shields.io/badge/try%20it-online-brightgreen)](https://wandbox.org/permlink/T0iHbd6sSIXzM9vO)
![stars](https://img.shields.io/github/stars/rajatjain1997/subprocess?style=social)

A no nonsense library for writing shell commands in C++.

Writing shell commands in modern C++ is deceptively hard. There are many C++ subprocessing libraries out there, but none of them *just* work. The aim of this library is to allow you to write shell commands in C++ *almost* as if you were writing them in shell.

Full documentation for `subprocess` is available [here](https://subprocess.thecodepad.com).

**Note:** Windows is not currently supported.
## TL;DR

It works exactly how you would expect it to work. Drop [`subprocess.hpp`](https://raw.githubusercontent.com/rajatjain1997/subprocess/master/include/subprocess/subprocess.hpp) in your project, include it, and start working!

```cpp
#include <subprocess/subprocess.hpp>

using namespace subprocess::literals;

int main()
{
    std::string cmd_output;
    ("ls -l"_cmd | "awk 'NR>1{print $3}'"_cmd | "sort"_cmd | "uniq"_cmd > cmd_output).run();
    // Use cmd_output in the program
}
```

## Philosophy

Instead of trying to emulate the subprocess interface from libraries in other languages, this library aims to use (and abuse) C++ operator overloading to achieve a shell-like syntax for writing shell commands.

`subprocess` follows these design goals:

 - **Intuitive Syntax**: Common operations on subprocess commands should feel like shell. This should allow users to compose commands in a familiar manner. There should ideally be no gotchas or differences in the behavior of `subprocess` and unix shell. In case such differences arise, they should be clearly documented.
 - **Trivial Integration**: The whole code comprises of a single [`subprocess.hpp`](https://raw.githubusercontent.com/rajatjain1997/subprocess/master/include/subprocess/subprocess.hpp) and requires no adjustments to compiler flags or project settings. It has no dependencies, subprojects or dependencies on any build system.
 - **Serious Testing**: A CI pipeline performs heavy [integration-testing](https://en.wikipedia.org/wiki/Integration_testing) that covers more than 90% of the code. These tests are run on all the platforms the library supports. Additionally, address and memory sanitizers are run to detect any memory or resource leaks.

## Overview

You can write shell commands using the `subprocess::command` class and use the resulting object to pipe I/O from/to standard streams, files, and variables.

**Examples:**

```cpp
// Running a command
subprocess::command cmd{"touch" + file_path}.run();

// Piping the output of a command to another command
cmd | "uniq"_cmd;
```

### Redirecting stdin

You can use `operator<` to redirect stdin to the command object.

**Examples:**

```cpp
// Reading input from a file
cmd < std::filesystem::path{file_name};
cmd < "file_name";

// Reading input from a variable
std::string input{"abc"};
cmd < input;

// Reading from an already created fd
cmd < subprocess::file_descriptor{fd_no};
```

### Redirecting stdout

The following operators are available for redirecting stdout:

 - `operator>`: Truncates the file and writes output
 - `operator>>`: Appends output to file if it already exists, otherwise creates one.

**Examples:**

```cpp
// Redirecting stdout to stderr
cmd > subprocess::err;

// Redirecting stdout to a file
cmd > std::filesystem::path{file_name};

// or appending to a file
cmd >> std::filesystem::path{file_name};

// Capturing stdout in a variable
std::string var_name;
cmd > var_name;
```

### Redirecting stderr

The following operators are available for redirecting stdout:

 - `operator>=`: Truncates the file and writes stderr
 - `operator>>=`: Appends stderr to file if it already exists, otherwise creates one.

**Examples:**

```cpp
// Redirecting stderr to stdout
cmd >= subprocess::out;

// Redirecting stderr to a file
cmd >= std::filesystem::path{file_name};

// or appending to a file
cmd >>= std::filesystem::path{file_name};

// Capturing stderr in a variable
std::string var_name;
cmd >= var_name;
```

##  CMake FetchContent

This is a header only library. So developer can just copy-paste its content into their project without any configuration.

For those who want to manage their dependencies using CMake FetchContent, this can be used:
```cmake
include(FetchContent)
FetchContent_Declare(subprocess
  GIT_REPOSITORY https://github.com/rajatjain1997/subprocess
  GIT_TAG v0.1.1
  GIT_PROGRESS TRUE
)
target_link_libraries(my_application subprocess::subprocess)
```

## Conclusion

I would love your feedback!
If you find any issues, feel free to [log them](https://github.com/rajatjain1997/subprocess/issues).
