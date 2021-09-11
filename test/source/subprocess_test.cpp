#include <string>

#include <subprocess/subprocess.hpp>

using subprocess::command;

int main()
{
  command{"ls"}.run();
  return 0;
}
