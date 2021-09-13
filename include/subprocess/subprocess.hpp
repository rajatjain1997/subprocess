#pragma once

#include <algorithm>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

extern "C"
{
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>
}

namespace subprocess
{

namespace exceptions
{
/**
 * @brief A catch-all class for all errors thrown by subprocess.
 *
 * All exceptions in the library derive from subprocess_error.
 * An exception of type subprocess_error is never actually thrown.
 *
 */
class subprocess_error : public std::runtime_error
{
public:
  explicit subprocess_error(const std::string& str) : runtime_error(str) {}
  explicit subprocess_error(const char* str) : runtime_error(str) {}
};

/**
 * @brief Thrown when there is an error at the operating system level
 *
 * Thrown whenever an error code is returned from a syscall and
 * subprocess is unable to proceed with the function called by
 * the user.
 *
 */
class os_error : public subprocess_error
{
public:
  explicit os_error(const std::string& str) : subprocess_error(str) {}
  explicit os_error(const char* str) : subprocess_error(str) {}
};

/**
 * @brief Thrown when there is an error in the usage of the library's public interface
 *
 * These errors should be infrequent on the user end.
 * They get thrown whenever an erroneous call is made to a function within the
 * library.
 *
 * For example, usage_error is thrown when you try to link an fd that is already linked
 * to another fd.
 *
 */
class usage_error : public subprocess_error
{
public:
  explicit usage_error(const std::string& str) : subprocess_error(str) {}
  explicit usage_error(const char* str) : subprocess_error(str) {}
};

/**
 * @brief Thrown when a command exits with a non-zero exit code.
 *
 * This exception is thrown whenever a subprocess constructed from subprocess::command
 * exits with an error. The return_code() member function can be called to get the return code.
 */

class command_error : public subprocess_error
{
  int return_code_;

public:
  explicit command_error(const std::string& str, int ret_code) : subprocess_error(str), return_code_{ret_code}
  {
  }
  explicit command_error(const char* str, int ret_code) : subprocess_error(str), return_code_{ret_code} {}

  /**
   * @brief Returns the exit code of the subprocess
   *
   * @return int exit code
   */
  int return_code() { return return_code_; }
};
} // namespace exceptions

class shell_expander
{
public:
  shell_expander(const std::string& s);
  ~shell_expander();

  decltype(std::declval<wordexp_t>().we_wordv) argv() & { return parsed_args.we_wordv; }

private:
  ::wordexp_t parsed_args;
};

shell_expander::shell_expander(const std::string& s) { ::wordexp(s.c_str(), &parsed_args, 0); }
shell_expander::~shell_expander() { ::wordfree(&parsed_args); }

/**
 * @brief Abstracts file descriptors
 *
 * Member functions of this class wrap syscalls to commonly
 * used file descriptor functions.
 *
 * The implementation of this class will be changed in the future with #4.
 *
 * @see https://github.com/rajatjain1997/subprocess/issues/4
 *
 */
class descriptor
{
public:
  /**
   * @brief Opens a file and returns a descriptor object
   *
   * @param file_name Name of the file to open
   * @param flags Flags with which to open the file
   * @return descriptor FD representing the opened file
   */
  static descriptor open(std::filesystem::path file_name, int flags);

  /**
   * @brief Create an OS level pipe and return read and write fds.
   *
   * @return std::pair<descriptor, descriptor> A pair of linked file_descriptos [read_fd, write_fd]
   */
  static std::pair<descriptor, descriptor> create_pipe();

  descriptor(int fd = -1) : fd_{fd} {}
  descriptor(const descriptor& other) = delete;
  descriptor& operator=(const descriptor& other) = delete;
  descriptor(descriptor&& fd) noexcept;
  descriptor& operator=(descriptor&& other) noexcept;
  ~descriptor()
  {
    if (file_path_) close();
  }

  int fd() const { return fd_; }

  bool linked() const { return linked_fd_ != nullptr; }

  operator int() const { return fd(); }

  void close();
  void close_linked();

  /**
   * @brief Wrapper for dup2 system call
   *
   * duplicates the fd to another
   *
   * @param other
   */
  void dup(const descriptor& other);

  /**
   * @brief Writes a given string to fd
   *
   * @param input
   */
  void write(std::string& input);

