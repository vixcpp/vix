#include <vix/console.hpp>
#include <string>
#include <string_view>

enum class Status
{
  Idle = 0,
  Running = 1,
  Failed = 2
};

int main()
{
  const char *raw = "raw c-string";
  std::string text = "std::string";
  std::string_view view = "std::string_view";

  int number = 42;
  double price = 19.99;
  bool ok = true;
  Status status = Status::Running;

  int value = 10;
  int *ptr = &value;

  vix::console.log("raw:", raw);
  vix::console.log("text:", text);
  vix::console.log("view:", view);
  vix::console.log("number:", number);
  vix::console.log("price:", price);
  vix::console.log("ok:", ok);
  vix::console.log("status:", status);
  vix::console.log("pointer:", ptr);

  return 0;
}
