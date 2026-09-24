#include <vix/print.hpp>

int main()
{
  std::vector<int> numbers{1, 2, 3, 4, 5};
  std::map<std::string, int> scores{
      {"alice", 10},
      {"bob", 20},
      {"charlie", 30}};

  std::vector<std::map<std::string, int>> nested{
      {{"a", 1}, {"b", 2}},
      {{"x", 10}, {"y", 20}}};

  vix::print("numbers:", numbers);
  vix::print("scores:", scores);

  vix::print(
      vix::options{.sep = " | "},
      "numbers", numbers,
      "scores", scores);

  vix::print(
      vix::options{.compact = false},
      nested);

  vix::print(
      vix::options{
          .sep = " -> ",
          .compact = false},
      "nested containers", nested);

  return 0;
}
