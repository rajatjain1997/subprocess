#pragma once

#include <algorithm>
#include <array>
#include <filesystem>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

extern "C"
{
#include <fcntl.h>
#include <spawn.h>
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
  [[nodiscard]] int return_code() const { return return_code_; }
};
} // namespace exceptions

namespace posix_util
{

enum class standard_filenos
{
  standard_in    = STDIN_FILENO,
  standard_out   = STDOUT_FILENO,
  standard_error = STDERR_FILENO,
  max_standard_fd
};

/**
 * @brief POSIX shell argument expander
 *
 * This class wraps the wordexp syscall in a RAII wrapper. wordexp is a POSIX
 * system call that emulates shell parsing for a string as shell would.
 *
 * The return type of wordexp includes includes a delimited string containing
 * args for the function that needs to be called.
 *
 * @see https://linux.die.net/man/3/wordexp
 *
 */
class shell_expander
{
public:
  explicit shell_expander(const std::string& s) { ::wordexp(s.c_str(), &parsed_args_, 0); }
  shell_expander(const shell_expander&)     = default;
  shell_expander(shell_expander&&) noexcept = default;
  shell_expander& operator=(const shell_expander&) = default;
  shell_expander& operator=(shell_expander&&) noexcept = default;
  ~shell_expander() { ::wordfree(&parsed_args_); }

  [[nodiscard]] decltype(std::declval<wordexp_t>().we_wordv) argv() const& { return parsed_args_.we_wordv; }

private:
  ::wordexp_t parsed_args_{};
};

} // namespace posix_util

/**
 * @brief Abstracts file descriptors
 *
 * Member functions of this class and its descendents
 * wrap syscalls to commonly used file descriptor functions.
 *
 * As a user, you can derive from this class to implement your own
 * custom descriptors.
 *
 */
class descriptor
{
public:
  descriptor() = default;
  explicit descriptor(int fd) : fd_{fd} {}
  descriptor(const descriptor&)     = default;
  descriptor(descriptor&&) noexcept = default;
  descriptor& operator=(const descriptor&) = default;
  descriptor& operator=(descriptor&&) noexcept = default;
  virtual ~descriptor()                        = default;

  /**
   * @brief Returns the encapsulated file descriptor
   *
   * The return value of fd() is used by subprocess::process
   * and sent to child processes. This is the fd() that the
   * process will read/write from.
   *
   * @return int OS-level file descriptor for subprocess I/O
   */
  [[nodiscard]] int fd() const { return fd_; }

  /**
   * @brief Marks whether the subprocess should close the FD
   *
   * @return true The subprocess should close the FD
   * @return false The subprocess shouldn't close the FD
   */
  [[nodiscard]] virtual bool closable() const { return false; }

  /**
   * @brief Initialize call before the process runs
   *
   * open() is called by subprocess::execute() right before
   * spawning the child process. This is the function to write
   * for the set up of your I/O from the process.
   */
  virtual void open() {}

  /**
   * @brief Tear down the descriptor
   *
   * close() is called by subprocess::execute() after the
   * process is spawned, but before waiting. This should
   * ideally be the place where you should tear down the constructs
   * that were required for process I/O.
   */
  virtual void close() {}

protected:
  int fd_{-1};
};

struct in_t
{
};
struct out_t
{
};
struct err_t
{
};

static inline in_t in;
static inline out_t out;
static inline err_t err;

/**
 * @brief A shorthand for describing a descriptor ptr
 *
 * @tparam T descriptor or one of its derived classes
 */
template <typename T>
using descriptor_ptr_t = std::enable_if_t<std::is_base_of_v<descriptor, T>, std::shared_ptr<T>>;

/**
 * @brief A shorthand for denoting an owning-pointer for the descriptor base class
 *
 */
using descriptor_ptr = descriptor_ptr_t<descriptor>;

/**
 * @brief Wraps std::make_unique. Used only for self-documentation purposes.
 *
 * @tparam T The type of descriptor to create.
 * @tparam Args
 * @param args Constructor args that should be passed to the descriptor's constructor
 * @return descriptor_ptr_t<T> Returns a descriptor_ptr
 */
