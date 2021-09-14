#pragma once

#include <algorithm>
#include <deque>
#include <filesystem>
#include <functional>
#include <iostream>
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
#include <spawn.h>
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
  static auto open(std::filesystem::path file_name, int flags);

  descriptor(int fd = -1) : fd_{fd} {}
  virtual ~descriptor() {}

  int fd() const { return fd_; }

  bool linked() const { return linked_fd_ != nullptr; }

  operator int() const { return fd(); }

  virtual void open() {}
  virtual void close() {}
  virtual bool closable() { return false; }

  descriptor* linked_fd_{nullptr};

protected:
  int fd_;
};

using descriptor_ptr_t = std::unique_ptr<descriptor>;

template <typename T, typename... Args> std::unique_ptr<T> make_descriptor(Args&&... args)
{
  return std::make_unique<T>(std::forward<Args>(args)...);
}

class odescriptor : public virtual descriptor
{
public:
  using descriptor::descriptor;
  /**
   * @brief Writes a given string to fd
   *
   * @param input
   */
  virtual void write(std::string& input);
  virtual void close() override;
  virtual bool closable() override { return true; }

private:
  bool closed_{false};
};

class idescriptor : public virtual descriptor
{
public:
  using descriptor::descriptor;
  /**
   * @brief Read from fd and return std::string
   *
   * @return std::string Contents of fd
   */
  virtual std::string read();
  virtual void close() override;
  virtual bool closable() override { return true; }

private:
  bool closed_{false};
};

class file_descriptor : public virtual descriptor
{
public:
  file_descriptor(std::filesystem::path path, int mode) : path_{path}, mode_{mode} {}
  virtual ~file_descriptor() { close(); }
  virtual void open() override;

public:
  std::filesystem::path path_;
  int mode_;
};

class ofile_descriptor : public virtual odescriptor, public virtual file_descriptor
{
public:
  using file_descriptor::closable;
  using file_descriptor::close;
  ofile_descriptor(std::filesystem::path path, int mode) : file_descriptor{std::move(path), mode} {}
};

class ifile_descriptor : public virtual idescriptor, public virtual file_descriptor
{
public:
  using file_descriptor::closable;
  using file_descriptor::close;
  ifile_descriptor(std::filesystem::path path, int mode) : file_descriptor{std::move(path), mode} {}
};

class ipipe_descriptor;

class opipe_descriptor : public virtual odescriptor
{
public:
  opipe_descriptor() {}
  virtual void open() override;

protected:
  ipipe_descriptor* linked_fd_{nullptr};
  friend class ipipe_descriptor;
  friend void link(ipipe_descriptor& fd1, opipe_descriptor& fd2);
};

class ipipe_descriptor : public virtual idescriptor
{
public:
  ipipe_descriptor() {}
  virtual void open() override;

protected:
  opipe_descriptor* linked_fd_{nullptr};
  friend class opipe_descriptor;
  friend void link(ipipe_descriptor& fd1, opipe_descriptor& fd2);
};

class ovariable_descriptor : public virtual opipe_descriptor
{
public:
  ovariable_descriptor(std::string& output_var) : output_{output_var} { linked_fd_ = &input_pipe_; }

  virtual void close() override;
  virtual void read();

private:
  std::string& output_;
  ipipe_descriptor input_pipe_;
};
class ivariable_descriptor : public virtual ipipe_descriptor
{
public:
  ivariable_descriptor(std::string input_data) : input_{input_data} { linked_fd_ = &output_pipe; }

  virtual void open() override;
  virtual void write();

private:
  std::string input_;
  opipe_descriptor output_pipe;
};

void opipe_descriptor::open()
{
  if (fd() > 0)
  {
    return;
  }
  int fd[2];
  if (::pipe(fd) < 0) throw exceptions::os_error{"Could not create a pipe!"};
  linked_fd_->fd_ = fd[0];
  fd_             = fd[1];
}

void ipipe_descriptor::open()
{
  if (fd() > 0)
  {
    return;
  }
  int fd[2];
  if (::pipe(fd) < 0) throw exceptions::os_error{"Could not create a pipe!"};
  fd_             = fd[0];
  linked_fd_->fd_ = fd[1];
}

void ovariable_descriptor::close()
{
  opipe_descriptor::close();
  read();
  input_pipe_.close();
}

void ovariable_descriptor::read() { output_ = input_pipe_.read(); }

void ivariable_descriptor::open()
{
  ipipe_descriptor::open();
  write();
  output_pipe.close();
}

void ivariable_descriptor::write() { output_pipe.write(input_); }

enum class standard_filenos
{
  standard_in    = STDIN_FILENO,
  standard_out   = STDOUT_FILENO,
  standard_error = STDERR_FILENO,
  max_standard_fd
};

