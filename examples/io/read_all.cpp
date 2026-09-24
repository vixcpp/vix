#include <vix/io/Io.hpp>

int main()
{
  auto in = vix::io::stdin_stream();
  auto out = vix::io::stdout_stream();

  auto data = vix::io::read_all(in);

  if (data)
  {
    vix::io::write(out, data.value());
  }
  else
  {
    auto err = vix::io::stderr_stream();
    vix::io::write_line(err, "read_all failed");
  }

  return 0;
}