template <typename T, typename... Args> inline descriptor_ptr_t<T> make_descriptor(Args&&... args)
{
  return std::make_shared<T>(std::forward<Args>(args)...);
}

/**
 * @brief Adds write ability to descriptor
 *
 * Additionally, the class is marked closable. All output descriptors
 * inherit from this class.
 */
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
  void close() override;
  [[nodiscard]] bool closable() const override { return true; }

private:
  bool closed_{false};
};

inline void odescriptor::write(std::string& input)
{
  if (::write(fd(), input.c_str(), input.size()) < static_cast<ssize_t>(input.size()))
  {
    throw exceptions::os_error{"Could not write the input to descriptor"};
  }
}

inline void odescriptor::close()
{
  if (not closed_)
  {
    ::close(fd());
    closed_ = true;
  }
}

/**
 * @brief Adds read ability to descriptor
 *
 * Additionally, the class is marked closable. All input descriptors
 * inherit from this class.
 */
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
  void close() override;
  [[nodiscard]] bool closable() const override { return true; }

private:
  bool closed_{false};
};

inline void idescriptor::close()
{
  if (not closed_)
  {
    ::close(fd());
    closed_ = true;
  }
}

inline std::string idescriptor::read()
{
  static constexpr int buf_size{2048};
  static std::array<char, buf_size> buf;
  static std::string output;
  output.clear();
  while (::read(fd(), buf.data(), buf_size) > 0)
  {
    output.append(buf.data());
  }
  return output;
}

/**
 * @brief Wraps a descriptor mapping to a file on the disc
 *
 * The class exports the open() syscall and is the parent class of
 * ofile_descriptor and ifile_descriptor.
 *
 */
class file_descriptor : public virtual descriptor
{
public:
  file_descriptor(std::string path, int mode) : path_{std::move(path)}, mode_{mode} {}
  file_descriptor(const file_descriptor&)     = default;
  file_descriptor(file_descriptor&&) noexcept = default;
  file_descriptor& operator=(const file_descriptor&) = default;
  file_descriptor& operator=(file_descriptor&&) noexcept = default;
  ~file_descriptor() override { close(); }
  void open() override;

private:
  std::string path_;
  int mode_;
};

inline void file_descriptor::open()
{
  if (int fd{::open(path_.c_str(), mode_)}; fd >= 0)
  {
    fd_ = fd;
  }
  else
  {
    throw exceptions::os_error{"Failed to open file " + path_};
  }
}

/**
 * @brief Always opens the file in write-mode
 *
 */
class ofile_descriptor : public virtual odescriptor, public virtual file_descriptor
{
public:
  using file_descriptor::closable;
  using file_descriptor::close;
  explicit ofile_descriptor(std::string path, int mode = 0)
      : file_descriptor{std::move(path), O_WRONLY | mode}
  {
  }
};

/**
 * @brief Always opens the file in read-mode
 *
 */
class ifile_descriptor : public virtual idescriptor, public virtual file_descriptor
{
public:
  using file_descriptor::closable;
  using file_descriptor::close;
  explicit ifile_descriptor(std::string path, int mode = 0)
      : file_descriptor{std::move(path), O_RDONLY | mode}
  {
  }
};

class ipipe_descriptor;

class opipe_descriptor : public odescriptor
{
public:
  opipe_descriptor() = default;
  void open() override;

protected:
  ipipe_descriptor* linked_fd_{nullptr};
  friend class ipipe_descriptor;
  friend void link(ipipe_descriptor& fd1, opipe_descriptor& fd2);
};

class ipipe_descriptor : public idescriptor
{
public:
  ipipe_descriptor() = default;
  void open() override;

protected:
  opipe_descriptor* linked_fd_{nullptr};
  friend class opipe_descriptor;
  friend void link(ipipe_descriptor& fd1, opipe_descriptor& fd2);
};

