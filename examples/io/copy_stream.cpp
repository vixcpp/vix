#include <vix/io/Io.hpp>

int main()
{
  auto in = vix::io::stdin_stream();
  auto out = vix::io::stdout_stream();

  auto result = vix::io::copy(in, out);

  if (!result)
  {
    auto err = vix::io::stderr_stream();
    vix::io::write_line(err, "copy failed");
    return 1;
  }

  return 0;
}
