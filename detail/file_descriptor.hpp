#pragma once

#include <optional>
#include <tuple>
#include <filesystem>
#include <memory>

namespace subprocess
{
    class file_descriptor
    {
    public:
        static file_descriptor open(std::filesystem::path file_name, int flags);
        static std::pair<file_descriptor, file_descriptor> create_pipe();

        file_descriptor(int fd = -1, std::optional<std::filesystem::path> file_path = {}) : fd_{std::make_shared<int>(fd)}, file_path_{file_path} {}
        file_descriptor(const file_descriptor &other) = default;
        file_descriptor &operator=(const file_descriptor &other) = default;
        file_descriptor(file_descriptor &&fd) noexcept;
        file_descriptor &operator=(file_descriptor &&other) noexcept;
        ~file_descriptor();

        int fd() const
        {
            return *fd_;
        }

        bool linked() const
        {
            return linked_fd_ != nullptr;
        }

        operator int() const
        {
            return fd();
        }

        void close();
        void close_linked();
        void dup(const file_descriptor &other);
        void write(std::string &input);
        std::string read();

    private:
        static const int kMinFD_;
        std::optional<std::filesystem::path> file_path_;
        std::shared_ptr<int> fd_;
        file_descriptor *linked_fd_{nullptr};
        friend void link(file_descriptor &fd1, file_descriptor &fd2);
    };

    extern const file_descriptor in;
    extern const file_descriptor out;
    extern const file_descriptor err;

    void link(file_descriptor &fd1, file_descriptor &fd2);
}