#include "fcntl.h"
#include <string>
#include <tuple>
#include <algorithm>
#include <deque>
#include <vector>
#include <initializer_list>
#include <filesystem>
#include <optional>
#include <functional>
#include "popen.hpp"
#include "file_descriptor.hpp"
#include "command.hpp"

namespace subprocess
{
    struct command::PrivateImpl
    {

        std::deque<subprocess::popen> processes;
        std::optional<std::pair<file_descriptor, std::reference_wrapper<std::string>>> captured_stdout, captured_stderr;
        std::deque<file_descriptor> descriptors_to_close;

        PrivateImpl() {}
        PrivateImpl(std::initializer_list<const char *> cmd)
        {
            processes.push_back(subprocess::popen{std::move(cmd)});
        }
    };

    command::command(std::initializer_list<const char *> cmd) : pimpl{std::make_unique<PrivateImpl>(std::move(cmd))}
    {
    }

    command::command(const command &other) : pimpl(std::make_unique<PrivateImpl>())
    {
        *pimpl = *(other.pimpl);
    };

    command::command(command &&other)
    {
        pimpl.reset();
        pimpl.swap(other.pimpl);
    };
    command &command::operator=(const command &other)
    {
        *pimpl = *(other.pimpl);
        return *this;
    };
    command &command::operator=(command &&other)
    {
        pimpl.swap(other.pimpl);
        return *this;
    };

    command::~command() {}

    int command::run()
    {
        static char buf[2048];
        static std::vector<int> pids;
        auto capture_stream = [](auto &optional_stream_pair)
        {
            if (optional_stream_pair)
            {
                std::string &output{optional_stream_pair->second.get()};
                output.clear();
                output = std::move(optional_stream_pair->first.read());
                optional_stream_pair->first.close();
            }
        };
        pids.clear();
        int pid, waitstatus;
        for (auto &process : pimpl->processes)
        {
            pids.push_back(process.execute());
        }
        capture_stream(pimpl->captured_stdout);
        capture_stream(pimpl->captured_stderr);
        for (auto pid : pids)
        {
            ::waitpid(pid, &waitstatus, 0);
        }
        return WEXITSTATUS(waitstatus);
    }

    command &command::operator|(command other)
    {
        auto [read_fd, write_fd] = file_descriptor::create_pipe();
        other.pimpl->processes.front().in() = read_fd;
        pimpl->processes.back().out() = write_fd;
        std::move(other.pimpl->processes.begin(), other.pimpl->processes.end(), std::back_inserter(pimpl->processes));
        pimpl->captured_stdout = std::move(other.pimpl->captured_stdout);
        pimpl->captured_stderr = std::move(other.pimpl->captured_stderr);
        return *this;
    }

    command &command::operator>(file_descriptor fd)
    {
        pimpl->captured_stdout.reset();
        pimpl->processes.back().out() = std::move(fd);
        return *this;
    }

    command &command::operator>=(file_descriptor fd)
    {
        pimpl->processes.back().err() = std::move(fd);
        return *this;
    }

    command &command::operator>>(file_descriptor fd)
    {
        return (*this > std::move(fd));
    }

    command &command::operator>>=(file_descriptor fd)
    {
        return (*this >= std::move(fd));
    }

    command &command::operator<(file_descriptor fd)
    {
        pimpl->processes.front().in() = std::move(fd);
        return *this;
    }

    command &command::operator>=(std::string &output)
    {
        auto [read_fd, write_fd] = file_descriptor::create_pipe();
        *this > write_fd;
        pimpl->captured_stderr = {read_fd, output};
        return *this;
    }

    command &command::operator>(std::string &output)
    {
        auto [read_fd, write_fd] = file_descriptor::create_pipe();
        *this > write_fd;
        pimpl->captured_stdout = {read_fd, output};
        return *this;
    }

    command &command::operator<(std::string &input)
    {
        auto [read_fd, write_fd] = file_descriptor::create_pipe();
        *this < std::move(read_fd);
        write_fd.write(input);
        write_fd.close();
        return *this;
    }

    command &command::operator>(const std::filesystem::path &file_name)
    {
        return *this > file_descriptor::open(file_name, O_WRONLY | O_CREAT | O_TRUNC);
    }

    command &command::operator>=(const std::filesystem::path &file_name)
    {
        return *this >= file_descriptor::open(file_name, O_WRONLY | O_CREAT | O_TRUNC);
    }

    command &command::operator>>(const std::filesystem::path &file_name)
    {
        return *this >> file_descriptor::open(file_name, O_WRONLY | O_CREAT | O_APPEND);
    }

    command &command::operator>>=(const std::filesystem::path &file_name)
    {
        return *this >>= file_descriptor::open(file_name, O_WRONLY | O_CREAT | O_APPEND);
    }

    command &command::operator<(std::filesystem::path file_name)
    {
        return *this < file_descriptor::open(file_name, O_RDONLY);
    }
}