#pragma once

#include <filesystem>
#include <initializer_list>
#include <memory>
#include <new>
#include <string>
#include <subprocess/detail/file_descriptor.hpp>
#include <subprocess/subprocess_export.h>

namespace subprocess
{

/**
 * @brief Main interface class for subprocess library
 *
 * A class that contains a list of linked shell commands and is responsible
 * for managing their file descriptors such that their input and output can
 * be chained to other commands.
 *
 */
class SUBPROCESS_EXPORT command
{
public:
  command(std::initializer_list<const char*> cmd);

  command(const command& other) = delete;
  command(command&& other);
  command& operator=(const command& other) = delete;
  command& operator                        =(command&& other);

  ~command();


  /**
   * @brief Runs the command pipeline and throws on error.
   *
   * run only returns when the return code from the pipeline is 0.
   * Otherwise, it throws subprocess::command_error.
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
   * It doesn't throw subprocess::command_error.
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

private:
  struct SUBPROCESS_NO_EXPORT PrivateImpl;
  std::unique_ptr<PrivateImpl> pimpl;

  friend command& operator<(command& cmd, file_descriptor fd);
  friend command&& operator<(command&& cmd, file_descriptor fd);
  friend command& operator<(command& cmd, std::string& input);
  friend command&& operator<(command&& cmd, std::string& input);
  friend command& operator<(command& cmd, std::filesystem::path file_name);
  friend command&& operator<(command&& cmd, std::filesystem::path file_name);

  friend command& operator>(command& cmd, file_descriptor fd);
  friend command&& operator>(command&& cmd, file_descriptor fd);
  friend command& operator>(command& cmd, std::string& output);
  friend command&& operator>(command&& cmd, std::string& output);
  friend command& operator>(command& cmd, const std::filesystem::path& file_name);
  friend command&& operator>(command&& cmd, const std::filesystem::path& file_name);

  friend command& operator>=(command& cmd, file_descriptor fd);
  friend command&& operator>=(command&& cmd, file_descriptor fd);
  friend command& operator>=(command& cmd, std::string& output);
  friend command&& operator>=(command&& cmd, std::string& output);
  friend command& operator>=(command& cmd, const std::filesystem::path& file_name);
  friend command&& operator>=(command&& cmd, const std::filesystem::path& file_name);

  friend command& operator>>(command& cmd, file_descriptor fd);
  friend command&& operator>>(command&& cmd, file_descriptor fd);
  friend command& operator>>(command& cmd, const std::filesystem::path& file_name);
  friend command&& operator>>(command&& cmd, const std::filesystem::path& file_name);

  friend command& operator>>=(command& cmd, file_descriptor fd);
  friend command&& operator>>=(command&& cmd, file_descriptor fd);
  friend command& operator>>=(command& cmd, const std::filesystem::path& file_name);
  friend command&& operator>>=(command&& cmd, const std::filesystem::path& file_name);
};

SUBPROCESS_EXPORT command& operator<(command& cmd, file_descriptor fd);
SUBPROCESS_EXPORT command&& operator<(command&& cmd, file_descriptor fd);
SUBPROCESS_EXPORT command& operator<(command& cmd, std::string& input);
SUBPROCESS_EXPORT command&& operator<(command&& cmd, std::string& input);
SUBPROCESS_EXPORT command& operator<(command& cmd, std::filesystem::path file_name);
SUBPROCESS_EXPORT command&& operator<(command&& cmd, std::filesystem::path file_name);

SUBPROCESS_EXPORT command& operator>(command& cmd, file_descriptor fd);
SUBPROCESS_EXPORT command&& operator>(command&& cmd, file_descriptor fd);
SUBPROCESS_EXPORT command& operator>(command& cmd, std::string& output);
SUBPROCESS_EXPORT command&& operator>(command&& cmd, std::string& output);
SUBPROCESS_EXPORT command& operator>(command& cmd, const std::filesystem::path& file_name);
SUBPROCESS_EXPORT command&& operator>(command&& cmd, const std::filesystem::path& file_name);

SUBPROCESS_EXPORT command& operator>=(command& cmd, file_descriptor fd);
SUBPROCESS_EXPORT command&& operator>=(command&& cmd, file_descriptor fd);
SUBPROCESS_EXPORT command& operator>=(command& cmd, std::string& output);
SUBPROCESS_EXPORT command&& operator>=(command&& cmd, std::string& output);
SUBPROCESS_EXPORT command& operator>=(command& cmd, const std::filesystem::path& file_name);
SUBPROCESS_EXPORT command&& operator>=(command&& cmd, const std::filesystem::path& file_name);

SUBPROCESS_EXPORT command& operator>>(command& cmd, file_descriptor fd);
SUBPROCESS_EXPORT command&& operator>>(command&& cmd, file_descriptor fd);
SUBPROCESS_EXPORT command& operator>>(command& cmd, const std::filesystem::path& file_name);
SUBPROCESS_EXPORT command&& operator>>(command&& cmd, const std::filesystem::path& file_name);

SUBPROCESS_EXPORT command& operator>>=(command& cmd, file_descriptor fd);
SUBPROCESS_EXPORT command&& operator>>=(command&& cmd, file_descriptor fd);
SUBPROCESS_EXPORT command& operator>>=(command& cmd, const std::filesystem::path& file_name);
SUBPROCESS_EXPORT command&& operator>>=(command&& cmd, const std::filesystem::path& file_name);

} // namespace subprocess