  /**
   * @brief Read from fd and return std::string
   *
   * @return std::string Contents of fd
   */
  std::string read();

private:
  descriptor(int fd, std::optional<std::filesystem::path> file_path) : descriptor(fd)
  {
    file_path_ = std::move(file_path);
  }
  std::optional<std::filesystem::path> file_path_;
  int fd_;
  descriptor* linked_fd_{nullptr};
  friend void link(descriptor& fd1, descriptor& fd2);
};

/**
 * @brief Returns an abstraction of stdin file descriptor
 *
 * @return descriptor stdin
 */
descriptor in();

/**
 * @brief Returns an abstraction of stdout file descriptor
 *
 * @return descriptor stdout
 */
descriptor out();

/**
 * @brief Returns an abstraction of stderr file descriptor
 *
 * @return descriptor stderr
 */
descriptor err();

enum class standard_filenos
{
  standard_in    = STDIN_FILENO,
  standard_out   = STDOUT_FILENO,
  standard_error = STDERR_FILENO,
  max_standard_fd
};

/*static*/ descriptor descriptor::open(std::filesystem::path file_name, int flags)
{
  if (int fd{::open(file_name.c_str(), flags)}; fd < 0)
  {
    throw exceptions::os_error{"Failed to open file " + file_name.string()};
  }
  else
  {
    return {fd, std::move(file_name)};
  }
}

/*static*/ std::pair<descriptor, descriptor> descriptor::create_pipe()
{
  int fd[2];
  if (::pipe(fd) < 0) throw exceptions::os_error{"Could not create a pipe!"};
  descriptor read_fd{fd[0]};
  descriptor write_fd{fd[1]};
  link(read_fd, write_fd);
  return {std::move(read_fd), std::move(write_fd)};
}

descriptor::descriptor(descriptor&& other) noexcept : descriptor() { *this = std::move(other); }

descriptor& descriptor::operator=(descriptor&& other) noexcept
{
  std::swap(fd_, other.fd_);
  std::swap(file_path_, other.file_path_);
  std::swap(linked_fd_, other.linked_fd_);
  return *this;
}

void descriptor::close()
{
  if (fd() > static_cast<int>(standard_filenos::max_standard_fd))
  {
    ::close(fd());
    file_path_.reset();
  }
}

void descriptor::close_linked()
{
  if (linked_fd_) linked_fd_->close();
}

void descriptor::dup(const descriptor& other) { ::dup2(fd(), other.fd()); }

void descriptor::write(std::string& input)
{
  if (::write(fd(), input.c_str(), input.size()) < static_cast<ssize_t>(input.size()))
  {
    throw exceptions::os_error{"Could not write the input to descriptor"};
  }
}

std::string descriptor::read()
{
  static char buf[2048];
  static std::string output;
  output.clear();
  while (::read(fd(), buf, 2048) > 0)
  {
    output.append(buf);
  }
  return output;
}

descriptor in() { return descriptor{static_cast<int>(standard_filenos::standard_in)}; };
descriptor out() { return descriptor{static_cast<int>(standard_filenos::standard_out)}; };
descriptor err() { return descriptor{static_cast<int>(standard_filenos::standard_error)}; };

/**
 * @brief Links two file descriptors
 *
 * This function is used to link a read and write file descriptor
 *
 * @param fd1
 * @param fd2
 */
void link(descriptor& fd1, descriptor& fd2)
{
  auto link_fds = [](descriptor& linking_fd, descriptor& linked_fd)
  {
    if (linking_fd.linked_fd_)
    {
      throw exceptions::usage_error{
          "You tried to link a file descriptor that is already linked to another file descriptor!"};
    }
    else
    {
      linking_fd.linked_fd_ = &linked_fd;
    }
  };
  link_fds(fd1, fd2);
  link_fds(fd2, fd1);
}

class posix_process
{

public:
  posix_process(std::string cmd) : cmd_{std::move(cmd)} {}

  void execute();

  int wait();

  descriptor& in() { return stdin_fd; }

  descriptor& out() { return stdout_fd; }

