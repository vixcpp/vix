#include <vix/print.hpp>

#include <map>
#include <string>
#include <vector>

int main()
{
  const std::vector<int> values{4, 8, 15, 16, 23, 42};
  const std::map<std::string, int> scores{
      {"alice", 91},
      {"bob", 77},
      {"charlie", 88}};

  vix::print_header("Named / inline / sprint");
  vix::print_named("values", values);
  vix::print_inline("inline:", 1, 2, 3);
  vix::print();

  const std::string text = vix::sprint("scores = ", scores);
  vix::print_named("sprint result", text);

  vix::print();
  vix::print_header("print_each / print_table");
  vix::print_each(values, true);
  vix::print_table(scores);

  vix::print();
  vix::print_header("stats / summary");
  vix::print_stats(values, "values");
  vix::print_summary(values, 2, 2);

  vix::print();
  vix::print_header("where / map_fn");
  vix::print_where(values, [](int v)
                   { return v % 2 == 0; });
  vix::print_map_fn(values, [](int v)
                    { return v * v; });

  vix::print();
  vix::print_header("sorted by value");
  vix::print_sorted_by_value(scores);

  vix::print();
  vix::print_header("boxed");
  vix::print_boxed("Operation completed successfully");
  vix::print_boxed_value("best score", 91);

  return 0;
}
