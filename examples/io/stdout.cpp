#include <vix/io/Io.hpp>

int main()
{
  auto out = vix::io::stdout_stream();

  vix::io::write(out, "Hello ");
  vix::io::write_line(out, "world");

  vix::io::Bytes bytes{65, 66, 67};
  vix::io::write_line(out, "\nBytes:");
  vix::io::write(out, bytes);

  return 0;
}