void file_descriptor::open()
{
  if (int fd{::open(path_.c_str(), mode_)}; fd < 0)
  {
    throw exceptions::os_error{"Failed to open file " + path_.string()};
  }
  else
  {
    fd_ = fd;
  }
}

void idescriptor::close()
{
  if (not closed_)
  {
    ::close(fd());
    closed_ = true;
  }
}

void odescriptor::close()
{
  if (not closed_)
  {
    ::close(fd());
    closed_ = true;
  }
}

/**
 * @brief Create an OS level pipe and return read and write fds.
 *
 * @return std::pair<descriptor, descriptor> A pair of linked file_descriptos [read_fd, write_fd]
 */
std::pair<std::unique_ptr<ipipe_descriptor>, std::unique_ptr<opipe_descriptor>> create_pipe()
{
  auto read_fd{make_descriptor<ipipe_descriptor>()};
  auto write_fd{make_descriptor<opipe_descriptor>()};
  link(*read_fd, *write_fd);
  return std::pair{std::move(read_fd), std::move(write_fd)};
}

void odescriptor::write(std::string& input)
{
  if (::write(fd(), input.c_str(), input.size()) < static_cast<ssize_t>(input.size()))
  {
    throw exceptions::os_error{"Could not write the input to descriptor"};
  }
}

std::string idescriptor::read()
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

/**
 * @brief Returns an abstraction of stdout file descriptor
 *
 * @return descriptor stdout
 */
auto in() { return make_descriptor<descriptor>(static_cast<int>(standard_filenos::standard_in)); };
/**
 * @brief Returns an abstraction of stdin file descriptor
 *
 * @return descriptor stdin
 */
auto out() { return make_descriptor<descriptor>(static_cast<int>(standard_filenos::standard_out)); };
/**
 * @brief Returns an abstraction of stderr file descriptor
 *
 * @return descriptor stderr
 */
auto err() { return make_descriptor<descriptor>(static_cast<int>(standard_filenos::standard_error)); };

/**
 * @brief Links two file descriptors
 *
 * This function is used to link a read and write file descriptor
 *
 * @param fd1
 * @param fd2
 */
void link(ipipe_descriptor& fd1, opipe_descriptor& fd2)
{
  auto link_fds = [](descriptor& linking_fd, descriptor& linked_fd)
  {
    if (linking_fd.linked_fd_)
    {
    }
    else
    {
      linking_fd.linked_fd_ = &linked_fd;
    }
  };
  if (fd1.linked_fd_ or fd2.linked_fd_)
  {
    throw exceptions::usage_error{
        "You tried to link a file descriptor that is already linked to another file descriptor!"};
  }
  fd1.linked_fd_ = &fd2;
  fd2.linked_fd_ = &fd1;
}

class posix_process
{

public:
  posix_process(std::string cmd) : cmd_{std::move(cmd)} {}

  void execute();

  int wait();

  const descriptor& in() { return *stdin_fd; }

  const descriptor& out() { return *stdout_fd; }

  const descriptor& err() { return *stderr_fd; }

  void in(descriptor_ptr_t&& fd) { stdin_fd = std::move(fd); }
  void out(descriptor_ptr_t&& fd) { stdout_fd = std::move(fd); }
  void err(descriptor_ptr_t&& fd) { stderr_fd = std::move(fd); }

private:
  std::string cmd_;
  descriptor_ptr_t stdin_fd{subprocess::in()}, stdout_fd{subprocess::out()}, stderr_fd{subprocess::err()};
  std::optional<int> pid_;
};