inline void opipe_descriptor::open()
{
  if (fd() > 0)
  {
    return;
  }
  std::array<int, 2> fd{};
  if (::pipe(fd.data()) < 0)
  {
    throw exceptions::os_error{"Could not create a pipe!"};
  }
  linked_fd_->fd_ = fd[0];
  fd_             = fd[1];
}

inline void ipipe_descriptor::open()
{
  if (fd() > 0)
  {
    return;
  }
  std::array<int, 2> fd{};
  if (::pipe(fd.data()) < 0)
  {
    throw exceptions::os_error{"Could not create a pipe!"};
  }
  fd_             = fd[0];
  linked_fd_->fd_ = fd[1];
}

class ovariable_descriptor : public opipe_descriptor
{
public:
  explicit ovariable_descriptor(std::string& output_var) : output_{output_var} { linked_fd_ = &input_pipe_; }

  void close() override;
  virtual void read() { output_ = input_pipe_.read(); }

private:
  std::string& output_;
  ipipe_descriptor input_pipe_;
};

inline void ovariable_descriptor::close()
{
  opipe_descriptor::close();
  read();
  input_pipe_.close();
}
class ivariable_descriptor : public ipipe_descriptor
{
public:
  explicit ivariable_descriptor(std::string input_data) : input_{std::move(input_data)}
  {
    linked_fd_ = &output_pipe_;
  }

  void open() override;
  virtual void write() { output_pipe_.write(input_); }

private:
  std::string input_;
  opipe_descriptor output_pipe_;
};

inline void ivariable_descriptor::open()
{
  ipipe_descriptor::open();
  write();
  output_pipe_.close();
}

/**
 * @brief Create an OS level pipe and return read and write fds.
 *
 * @return std::pair<descriptor, descriptor> A pair of linked file_descriptos [read_fd, write_fd]
 */
inline std::pair<descriptor_ptr_t<ipipe_descriptor>, descriptor_ptr_t<opipe_descriptor>> create_pipe()
{
  auto read_fd{make_descriptor<ipipe_descriptor>()};
  auto write_fd{make_descriptor<opipe_descriptor>()};
  link(*read_fd, *write_fd);
  return std::pair{std::move(read_fd), std::move(write_fd)};
}

/**
 * @brief Returns an abstraction of stdout file descriptor
 *
 * @return descriptor stdout
 */
inline descriptor_ptr std_in()
{
  static auto stdin_fd{
      make_descriptor<descriptor>(static_cast<int>(posix_util::standard_filenos::standard_in))};
  return stdin_fd;
};
/**
 * @brief Returns an abstraction of stdin file descriptor
 *
 * @return descriptor stdin
 */
inline descriptor_ptr std_out()
{
  static auto stdout_fd{
      make_descriptor<descriptor>(static_cast<int>(posix_util::standard_filenos::standard_out))};
  return stdout_fd;
};
/**
 * @brief Returns an abstraction of stderr file descriptor
 *
 * @return descriptor stderr
 */
inline descriptor_ptr std_err()
{
  static auto stderr_fd{
      make_descriptor<descriptor>(static_cast<int>(posix_util::standard_filenos::standard_error))};
  return stderr_fd;
};

/**
 * @brief Links two file descriptors
 *
 * This function is used to link a read and write file descriptor
 *
 * @param fd1
 * @param fd2
 */
inline void link(ipipe_descriptor& fd1, opipe_descriptor& fd2)
{
  if (fd1.linked_fd_ != nullptr or fd2.linked_fd_ != nullptr)
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
  explicit posix_process(std::string cmd) : cmd_{std::move(cmd)} {}

  void execute();

  int wait();

  const descriptor& in() { return *stdin_fd_; }

  const descriptor& out() { return *stdout_fd_; }

  const descriptor& err() { return *stderr_fd_; }

  void in(descriptor_ptr&& fd) { stdin_fd_ = std::move(fd); }
  void out(descriptor_ptr&& fd) { stdout_fd_ = std::move(fd); }
  void err(descriptor_ptr&& fd) { stderr_fd_ = std::move(fd); }
  void out(err_t /*unused*/) { stdout_fd_ = stderr_fd_; }
  void err(out_t /*unused*/) { stderr_fd_ = stdout_fd_; }

private:
  std::string cmd_;
  descriptor_ptr stdin_fd_{subprocess::std_in()}, stdout_fd_{subprocess::std_out()},
      stderr_fd_{subprocess::std_err()};
  std::optional<int> pid_;
};

