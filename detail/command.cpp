#include <fcntl.h>
#include <unistd.h>
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
#include "util.hpp"
#include "command.hpp"

namespace subprocess
{
    struct command::PrivateImpl
    {

        std::deque<subprocess::popen> processes;
        int max_fd{util::kMinFD};
        std::optional<std::pair<int, std::reference_wrapper<std::string>>> captured_stdout, captured_stderr;

        PrivateImpl() {}
        PrivateImpl(std::initializer_list<const char *> cmd)
        {
            processes.push_back(subprocess::popen{std::move(cmd)});
        }

        void set_max_fd(int fd)
        {
            max_fd = std::max(max_fd, fd);
        }

        std::tuple<int, int> create_pipe_()
        {
            const auto &[read_fd, write_fd] = util::safe_create_pipe();
            set_max_fd(read_fd);
            set_max_fd(write_fd);
            return {read_fd, write_fd};
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
                optional_stream_pair->second.get().clear();
                while (::read(optional_stream_pair->first, buf, 2048) > 0)
                {
                    optional_stream_pair->second.get().append(buf);
                }
            }
        };
        pids.clear();
        int pid, waitstatus;
        for (auto &process : pimpl->processes)
        {
            pids.push_back(process.execute(pimpl->max_fd));
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
        const auto &[read_fd, write_fd] = pimpl->create_pipe_();
        other.pimpl->processes.front().in() = read_fd;
        pimpl->processes.back().out() = write_fd;
        std::move(other.pimpl->processes.begin(), other.pimpl->processes.end(), std::back_inserter(pimpl->processes));
        pimpl->set_max_fd(other.pimpl->max_fd);
        return *this;
    }

    command &command::operator>(int fd)
    {
        pimpl->captured_stdout.reset();
        pimpl->processes.back().out() = fd;
        pimpl->set_max_fd(fd);
        return *this;
    }

    command &command::operator>=(int fd)
    {
        pimpl->processes.back().err() = fd;
        pimpl->set_max_fd(fd);
        return *this;
    }

    command &command::operator>>(int fd)
    {
        return (*this > fd);
    }

    command &command::operator>>=(int fd)
    {
        return (*this >= fd);
    }

    command &command::operator<(int fd)
    {
        pimpl->processes.front().in() = fd;
        pimpl->set_max_fd(fd);
        return *this;
    }

    command &command::operator>=(std::string &output)
    {
        const auto &[read_fd, write_fd] = pimpl->create_pipe_();
        *this > write_fd;
        pimpl->captured_stderr = {read_fd, output};
        return *this;
    }

    command &command::operator>(std::string &output)
    {
        const auto &[read_fd, write_fd] = pimpl->create_pipe_();
        *this > write_fd;
        pimpl->captured_stdout = {read_fd, output};
        return *this;
    }

    command &command::operator<(std::string &input)
    {
        const auto &[read_fd, write_fd] = pimpl->create_pipe_();
        *this < read_fd;
        ::write(write_fd, input.c_str(), input.size());
        return *this;
    }

    command &command::operator>(const std::filesystem::path &file_name)
    {
        return *this > util::safe_open_file(file_name, O_WRONLY | O_CREAT | O_TRUNC);
    }

    command &command::operator>=(const std::filesystem::path &file_name)
    {
        return *this >= util::safe_open_file(file_name, O_WRONLY | O_CREAT | O_TRUNC);
    }

    command &command::operator>>(const std::filesystem::path &file_name)
    {
        return *this >> util::safe_open_file(file_name, O_WRONLY | O_CREAT | O_APPEND);
    }

    command &command::operator>>=(const std::filesystem::path &file_name)
    {
        return *this >>= util::safe_open_file(file_name, O_WRONLY | O_CREAT | O_APPEND);
    }

    command &command::operator<(std::filesystem::path file_name)
    {
        return *this < util::safe_open_file(file_name, O_RDONLY);
    }
}