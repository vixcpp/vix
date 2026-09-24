#include <vix/inspect.hpp>

#include <array>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

int main()
{
  vix::inspect(std::array<int, 4>{10, 20, 30, 40});
  vix::inspect(std::deque<std::string>{"red", "green", "blue"});
  vix::inspect(std::list<int>{7, 8, 9});
  vix::inspect(std::forward_list<int>{100, 200, 300});

  vix::inspect(std::set<int>{5, 1, 9, 2});
  vix::inspect(std::map<std::string, int>{
      {"apple", 3},
      {"banana", 7},
      {"orange", 5},
  });

  vix::inspect(std::unordered_map<std::string, double>{
      {"pi", 3.14159},
      {"e", 2.71828},
  });

  std::stack<int> st;
  st.push(1);
  st.push(2);
  st.push(3);
  vix::inspect(st);

  std::queue<std::string> q;
  q.push("first");
  q.push("second");
  q.push("third");
  vix::inspect(q);

  std::priority_queue<int> pq;
  pq.push(10);
  pq.push(5);
  pq.push(30);
  pq.push(20);
  vix::inspect(pq);

  std::vector<std::map<std::string, int>> nested = {
      {{"a", 1}, {"b", 2}},
      {{"x", 10}, {"y", 20}},
  };
  vix::inspect(nested);

  return 0;
}
