#include <string>
#include <initializer_list>
#include <filesystem>
#include <memory>

namespace subprocess
{

    class command
    {
    public:
        command(std::initializer_list<const char *> cmd);

        command(const command& other);
        command(command&& other);
        command& operator=(const command& other);
        command& operator=(command&& other);

        ~command();

        int run();

        command &operator|(command other);

        command &operator>(int fd);

        command &operator>=(int fd);

        command &operator>>(int fd);

        command &operator>>=(int fd);

        command &operator<(int fd);

        command &operator>=(std::string &output);

        command &operator>(std::string &output);

        command &operator<(std::string &input);

        command &operator>(const std::filesystem::path &file_name);

        command &operator>=(const std::filesystem::path &file_name);
        command &operator>>(const std::filesystem::path &file_name);

        command &operator>>=(const std::filesystem::path &file_name);

        command &operator<(std::filesystem::path file_name);

    private:
        struct PrivateImpl;
        std::unique_ptr<PrivateImpl> pimpl;
    };
}