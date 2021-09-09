#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include "file_descriptor.hpp"
#include <iostream>

namespace subprocess
{

    enum standard_filenos
    {
        standard_in = STDIN_FILENO,
        standard_out = STDOUT_FILENO,
        standard_error = STDERR_FILENO,
        max_standard_fd
    };

    /*static*/ const int file_descriptor::kMinFD_{STDERR_FILENO + 1};

    /*static*/ file_descriptor file_descriptor::open(std::filesystem::path file_name, int flags)
    {
        if (int fd{::open(file_name.c_str(), flags)}; fd < 0)
        {
            throw std::exception{};
        }
        else
        {
            return {fd, std::move(file_name)};
        }
    }

    /*static*/ std::pair<file_descriptor, file_descriptor> file_descriptor::create_pipe()
    {
        int fd[2];
        if (::pipe(fd) < 0)
        {
            throw std::exception{};
        }
        file_descriptor read_fd{fd[0]};
        file_descriptor write_fd{fd[1]};
        link(read_fd, write_fd);
        return {read_fd, write_fd};
    }

    file_descriptor::file_descriptor(file_descriptor &&other) noexcept : file_descriptor()
    {
        *this = std::move(other);
    }

    file_descriptor &file_descriptor::operator=(file_descriptor &&other) noexcept
    {
        std::swap(fd_, other.fd_);
        std::swap(file_path_, other.file_path_);
        std::swap(linked_fd_, other.linked_fd_);
        return *this;
    }

    file_descriptor::~file_descriptor()
    {
        if (file_path_ and fd_.use_count() <= 1)
        {
            close();
        }
    }

    void file_descriptor::close()
    {
        if (fd() > kMinFD_)
        {
            ::close(fd());
            file_path_.reset();
        }
    }

    void file_descriptor::close_linked()
    {
        if (linked_fd_)
        {
            linked_fd_->close();
        }
    }

    void file_descriptor::dup(const file_descriptor &other)
    {
        ::dup2(fd(), other.fd());
    }

    void file_descriptor::write(std::string &input)
    {
        ::write(fd(), input.c_str(), input.size());
    }

    std::string file_descriptor::read()
    {
        static char buf[2048];
        static std::string output;
        output.clear();
        while (::read(fd(), buf, 2048) > 0)
        {
            output.append(buf);
        }
        return output;
    }

    const file_descriptor in{file_descriptor{standard_filenos::standard_in}};
    const file_descriptor out{file_descriptor{standard_filenos::standard_out}};
    const file_descriptor err{file_descriptor{standard_filenos::standard_error}};

    void link(file_descriptor &fd1, file_descriptor &fd2)
    {
        auto link_fds = [](file_descriptor &linking_fd, file_descriptor &linked_fd)
        {
            if (linking_fd.linked_fd_)
            {
                throw std::exception{};
            }
            else
            {
                linking_fd.linked_fd_ = &linked_fd;
            }
        };
        link_fds(fd1, fd2);
        link_fds(fd2, fd1);
    }
}