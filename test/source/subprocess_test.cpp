#include <string>

#include <subprocess/subprocess.hpp>
#include <gtest/gtest.h>

using subprocess::command;

TEST(subprocess, test_simple_cmd)
{
  command{"ls"}.run();
}
