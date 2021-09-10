#include <stdexcept>
#include <string>

namespace subprocess
{
    class subprocess_error : public std::runtime_error {
        public:
            explicit subprocess_error(const std::string& str) : runtime_error(str) {}
            explicit subprocess_error(const char* str)   : runtime_error(str) {}
    };

    class os_error : public subprocess_error {
        public:
            explicit os_error(const std::string& str) : subprocess_error(str) {}
            explicit os_error(const char* str)   : subprocess_error(str) {}
    };

    class usage_error : public subprocess_error {
        public:
            explicit usage_error(const std::string& str) : subprocess_error(str) {}
            explicit usage_error(const char* str)   : subprocess_error(str) {}
    };

    class command_error : public subprocess_error {
        int return_code_;

        public:

        explicit command_error(const std::string& str, int ret_code) : subprocess_error(str), return_code_{ret_code} {}
        explicit command_error(const char* str, int ret_code)   : subprocess_error(str), return_code_{ret_code} {}

        int return_code()
        {
            return return_code_;
        }
    };
}