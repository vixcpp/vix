#include <vix/print.hpp>

#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

int main()
{
  vix::print_header("Sequential containers");
  vix::print(std::vector<int>{1, 2, 3, 4, 5});
  vix::print(std::deque<std::string>{"alpha", "beta", "gamma"});
  vix::print(std::list<double>{1.5, 2.5, 3.5});
  vix::print(std::forward_list<int>{9, 8, 7});

  vix::print();
  vix::print_header("Associative containers");
  vix::print(std::set<int>{10, 20, 30});
  vix::print(std::multiset<int>{1, 1, 2, 3});
  vix::print(std::unordered_set<int>{7, 8, 9});
  vix::print(std::map<std::string, int>{{"one", 1}, {"two", 2}});
  vix::print(std::unordered_map<std::string, double>{{"pi", 3.14159}, {"e", 2.71828}});

  vix::print();
  vix::print_header("Container adapters");

  std::stack<int> st;
  st.push(1);
  st.push(2);
  st.push(3);
  vix::print(st);

  std::queue<std::string> q;
  q.push("first");
  q.push("second");
  q.push("third");
  vix::print(q);

  std::priority_queue<int> pq;
  pq.push(5);
  pq.push(1);
  pq.push(9);
  pq.push(3);
  vix::print(pq);

  vix::print();
  vix::print_header("Nested containers");

  const std::vector<std::map<std::string, int>> nested = {
      {{"a", 1}, {"b", 2}},
      {{"x", 10}, {"y", 20}}};
  vix::print(nested);

  return 0;
}