  descriptor& err() { return stderr_fd; }

private:
  std::string cmd_;
  descriptor stdin_fd{subprocess::in()}, stdout_fd{subprocess::out()}, stderr_fd{subprocess::err()};
  std::optional<int> pid_;
};

void posix_process::execute()
{
  auto dup_and_close = [](descriptor& fd, const descriptor& dup_to)
  {
    fd.dup(dup_to);
    fd.close();
    fd.close_linked();
  };
  auto close_if_linked = [](descriptor& fd)
  {
    if (fd.linked()) fd.close();
  };

  shell_expander sh{cmd_};

  if (int pid{::fork()}; pid < 0)
  {
    throw exceptions::os_error{"Failed to fork process"};
  }
  else if (pid == 0)
  {
    dup_and_close(stdin_fd, {STDIN_FILENO});
    dup_and_close(stdout_fd, {STDOUT_FILENO});
    dup_and_close(stderr_fd, {STDERR_FILENO});
    exit(::execvp(sh.argv()[0], sh.argv()));
  }
  else
  {
    close_if_linked(stdin_fd);
    close_if_linked(stdout_fd);
    close_if_linked(stderr_fd);
    pid_ = pid;
  }
}

int posix_process::wait()
{
  if (not pid_)
  {
    throw exceptions::usage_error{"posix_process.wait() called before posix_process.execute()"};
  }
  int waitstatus;
  ::waitpid(*(pid_), &waitstatus, 0);
  return waitstatus;
}

using process = posix_process;

/**
 * @brief Main interface class for subprocess library
 *
 * A class that contains a list of linked shell commands and is responsible
 * for managing their file descriptors such that their input and output can
 * be chained to other commands.
 *
 */
class command
{
public:
  command(std::string cmd) { processes.push_back(process{std::move(cmd)}); }

  /**
   * @brief Runs the command pipeline and throws on error.
   *
   * run only returns when the return code from the pipeline is 0.
   * Otherwise, it throws subprocess::exceptions::command_error.
   *
   * It can also throw subprocess::os_error if the command could not be
   * run due to some operating system level restrictions/errors.
   *
   * @return int Return code from the pipeline
   */
  int run();

  /**
   * @brief Runs the command pipeline and doesn't throw.
   *
   * run(nothrow_t) returns the exit code from the pipeline.
   * It doesn't throw subprocess::exceptions::command_error.
   *
   * However, it can still throw subprocess::os_error in case of
   * operating system level restrictions/errors.
   *
   * @return int Return code from the pipeline
   */
  int run(std::nothrow_t);

  /**
   * @brief Chains a command object to the current one.
   *
   * The output of the current command object is piped into
   * the other command object.
   *
   * Currently, the argument can only be an r-value because of
   * open file descriptors. This might be changed with implementation
   * of #4.
   *
   * @see https://github.com/rajatjain1997/subprocess/issues/4
   *
   * @param other Command ob
   * @return command&
   */
  command& operator|(command&& other);
  command& operator|(std::string other);

private:
  std::deque<process> processes;
  std::optional<std::pair<descriptor, std::reference_wrapper<std::string>>> captured_stdout, captured_stderr;

  friend command& operator<(command& cmd, descriptor fd);
  friend command&& operator<(command&& cmd, descriptor fd);
  friend command& operator<(command& cmd, std::string& input);
  friend command&& operator<(command&& cmd, std::string& input);
  friend command& operator<(command& cmd, std::filesystem::path file_name);
  friend command&& operator<(command&& cmd, std::filesystem::path file_name);

  friend command& operator>(command& cmd, descriptor fd);
  friend command&& operator>(command&& cmd, descriptor fd);
  friend command& operator>(command& cmd, std::string& output);
  friend command&& operator>(command&& cmd, std::string& output);
  friend command& operator>(command& cmd, const std::filesystem::path& file_name);
  friend command&& operator>(command&& cmd, const std::filesystem::path& file_name);

  friend command& operator>=(command& cmd, descriptor fd);
  friend command&& operator>=(command&& cmd, descriptor fd);
  friend command& operator>=(command& cmd, std::string& output);
  friend command&& operator>=(command&& cmd, std::string& output);
  friend command& operator>=(command& cmd, const std::filesystem::path& file_name);
  friend command&& operator>=(command&& cmd, const std::filesystem::path& file_name);

  friend command& operator>>(command& cmd, descriptor fd);
  friend command&& operator>>(command&& cmd, descriptor fd);
  friend command& operator>>(command& cmd, const std::filesystem::path& file_name);
  friend command&& operator>>(command&& cmd, const std::filesystem::path& file_name);

