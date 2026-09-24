#include <vix/json/Simple.hpp>
#include <iostream>

using namespace vix::json;

int main()
{
  array_t arr;

  arr.push_int(1);
  arr.push_int(2);
  arr.push_string("three");
  arr.push_bool(true);

  arr.reserve(10);
  arr.ensure(6).set_string("auto-filled");

  for (std::size_t i = 0; i < arr.size(); ++i)
  {
    if (arr[i].is_string())
      std::cout << "string: " << arr[i].as_string_or("") << "\n";
    else if (arr[i].is_i64())
      std::cout << "int: " << arr[i].as_i64_or(0) << "\n";
  }

  return 0;
}
