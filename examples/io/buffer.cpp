#include <vix/io/Io.hpp>

int main()
{
  vix::io::Buffer buffer;

  buffer.append("Hello ");
  buffer.append("Buffer");

  auto out = vix::io::stdout_stream();

  vix::io::write_line(out, buffer.to_string());

  return 0;
}