inline void posix_process::execute()
{
  auto dup_and_close = [](posix_spawn_file_actions_t* action, descriptor_ptr& fd, const descriptor& dup_to)
  {
    fd->open();
    posix_spawn_file_actions_adddup2(action, fd->fd(), dup_to.fd());
    if (fd->closable())
    {
      posix_spawn_file_actions_addclose(action, fd->fd());
    }
  };

  posix_util::shell_expander sh{cmd_};
  posix_spawn_file_actions_t action;

  posix_spawn_file_actions_init(&action);
  dup_and_close(&action, stdin_fd_, descriptor{static_cast<int>(posix_util::standard_filenos::standard_in)});
  dup_and_close(&action, stdout_fd_,
                descriptor{static_cast<int>(posix_util::standard_filenos::standard_out)});
  dup_and_close(&action, stderr_fd_,
                descriptor{static_cast<int>(posix_util::standard_filenos::standard_error)});
  int pid{};
  if (::posix_spawnp(&pid, sh.argv()[0], &action, nullptr, sh.argv(), nullptr) != 0)
  {
    throw exceptions::os_error{"Failed to spawn process"};
  }
  posix_spawn_file_actions_destroy(&action);
  pid_ = pid;
  stdin_fd_->close();
  stdout_fd_->close();
  stderr_fd_->close();
}

inline int posix_process::wait()
{
  if (not pid_)
  {
    throw exceptions::usage_error{"posix_process.wait() called before posix_process.execute()"};
  }
  int waitstatus{};
  ::waitpid(*(pid_), &waitstatus, 0);
  return WEXITSTATUS(waitstatus);
}

using process_t = posix_process;

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
  explicit command(std::string cmd) { processes_.emplace_back(std::move(cmd)); }

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
  int run(std::nothrow_t /*unused*/);

  /**
   * @brief Chains a command object to the current one.
   *
   * The output of the current command object is piped into
   * the other command object.
   *
   * Currently, the argument can only be an r-value because of
   * open file descriptors. This will be changed soon.
   *
   * @param other Command ob
   * @return command&
   */
  command& operator|(command&& other);
  command& operator|(std::string other);

private:
  std::list<process_t> processes_;

  friend command& operator<(command& cmd, descriptor_ptr fd);

  friend command& operator>(command& cmd, err_t err_tag);
  friend command& operator>(command& cmd, descriptor_ptr fd);

  friend command& operator>=(command& cmd, out_t out_tag);
  friend command& operator>=(command& cmd, descriptor_ptr fd);
};

inline int command::run(std::nothrow_t /*unused*/)
{
  for (auto& process : processes_)
  {
    process.execute();
  }
  int waitstatus{};
  for (auto& process : processes_)
  {
    waitstatus = process.wait();
  }
  return waitstatus;
}

inline int command::run()
{
  int status{run(std::nothrow)};
  if (status != 0)
  {
    throw exceptions::command_error{"Command exited with code " + std::to_string(status) + ".", status};
  }
  return status;
}

inline command& command::operator|(command&& other)
{
  auto [read_fd, write_fd] = create_pipe();
  other.processes_.front().in(std::move(read_fd));
  processes_.back().out(std::move(write_fd));
  processes_.splice(processes_.end(), std::move(other.processes_));
  return *this;
}

inline command& command::operator|(std::string other) { return *this | command{std::move(other)}; }

inline command& operator<(command& cmd, descriptor_ptr fd)
{
  cmd.processes_.front().in(std::move(fd));
  return cmd;
}

