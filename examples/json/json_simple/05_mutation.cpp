#include <vix/json/Simple.hpp>
#include <iostream>

using namespace vix::json;

int main()
{
  token root;

  // Transforme dynamiquement en objet
  kvs &obj = root.ensure_object();
  obj.set_string("mode", "dev");

  // Puis en array
  array_t &logs = obj.ensure_array("logs");
  logs.push_string("boot");
  logs.push_string("init");
  logs.push_string("ready");

  logs.erase_at(1);

  for (const auto &t : logs)
    std::cout << t.as_string_or("") << "\n";

  return 0;
}
