#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <string>

#include <subprocess/subprocess.hpp>

using subprocess::command;

TEST_CASE("testing simple echo command") { CHECK(command{"echo running correctly"}.run() == 0); }

TEST_CASE("testing output redirection")
{
  std::string output;
  (command{"echo abc"} > output).run();
  CHECK(output == "abc\n");
}

TEST_CASE("testing input redirection")
{
  std::string input{"1\n2\n3\n4\n5"}, output;
  (command{"head -n2"}<input> output).run();
  CHECK(output == "1\n2\n");
}

TEST_CASE("testing piping")
{
  std::string output;
  int errc = (command{"ps aux"} | command{"awk '{print $2}'"} | command{"sort"} | command{"uniq"} |
              command{"head -n1"} > output)
                 .run();
  CHECK(errc == 0);
}
