#include <string>
#include <subprocess/detail/shell_expander.hpp>
#include <wordexp.h>

namespace subprocess
{

struct shell_expander::PrivateImpl
{
  ::wordexp_t parsed_args;

  PrivateImpl(const std::string& s) { ::wordexp(s.c_str(), &parsed_args, 0); }
  ~PrivateImpl() { ::wordfree(&parsed_args); }
};

shell_expander::shell_expander(const std::string& s) : pimpl{std::make_unique<shell_expander::PrivateImpl>(s)}
{
}
shell_expander::~shell_expander() {}

char** shell_expander::argv() { return pimpl->parsed_args.we_wordv; }
} // namespace subprocess