inline command& operator>(command& cmd, err_t err_tag)
{
  cmd.processes_.back().out(err_tag);
  return cmd;
}

inline command& operator>(command& cmd, descriptor_ptr fd)
{
  cmd.processes_.back().out(std::move(fd));
  return cmd;
}

inline command& operator>=(command& cmd, out_t out_tag)
{
  cmd.processes_.back().err(out_tag);
  return cmd;
}

inline command& operator>=(command& cmd, descriptor_ptr fd)
{
  cmd.processes_.back().err(std::move(fd));
  return cmd;
}

inline command& operator>>(command& cmd, descriptor_ptr fd) { return (cmd > std::move(fd)); }

inline command& operator>>=(command& cmd, descriptor_ptr fd) { return (cmd >= std::move(fd)); }

inline command& operator>=(command& cmd, std::string& output)
{
  return cmd >= make_descriptor<ovariable_descriptor>(output);
}

inline command& operator>(command& cmd, std::string& output)
{
  return cmd > make_descriptor<ovariable_descriptor>(output);
}

inline command& operator<(command& cmd, std::string& input)
{
  return cmd < make_descriptor<ivariable_descriptor>(input);
}

inline command& operator>(command& cmd, std::filesystem::path file_name)
{
  return cmd > make_descriptor<ofile_descriptor>(file_name.string(), O_CREAT | O_TRUNC);
}

inline command& operator>=(command& cmd, std::filesystem::path file_name)
{
  return cmd >= make_descriptor<ofile_descriptor>(file_name.string(), O_CREAT | O_TRUNC);
}

inline command& operator>>(command& cmd, std::filesystem::path file_name)
{
  return cmd >> make_descriptor<ofile_descriptor>(file_name.string(), O_CREAT | O_APPEND);
}

inline command& operator>>=(command& cmd, std::filesystem::path file_name)
{
  return cmd >>= make_descriptor<ofile_descriptor>(file_name.string(), O_CREAT | O_APPEND);
}

inline command& operator<(command& cmd, std::filesystem::path file_name)
{
  return cmd < make_descriptor<ofile_descriptor>(file_name.string());
}

inline command&& operator>(command&& cmd, descriptor_ptr fd) { return std::move(cmd > std::move(fd)); }

inline command&& operator>=(command&& cmd, descriptor_ptr fd) { return std::move(cmd >= std::move(fd)); }

inline command&& operator>>(command&& cmd, descriptor_ptr fd) { return std::move(cmd >> std::move(fd)); }

inline command&& operator>>=(command&& cmd, descriptor_ptr fd) { return std::move(cmd >>= std::move(fd)); }

inline command&& operator<(command&& cmd, descriptor_ptr fd) { return std::move(cmd < std::move(fd)); }

inline command&& operator>=(command&& cmd, out_t out_tag) { return std::move(cmd >= out_tag); }

inline command&& operator>=(command&& cmd, std::string& output) { return std::move(cmd >= output); }

inline command&& operator>(command&& cmd, err_t err_tag) { return std::move(cmd > err_tag); }

inline command&& operator>(command&& cmd, std::string& output) { return std::move(cmd > output); }

inline command&& operator<(command&& cmd, std::string& input) { return std::move(cmd < input); }

inline command&& operator>(command&& cmd, std::filesystem::path file_name)
{
  return std::move(cmd > std::move(file_name));
}

inline command&& operator>=(command&& cmd, std::filesystem::path file_name)
{
  return std::move(cmd >= std::move(file_name));
}
inline command&& operator>>(command&& cmd, std::filesystem::path file_name)
{
  return std::move(cmd >> std::move(file_name));
}

inline command&& operator>>=(command&& cmd, std::filesystem::path file_name)
{
  return std::move(cmd >>= std::move(file_name));
}

inline command&& operator<(command&& cmd, std::filesystem::path file_name)
{
  return std::move(cmd < std::move(file_name));
}

namespace literals
{
inline command operator""_cmd(const char* cmd, size_t /*unused*/) { return command{cmd}; }
} // namespace literals

} // namespace subprocess
