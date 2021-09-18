#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <subprocess/subprocess.hpp>

using subprocess::command;

TEST_CASE("testing simple echo command") { CHECK(command{"echo running correctly"}.run() == 0); }
