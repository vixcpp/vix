#include <vix/json/Simple.hpp>
#include <iostream>

using namespace vix::json;

int main()
{
  kvs user;

  user.set_string("name", "Alice");
  user.set_int("age", 30);
  user.set_bool("active", true);

  std::cout << user.get_string_or("name", "unknown") << "\n";
  std::cout << user.get_i64_or("age", 0) << "\n";
  std::cout << user.get_bool_or("active", false) << "\n";

  return 0;
}
