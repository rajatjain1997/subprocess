#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <filesystem>
#include <tuple>
#include "util.hpp"

namespace subprocess::util
{
    void safe_close_fd(int fd)
    {
        if (fd > kMinFD)
        {
            ::close(fd);
        }
    }

    int safe_open_file(const std::filesystem::path &file_name, int flags)
    {
        if (int fd{::open(file_name.c_str(), flags)}; fd < 0)
        {
            throw std::exception{};
        }
        else
        {
            return fd;
        }
    }

    const std::tuple<int, int> safe_create_pipe()
    {
        int fd[2];
        if (::pipe(fd) < 0)
        {
            throw std::exception{};
        }
        return {fd[0], fd[1]};
    }
}