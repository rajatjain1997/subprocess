#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <string>

#include <subprocess/subprocess.hpp>

using subprocess::command;

TEST_CASE("simple echo command") { CHECK(command{"echo running correctly"}.run() == 0); }

TEST_CASE("variable output redirection")
{
  std::string output;
  (command{"echo abc"} > output).run();
  CHECK_EQ(output, "abc\n");
}

TEST_CASE("variable input redirection")
{
  std::string input{"1\n2\n3\n4\n5"}, output;
  (command{"head -n2"}<input> output).run();
  CHECK_EQ(output, "1\n2\n");
}

TEST_CASE("piping")
{
  int errc = (command{"ps aux"} | command{"awk '{print $2}'"} | command{"sort"} | command{"uniq"} |
              command{"head -n1"} > "/dev/null")
                 .run();
  CHECK_EQ(errc, 0);
}

TEST_CASE("os_error throw on bad command")
{
  REQUIRE_THROWS_AS(command{"random-unavailable-cmd"}.run(), subprocess::exceptions::os_error);
}

TEST_CASE("command_error throw on bad exit status")
{
  REQUIRE_THROWS_AS(command{"false"}.run(), subprocess::exceptions::command_error);
}

TEST_CASE("nothrow variant doesn't throw on bad exit status")
{
  REQUIRE_NOTHROW(command{"false"}.run(std::nothrow));
  CHECK_NE(command{"false"}.run(std::nothrow), 0);
}

TEST_CASE("_cmd literal constructs subprocess::command")
{
  using namespace subprocess::literals;
  CHECK_EQ(("true"_cmd).run(), 0);
}