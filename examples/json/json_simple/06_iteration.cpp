#include <vix/json/Simple.hpp>
#include <iostream>

using namespace vix::json;

int main()
{
  kvs cfg = obj({"host", "localhost",
                 "port", 8080,
                 "secure", false});

  cfg.for_each_pair([](std::string_view key, const token &value)
                    {
    std::cout << key << " -> ";
    if (value.is_string())
      std::cout << value.as_string_or("");
    else if (value.is_i64())
      std::cout << value.as_i64_or(0);
    else if (value.is_bool())
      std::cout << value.as_bool_or(false);
    std::cout << "\n"; });

  return 0;
}