void posix_process::execute()
{
  auto dup_and_close = [](posix_spawn_file_actions_t* action, descriptor_ptr_t& fd, const descriptor& dup_to)
  {
    fd->open();
    posix_spawn_file_actions_adddup2(action, fd->fd(), dup_to.fd());
    if (fd->closable()) posix_spawn_file_actions_addclose(action, fd->fd());
  };

  shell_expander sh{cmd_};
  posix_spawn_file_actions_t action;

  posix_spawn_file_actions_init(&action);
  dup_and_close(&action, stdin_fd, {STDIN_FILENO});
  dup_and_close(&action, stdout_fd, {STDOUT_FILENO});
  dup_and_close(&action, stderr_fd, {STDERR_FILENO});
  int pid;
  if (::posix_spawnp(&pid, sh.argv()[0], &action, NULL, sh.argv(), NULL))
  {
    throw exceptions::os_error{"Failed to spawn process"};
  }
  pid_ = pid;
  stdin_fd->close();
  stdout_fd->close();
  stderr_fd->close();
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

  friend command& operator<(command& cmd, descriptor_ptr_t fd);
  friend command&& operator<(command&& cmd, descriptor_ptr_t fd);
  // friend command& operator<(command& cmd, std::string& input);
  // friend command&& operator<(command&& cmd, std::string& input);
  friend command& operator<(command& cmd, std::filesystem::path file_name);
  friend command&& operator<(command&& cmd, std::filesystem::path file_name);

  friend command& operator>(command& cmd, descriptor_ptr_t fd);
  friend command&& operator>(command&& cmd, descriptor_ptr_t fd);
  // friend command& operator>(command& cmd, std::string& output);
  // friend command&& operator>(command&& cmd, std::string& output);
  friend command& operator>(command& cmd, const std::filesystem::path& file_name);
  friend command&& operator>(command&& cmd, const std::filesystem::path& file_name);

  friend command& operator>=(command& cmd, descriptor_ptr_t fd);
  friend command&& operator>=(command&& cmd, descriptor_ptr_t fd);
  // friend command& operator>=(command& cmd, std::string& output);
  // friend command&& operator>=(command&& cmd, std::string& output);
  friend command& operator>=(command& cmd, const std::filesystem::path& file_name);
  friend command&& operator>=(command&& cmd, const std::filesystem::path& file_name);

  friend command& operator>>(command& cmd, descriptor_ptr_t fd);
  friend command&& operator>>(command&& cmd, descriptor_ptr_t fd);
  friend command& operator>>(command& cmd, const std::filesystem::path& file_name);
  friend command&& operator>>(command&& cmd, const std::filesystem::path& file_name);

  friend command& operator>>=(command& cmd, descriptor_ptr_t fd);
  friend command&& operator>>=(command&& cmd, descriptor_ptr_t fd);
  friend command& operator>>=(command& cmd, const std::filesystem::path& file_name);
  friend command&& operator>>=(command&& cmd, const std::filesystem::path& file_name);
};

int command::run(std::nothrow_t)
{
  for (auto& process : processes)
  {
    process.execute();
  }
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
  auto [read_fd, write_fd] = create_pipe();
  other.processes.front().in(std::move(read_fd));
  processes.back().out(std::move(write_fd));
  std::move(other.processes.begin(), other.processes.end(), std::back_inserter(processes));
  return *this;
}

command& command::operator|(std::string other) { return *this | command{std::move(other)}; }

command& operator>(command& cmd, descriptor_ptr_t fd)
{
  cmd.processes.back().out(std::move(fd));
  return cmd;
}

command& operator>=(command& cmd, descriptor_ptr_t fd)
{
  cmd.processes.back().err(std::move(fd));
  return cmd;
}

command& operator>>(command& cmd, descriptor_ptr_t fd) { return (cmd > std::move(fd)); }

command& operator>>=(command& cmd, descriptor_ptr_t fd) { return (cmd >= std::move(fd)); }

command& operator<(command& cmd, descriptor_ptr_t fd)
{
  cmd.processes.front().in(std::move(fd));
  return cmd;
}

command& operator>=(command& cmd, std::string& output)
{
  return cmd >= make_descriptor<ovariable_descriptor>(output);
}

command& operator>(command& cmd, std::string& output)
{
  return cmd > make_descriptor<ovariable_descriptor>(output);
}

command& operator<(command& cmd, std::string& input)
{
  return cmd < make_descriptor<ivariable_descriptor>(input);
}

command& operator>(command& cmd, const std::filesystem::path& file_name)
{
  return cmd > make_descriptor<ofile_descriptor>(file_name, O_WRONLY | O_CREAT | O_TRUNC);
}

command& operator>=(command& cmd, const std::filesystem::path& file_name)
{
  return cmd >= make_descriptor<ofile_descriptor>(file_name, O_WRONLY | O_CREAT | O_TRUNC);
}

command& operator>>(command& cmd, const std::filesystem::path& file_name)
{
  return cmd >> make_descriptor<ofile_descriptor>(file_name, O_WRONLY | O_CREAT | O_APPEND);
}

command& operator>>=(command& cmd, const std::filesystem::path& file_name)
{
  return cmd >>= make_descriptor<ofile_descriptor>(file_name, O_WRONLY | O_CREAT | O_APPEND);
}

command& operator<(command& cmd, std::filesystem::path file_name)
{
  return cmd < make_descriptor<ofile_descriptor>(file_name, O_RDONLY);
}

command&& operator>(command&& cmd, descriptor_ptr_t fd) { return std::move(cmd > std::move(fd)); }

command&& operator>=(command&& cmd, descriptor_ptr_t fd) { return std::move(cmd >= std::move(fd)); }

command&& operator>>(command&& cmd, descriptor_ptr_t fd) { return std::move(cmd >> std::move(fd)); }

command&& operator>>=(command&& cmd, descriptor_ptr_t fd) { return std::move(cmd >>= std::move(fd)); }

command&& operator<(command&& cmd, descriptor_ptr_t fd) { return std::move(cmd < std::move(fd)); }

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
