#pragma once

#include "file_descriptor.hpp"
#include <initializer_list>
#include <memory>
#include <subprocess/subprocess_export.h>

namespace subprocess
{

class popen
{

public:
  popen(std::initializer_list<const char*> cmd);
  popen(const popen& other) = delete;
  popen(popen&& other);
  popen& operator=(const popen& other) = delete;
  popen& operator                      =(popen&& other);

  ~popen();

  void execute();

  int wait();

  file_descriptor& in();

  file_descriptor& out();

  file_descriptor& err();

private:
  struct PrivateImpl;
  std::unique_ptr<PrivateImpl> pimpl;
};
} // namespace subprocess