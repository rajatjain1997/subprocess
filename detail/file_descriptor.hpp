#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <tuple>

namespace subprocess
{
class file_descriptor
{
public:
  static file_descriptor open(std::filesystem::path file_name, int flags);
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
  void dup(const file_descriptor& other);
  void write(std::string& input);
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

file_descriptor in();
file_descriptor out();
file_descriptor err();

void link(file_descriptor& fd1, file_descriptor& fd2);
} // namespace subprocess