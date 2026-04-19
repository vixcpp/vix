#include <vix/io/Io.hpp>

int main()
{
  auto in = vix::io::stdin_stream();
  auto out = vix::io::stdout_stream();

  vix::io::write_line(out, "Enter lines (Ctrl+D to finish):");

  while (true)
  {
    auto line = vix::io::read_line(in);

    if (!line)
    {
      break;
    }

    if (line.value().empty())
    {
      break;
    }

    vix::io::write(out, ">> ");
    vix::io::write_line(out, line.value());
  }

  return 0;
}
