#include <memory>

namespace subprocess
{
class shell_expander
{
public:
  shell_expander(const std::string& s);
  ~shell_expander();

  char** argv();

private:
  struct PrivateImpl;
  std::unique_ptr<PrivateImpl> pimpl;
};

} // namespace subprocess