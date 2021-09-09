#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <initializer_list>
#include <vector>
#include "util.hpp"
#include "popen.hpp"

namespace subprocess
{

    struct popen::PrivateImpl
    {
        std::vector<const char *> cmd_;
        int stdin_fd{STDIN_FILENO}, stdout_fd{STDOUT_FILENO}, stderr_fd{STDERR_FILENO};

        PrivateImpl()
        {
        }

        PrivateImpl(std::initializer_list<const char *> cmd)
        {
            cmd_.reserve(cmd.size() + 1);
            std::copy(cmd.begin(), cmd.end(), std::back_inserter(cmd_));
            cmd_.push_back(NULL);
        }
    };

    popen::popen(std::initializer_list<const char *> cmd) : pimpl{std::make_unique<PrivateImpl>(std::move(cmd))}
    {
    }

    popen::popen(const popen &other) : pimpl(std::make_unique<PrivateImpl>())
    {
        *pimpl = *(other.pimpl);
    };

    popen::popen(popen &&other)
    {
        pimpl.reset();
        pimpl.swap(other.pimpl);
    };
    popen &popen::operator=(const popen &other)
    {
        *pimpl = *(other.pimpl);
        return *this;
    };
    popen &popen::operator=(popen &&other)
    {
        pimpl.swap(other.pimpl);
        return *this;
    };

    popen::~popen() {}

    int popen::execute(int max_fd)
    {
        if (int pid{::fork()}; pid < 0)
        {
            throw std::exception{};
        }
        else if (pid == 0)
        {
            ::dup2(pimpl->stdin_fd, STDIN_FILENO);
            ::dup2(pimpl->stdout_fd, STDOUT_FILENO);
            ::dup2(pimpl->stderr_fd, STDERR_FILENO);
            for (int i{0}; i <= max_fd; i++)
            {
                util::safe_close_fd(i);
            }
            ::execvp(pimpl->cmd_[0], const_cast<char *const *>(&(pimpl->cmd_[0])));
            return 0;
        }
        else
        {
            util::safe_close_fd(pimpl->stdout_fd);
            util::safe_close_fd(pimpl->stderr_fd);
            util::safe_close_fd(pimpl->stdin_fd);
            return pid;
        }
    }

    int &popen::in()
    {
        return pimpl->stdin_fd;
    }

    int &popen::out()
    {
        return pimpl->stdout_fd;
    }

    int &popen::err()
    {
        return pimpl->stderr_fd;
    }
}