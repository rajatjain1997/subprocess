#pragma once

#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <initializer_list>
#include <vector>
#include "util.h"

namespace subprocess
{

    class popen
    {

    public:
        popen(std::initializer_list<const char *> cmd)
        {
            cmd_.reserve(cmd.size() + 1);
            std::copy(cmd.begin(), cmd.end(), std::back_inserter(cmd_));
            cmd_.push_back(NULL);
        }

        friend class command;

    private:
        std::vector<const char *> cmd_;

        int stdin_fd{STDIN_FILENO}, stdout_fd{STDOUT_FILENO}, stderr_fd{STDERR_FILENO};

        int execute(int max_fd = util::kMinFD)
        {
            if (int pid{::fork()}; pid < 0)
            {
                throw std::exception{};
            }
            else if (pid == 0)
            {
                ::dup2(stdin_fd, STDIN_FILENO);
                ::dup2(stdout_fd, STDOUT_FILENO);
                ::dup2(stderr_fd, STDERR_FILENO);
                for (int i{0}; i <= max_fd; i++)
                {
                    util::safe_close_fd(i);
                }
                ::execvp(cmd_[0], const_cast<char *const *>(&cmd_[0]));
                return 0;
            }
            else
            {
                util::safe_close_fd(stdout_fd);
                util::safe_close_fd(stderr_fd);
                util::safe_close_fd(stdin_fd);
                return pid;
            }
        }

        void set_stdin(int fd)
        {
            stdin_fd = fd;
        }

        void set_stdout(int fd)
        {
            stdout_fd = fd;
        }

        void set_stderr(int fd)
        {
            stderr_fd = fd;
        }
    };
}