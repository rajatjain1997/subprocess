# subprocess

![cppver](https://img.shields.io/badge/C%2B%2B-17-blue)
![build](https://github.com/rajatjain1997/subprocess/actions/workflows/cmake.yml/badge.svg?branch=master)
[![codecov](https://codecov.io/gh/rajatjain1997/subprocess/branch/master/graph/badge.svg?token=BQX8LHUXQ8)](https://codecov.io/gh/rajatjain1997/subprocess)
[![issues](https://img.shields.io/github/issues/rajatjain1997/subprocess)](https://github.com/rajatjain1997/subprocess/issues)
![GitHub](https://img.shields.io/github/license/rajatjain1997/subprocess)
[![online](https://img.shields.io/badge/try%20it-online-brightgreen)](https://wandbox.org/permlink/T0iHbd6sSIXzM9vO)
![stars](https://img.shields.io/github/stars/rajatjain1997/subprocess?style=social)

There are many C++ subprocessing libraries out there, but none of them *just* work. The aim of this library is to allow you to write shell commands in C++ *almost* as if you were writing them in shell.

Full documentation for `subprocess` is available [here](https://subprocess.thecodepad.com).

**Note:** Windows is not currently supported. I would like to pin down the library interface before adding compatibility for the OS.
## TL;DR

It works exactly how you would expect it to work.

```cpp
#include <subprocess.hpp>

using namespace subprocess::literals;

int main()
{
    std::string owners;
    ("ls -l"_cmd | "awk 'NR>1{print $3}'"_cmd | "sort"_cmd | "uniq"_cmd > owners).run();
    // Use the owners as you wish
}
```

## Philosophy

Instead of trying to emulate the subprocess interface from libraries in other languages, this library aims to use (and abuse) C++ operator overloading to achieve a shell-like syntax for writing shell commands.

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

## Conclusion

I would love your feedback!
If you find any issues, feel free to [log them](https://github.com/rajatjain1997/subprocess/issues).
