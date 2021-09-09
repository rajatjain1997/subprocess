#pragma once

#include <filesystem>
#include <tuple>

namespace subprocess::util
{
    static constexpr int kMinFD{STDERR_FILENO + 1};

    void safe_close_fd(int fd);

    int safe_open_file(const std::filesystem::path &file_name, int flags);

    const std::tuple<int, int> safe_create_pipe();
}