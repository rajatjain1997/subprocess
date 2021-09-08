#include <fcntl.h>
#include <string>
#include <tuple>
#include <algorithm>
#include <deque>
#include <initializer_list>
#include <filesystem>
#include <optional>
#include <functional>
#include "detail/popen.h"

namespace subprocess
{

    class command
    {
    public:
        command(std::initializer_list<const char *> cmd)
        {
            processes.push_back(subprocess::popen{std::move(cmd)});
        }

        int run()
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
            for (auto &process : processes)
            {
                pids.push_back(process.execute(max_fd));
            }
            capture_stream(captured_stdout);
            capture_stream(captured_stderr);
            for (auto pid : pids)
            {
                ::waitpid(pid, &waitstatus, 0);
            }
            return WEXITSTATUS(waitstatus);
        }

        command &operator|(command other)
        {
            const auto &[read_fd, write_fd] = create_pipe_();
            other.processes.front().set_stdin(read_fd);
            processes.back().set_stdout(write_fd);
            std::move(other.processes.begin(), other.processes.end(), std::back_inserter(processes));
            set_max_fd(other.max_fd);
            return *this;
        }

        command &operator>(int fd)
        {
            captured_stdout.reset();
            processes.back().set_stdout(fd);
            set_max_fd(fd);
            return *this;
        }

        command &operator>=(int fd)
        {
            processes.back().set_stderr(fd);
            set_max_fd(fd);
            return *this;
        }

        command &operator>>(int fd)
        {
            return (*this > fd);
        }

        command &operator>>=(int fd)
        {
            return (*this >= fd);
        }

        command &operator<(int fd)
        {
            processes.front().set_stdin(fd);
            set_max_fd(fd);
            return *this;
        }

        command &operator>=(std::string &output)
        {
            const auto &[read_fd, write_fd] = create_pipe_();
            *this > write_fd;
            captured_stderr = {read_fd, output};
            return *this;
        }

        command &operator>(std::string &output)
        {
            const auto &[read_fd, write_fd] = create_pipe_();
            *this > write_fd;
            captured_stdout = {read_fd, output};
            return *this;
        }

        command &operator<(std::string &input)
        {
            const auto &[read_fd, write_fd] = create_pipe_();
            *this < read_fd;
            ::write(write_fd, input.c_str(), input.size());
            return *this;
        }

        command &operator>(const std::filesystem::path &file_name)
        {
            return *this > util::safe_open_file(file_name, O_WRONLY | O_CREAT | O_TRUNC);
        }

        command &operator>=(const std::filesystem::path &file_name)
        {
            return *this >= util::safe_open_file(file_name, O_WRONLY | O_CREAT | O_TRUNC);
        }

        command &operator>>(const std::filesystem::path &file_name)
        {
            return *this >> util::safe_open_file(file_name, O_WRONLY | O_CREAT | O_APPEND);
        }

        command &operator>>=(const std::filesystem::path &file_name)
        {
            return *this >>= util::safe_open_file(file_name, O_WRONLY | O_CREAT | O_APPEND);
        }

        command &operator<(std::filesystem::path file_name)
        {
            return *this < util::safe_open_file(file_name, O_RDONLY);
        }

    private:
        std::deque<subprocess::popen> processes;
        int max_fd{util::kMinFD};
        std::optional<std::pair<int, std::reference_wrapper<std::string>>> captured_stdout, captured_stderr;

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
}