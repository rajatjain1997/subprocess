#include <string>

// #include <gtest/gtest.h>
#include <subprocess/subprocess.hpp>

using subprocess::command;

// TEST(subprocess, test_simple_cmd) { command{"ls"}.run(); }

int main() { command{"ls"}.run(); }
