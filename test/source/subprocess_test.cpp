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
  (command{"head -n2"} > output < input).run();
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
  REQUIRE_THROWS_WITH(command{"false"}.run(), "command exitstatus 1 : subprocess_error");
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

TEST_CASE("_cmd literal + string chaining")
{
  using namespace subprocess::literals;
  int errc = ("ps aux"_cmd | "awk '{print $2}'" | "sort" | "uniq" | "head -n1"_cmd > "/dev/null").run();
  CHECK_EQ(errc, 0);
}

TEST_CASE("bash-like redirection - stderr to stdout")
{
  using namespace subprocess::literals;
  std::string output;
  int errc = ("awk -l"_cmd > output >= subprocess::out).run(std::nothrow);
  REQUIRE_NE(errc, 0);
  REQUIRE(output != "\n");
  REQUIRE(not output.empty());
}

TEST_CASE("bash-like redirection - stdout to stderr")
{
  using namespace subprocess::literals;
  std::string output;
  int errc = ("echo abc"_cmd >= output > subprocess::err).run(std::nothrow);
  REQUIRE_EQ(errc, 0);
  REQUIRE(output == "abc\n");
}

TEST_CASE("expanding subcommands")
{
  std::string output;
  (subprocess::command{"echo $(yes | head -1)"} > output).run();
  REQUIRE_EQ(output, "y\n");
}

TEST_CASE("SIGPIPE handling for child processes")
{
  using namespace subprocess::literals;
  std::string output;
  int errc = ("yes"_cmd | "head -n1"_cmd > output).run();
  REQUIRE_EQ(errc, 0);
  REQUIRE_EQ(output, "y\n");
}

TEST_CASE("pipe_descriptors double linking not allowed")
{
  auto [read_desc, write_desc] = subprocess::create_pipe();
  REQUIRE_THROWS_AS(subprocess::link(*read_desc, *write_desc), subprocess::exceptions::usage_error);
}