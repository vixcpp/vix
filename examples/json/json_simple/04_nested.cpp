#include <vix/json/Simple.hpp>
#include <iostream>

using namespace vix::json;

int main()
{
  kvs root;

  kvs &user = root.ensure_object("user");
  user.set_string("name", "Gaspard");
  user.set_int("id", 42);

  array_t &skills = user.ensure_array("skills");
  skills.push_string("C++");
  skills.push_string("P2P");
  skills.push_string("Systems");

  std::cout << user.get_string_or("name", "") << "\n";
  std::cout << skills[0].as_string_or("") << "\n";

  return 0;
}
