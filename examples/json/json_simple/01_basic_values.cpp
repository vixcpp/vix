#include <vix/json/Simple.hpp>
#include <iostream>

using namespace vix::json;

int main()
{
  token t1;           // null
  token t2 = true;    // bool
  token t3 = 42;      // int -> int64
  token t4 = 3.14;    // double
  token t5 = "hello"; // string

  std::cout << "is_null=" << t1.is_null() << "\n";
  std::cout << "bool=" << t2.as_bool_or(false) << "\n";
  std::cout << "int64=" << t3.as_i64_or(-1) << "\n";
  std::cout << "double=" << t4.as_f64_or(0.0) << "\n";
  std::cout << "string=" << t5.as_string_or("empty") << "\n";

  return 0;
}
