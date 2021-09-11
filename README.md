# subprocess

There are so many C++ subprocessing libraries out there. And none of them *just* work. The aim of this library is to allow you to write shell commands in C++ as if you were writing them in shell. 

**Note:** Windows is not currently supported. I would like to pin down the library interface before adding compatibility for the OS.

## TL;DR

It doesn't get much simpler than this:

    #include <subprocess.hpp>

    using subprocess::command;

    int main(int argc, char* argv[])
    {
        std::string owners;
        (command{"ls", "-l", argv[1]} | command{"awk", "NR>1{print $3}"} | command{"sort"} | command{"uniq"} > owners).run();
        // Use the owners as you wish
    }


## Philosophy

Instead of trying to emulate the subprocess interface from libraries in other languages, this library aims to use (and abuse) C++ operator overloading to achieve a shell-like syntax for writing shell commands.

## Overview

You can write shell commands using the `subprocess::command` class and use it to pipe I/O from/to standard streams, files, and variables.

    // Running a command
    subprocess::command cmd{"touch", file_path}.run();
    
    // Piping the output of a command to another command
    cmd | subprocess::command{"uniq"};

    // Redirecting stdout to stderr
    cmd > subprocess::err();

    // Redirecting stdout to a file
    cmd > std::filesystem::path{file_name};

    // or for appendig to the file
    cmd >> std::filesystem::path{file_name};

    // Redirecting stderr

