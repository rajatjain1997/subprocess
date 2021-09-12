#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <subprocess/subprocess_export.h>
#include <tuple>

namespace subprocess
{

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
class SUBPROCESS_EXPORT file_descriptor
{
public:
  /**
   * @brief Opens a file and returns a file_descriptor object
   *
   * @param file_name Name of the file to open
   * @param flags Flags with which to open the file
   * @return file_descriptor FD representing the opened file
   */
  static file_descriptor open(std::filesystem::path file_name, int flags);

  /**
   * @brief Create an OS level pipe and return read and write fds.
   *
   * @return std::pair<file_descriptor, file_descriptor> A pair of linked file_descriptos [read_fd, write_fd]
   */
  static std::pair<file_descriptor, file_descriptor> create_pipe();

  file_descriptor(int fd = -1) : fd_{fd} {}
  file_descriptor(const file_descriptor& other) = delete;
  file_descriptor& operator=(const file_descriptor& other) = delete;
  file_descriptor(file_descriptor&& fd) noexcept;
  file_descriptor& operator=(file_descriptor&& other) noexcept;
  ~file_descriptor();

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
  void dup(const file_descriptor& other);

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
  file_descriptor(int fd, std::optional<std::filesystem::path> file_path) : file_descriptor(fd)
  {
    file_path_ = std::move(file_path);
  }
  static const int kMinFD_;
  std::optional<std::filesystem::path> file_path_;
  int fd_;
  file_descriptor* linked_fd_{nullptr};
  friend void link(file_descriptor& fd1, file_descriptor& fd2);
};

/**
 * @brief Returns an abstraction of stdin file descriptor
 *
 * @return file_descriptor stdin
 */
SUBPROCESS_EXPORT file_descriptor in();

/**
 * @brief Returns an abstraction of stdout file descriptor
 *
 * @return file_descriptor stdout
 */
SUBPROCESS_EXPORT file_descriptor out();

/**
 * @brief Returns an abstraction of stderr file descriptor
 *
 * @return file_descriptor stderr
 */
SUBPROCESS_EXPORT file_descriptor err();

/**
 * @brief Links two file descriptors
 *
 * This function is used to link a read and write file descriptor
 *
 * @param fd1
 * @param fd2
 */
SUBPROCESS_EXPORT void link(file_descriptor& fd1, file_descriptor& fd2);
} // namespace subprocess