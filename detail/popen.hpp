#pragma once

#include <initializer_list>
#include <memory>

namespace subprocess
{

    class popen
    {

    public:
        popen(std::initializer_list<const char *> cmd);
        popen(const popen& other);
        popen(popen&& other);
        popen& operator=(const popen& other);
        popen& operator=(popen&& other);

        ~popen();

        int execute(int max_fd);

        int& in();

        int& out();

        int& err();

    private:
        struct PrivateImpl;
        std::unique_ptr<PrivateImpl> pimpl;
    };
}