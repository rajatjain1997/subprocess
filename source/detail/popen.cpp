#include <algorithm>
#include <initializer_list>
#include <stdio.h>
#include <subprocess/detail/exceptions.hpp>
#include <subprocess/detail/popen.hpp>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace subprocess
{

struct popen::PrivateImpl
{
  std::vector<const char*> cmd_;
  file_descriptor stdin_fd{subprocess::in()}, stdout_fd{subprocess::out()}, stderr_fd{subprocess::err()};
  std::optional<int> pid_;

  PrivateImpl() {}

  PrivateImpl(std::initializer_list<const char*> cmd)
  {
    cmd_.reserve(cmd.size() + 1);
    std::copy(cmd.begin(), cmd.end(), std::back_inserter(cmd_));
    cmd_.push_back(NULL);
  }
};

popen::popen(std::initializer_list<const char*> cmd) : pimpl{std::make_unique<PrivateImpl>(std::move(cmd))} {}

popen::popen(popen&& other)
{
  pimpl.reset();
  pimpl.swap(other.pimpl);
};
popen& popen::operator=(popen&& other)
{
  pimpl.swap(other.pimpl);
  return *this;
};

popen::~popen() {}

void popen::execute()
{
  auto dup_and_close = [](file_descriptor& fd, const file_descriptor& dup_to)
  {
    fd.dup(dup_to);
    fd.close();
    fd.close_linked();
  };
  auto close_if_linked = [](file_descriptor& fd)
  {
    if (fd.linked()) fd.close();
  };
  if (int pid{::fork()}; pid < 0)
  {
    throw os_error{"Failed to fork process"};
  }
  else if (pid == 0)
  {
    dup_and_close(pimpl->stdin_fd, {STDIN_FILENO});
    dup_and_close(pimpl->stdout_fd, {STDOUT_FILENO});
    dup_and_close(pimpl->stderr_fd, {STDERR_FILENO});
    exit(::execvp(pimpl->cmd_[0], const_cast<char* const*>(&(pimpl->cmd_[0]))));
  }
  else
  {
    close_if_linked(pimpl->stdin_fd);
    close_if_linked(pimpl->stdout_fd);
    close_if_linked(pimpl->stderr_fd);
    pimpl->pid_ = pid;
  }
}

int popen::wait()
{
  if (not pimpl->pid_)
  {
    throw usage_error{"popen.wait() called before popen.execute()"};
  }
  int waitstatus;
  ::waitpid(*(pimpl->pid_), &waitstatus, 0);
  return waitstatus;
}

file_descriptor& popen::in() { return pimpl->stdin_fd; }

file_descriptor& popen::out() { return pimpl->stdout_fd; }

file_descriptor& popen::err() { return pimpl->stderr_fd; }
} // namespace subprocess
