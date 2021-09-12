#include <stdexcept>
#include <subprocess/subprocess_export.h>

namespace subprocess
{

/**
 * @brief A catch-all class for all errors thrown by subprocess.
 *
 * All exceptions in the library derive from subprocess_error.
 * An exception of type subprocess_error is never actually thrown.
 *
 */
class SUBPROCESS_EXPORT subprocess_error : public std::runtime_error
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
class SUBPROCESS_EXPORT os_error : public subprocess_error
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
class SUBPROCESS_EXPORT usage_error : public subprocess_error
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

class SUBPROCESS_EXPORT command_error : public subprocess_error
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
} // namespace subprocess