  friend command& operator>>=(command& cmd, descriptor fd);
  friend command&& operator>>=(command&& cmd, descriptor fd);
  friend command& operator>>=(command& cmd, const std::filesystem::path& file_name);
  friend command&& operator>>=(command&& cmd, const std::filesystem::path& file_name);
};

int command::run(std::nothrow_t)
{
  auto capture_stream = [](auto& optional_stream_pair)
  {
    if (optional_stream_pair)
    {
      std::string& output{optional_stream_pair->second.get()};
      output.clear();
      output = std::move(optional_stream_pair->first.read());
      optional_stream_pair->first.close();
    }
  };
  for (auto& process : processes)
  {
    process.execute();
  }
  capture_stream(captured_stdout);
  capture_stream(captured_stderr);
  int waitstatus;
  for (auto& process : processes)
  {
    waitstatus = process.wait();
  }
  return WEXITSTATUS(waitstatus);
}

int command::run()
{
  if (int status{run(std::nothrow)}; status != 0)
  {
    throw exceptions::command_error{"Command exited with code " + std::to_string(status) + ".", status};
  }
  else
  {
    return status;
  }
}

command& command::operator|(command&& other)
{
  auto [read_fd, write_fd]     = descriptor::create_pipe();
  other.processes.front().in() = std::move(read_fd);
  processes.back().out()       = std::move(write_fd);
  std::move(other.processes.begin(), other.processes.end(), std::back_inserter(processes));
  captured_stdout = std::move(other.captured_stdout);
  captured_stderr = std::move(other.captured_stderr);
  return *this;
}

command& command::operator|(std::string other) { return *this | command{std::move(other)}; }

command& operator>(command& cmd, descriptor fd)
{
  cmd.captured_stdout.reset();
  cmd.processes.back().out() = std::move(fd);
  return cmd;
}

command& operator>=(command& cmd, descriptor fd)
{
  cmd.processes.back().err() = std::move(fd);
  return cmd;
}

command& operator>>(command& cmd, descriptor fd) { return (cmd > std::move(fd)); }

command& operator>>=(command& cmd, descriptor fd) { return (cmd >= std::move(fd)); }

command& operator<(command& cmd, descriptor fd)
{
  cmd.processes.front().in() = std::move(fd);
  return cmd;
}

command& operator>=(command& cmd, std::string& output)
{
  auto [read_fd, write_fd] = descriptor::create_pipe();
  cmd > std::move(write_fd);
  cmd.captured_stderr = {std::move(read_fd), output};
  return cmd;
}

command& operator>(command& cmd, std::string& output)
{
  auto [read_fd, write_fd] = descriptor::create_pipe();
  cmd > std::move(write_fd);
  cmd.captured_stdout = {std::move(read_fd), output};
  return cmd;
}

command& operator<(command& cmd, std::string& input)
{
  auto [read_fd, write_fd] = descriptor::create_pipe();
  cmd < std::move(read_fd);
  write_fd.write(input);
  write_fd.close();
  return cmd;
}

command& operator>(command& cmd, const std::filesystem::path& file_name)
{
  return cmd > descriptor::open(file_name, O_WRONLY | O_CREAT | O_TRUNC);
}

command& operator>=(command& cmd, const std::filesystem::path& file_name)
{
  return cmd >= descriptor::open(file_name, O_WRONLY | O_CREAT | O_TRUNC);
}

command& operator>>(command& cmd, const std::filesystem::path& file_name)
{
  return cmd >> descriptor::open(file_name, O_WRONLY | O_CREAT | O_APPEND);
}

command& operator>>=(command& cmd, const std::filesystem::path& file_name)
{
  return cmd >>= descriptor::open(file_name, O_WRONLY | O_CREAT | O_APPEND);
}

command& operator<(command& cmd, std::filesystem::path file_name)
{
  return cmd < descriptor::open(file_name, O_RDONLY);
}

command&& operator>(command&& cmd, descriptor fd) { return std::move(cmd > std::move(fd)); }

command&& operator>=(command&& cmd, descriptor fd) { return std::move(cmd >= std::move(fd)); }

command&& operator>>(command&& cmd, descriptor fd) { return std::move(cmd >> std::move(fd)); }

command&& operator>>=(command&& cmd, descriptor fd) { return std::move(cmd >>= std::move(fd)); }

command&& operator<(command&& cmd, descriptor fd) { return std::move(cmd < std::move(fd)); }

command&& operator>=(command&& cmd, std::string& output) { return std::move(cmd >= output); }

command&& operator>(command&& cmd, std::string& output) { return std::move(cmd > output); }

command&& operator<(command&& cmd, std::string& input) { return std::move(cmd < input); }

command&& operator>(command&& cmd, const std::filesystem::path& file_name)
{
  return std::move(cmd > file_name);
}

command&& operator>=(command&& cmd, const std::filesystem::path& file_name)
{
  return std::move(cmd >= file_name);
}
command&& operator>>(command&& cmd, const std::filesystem::path& file_name)
{
  return std::move(cmd >> file_name);
}

command&& operator>>=(command&& cmd, const std::filesystem::path& file_name)
{
  return std::move(cmd >>= file_name);
}

command&& operator<(command&& cmd, std::filesystem::path file_name) { return std::move(cmd < file_name); }

namespace literals
{
command operator""_cmd(const char* cmd, size_t) { return command{cmd}; }
} // namespace literals

} // namespace subprocess
