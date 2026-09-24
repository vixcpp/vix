#include <iostream>
#include <vix/print.hpp>

namespace showcase
{
  struct Point
  {
    int x;
    int y;
  };

  inline void vix_format(std::ostream &os, const Point &point)
  {
    os << "Point{x=" << point.x << ", y=" << point.y << "}";
  }

  struct User
  {
    std::string name;
    int age;
    bool active;
  };

  enum class Status
  {
    Idle = 0,
    Running = 1,
    Failed = 2
  };

  struct Product
  {
    std::string name;
    double price;
  };
} // namespace showcase

namespace vix
{
  template <>
  struct formatter<showcase::User>
  {
    static void format(std::ostream &os, const showcase::User &user)
    {
      os << "User{name=\"" << user.name
         << "\", age=" << user.age
         << ", active=" << (user.active ? "true" : "false")
         << "}";
    }
  };

  template <>
  struct formatter<showcase::Product>
  {
    static void format(std::ostream &os, const showcase::Product &product)
    {
      os << "Product{name=\"" << product.name
         << "\", price=" << product.price
         << "}";
    }
  };
} // namespace vix

namespace showcase
{
  using namespace std::chrono_literals;

  void print_title(std::string_view title)
  {
    vix::print(vix::options{.end = "\n\n"}, "=== ", title, " ===");
  }

  void demo_basic()
  {
    print_title("basic");

    vix::print("Hello from Vix");
    vix::print("name:", "Gaspard", "age:", 25, "country:", "Uganda");
    vix::print(42);
    vix::print(true, false, nullptr);
    vix::print();
  }

  void demo_blank_lines()
  {
    print_title("blank lines");

    vix::print("line 1");
    vix::print();
    vix::print("line 2");
    vix::print();
  }

  void demo_separator()
  {
    print_title("separator");

    vix::print(
        vix::options{.sep = " | "},
        "red", "green", "blue");

    vix::print(
        vix::options{.sep = " -> "},
        "A", "B", "C", "D");

    vix::print();
  }

  void demo_end()
  {
    print_title("custom end");

    vix::print(
        vix::options{.end = ""},
        "loading...");
    vix::print();

    vix::print(
        vix::options{.end = " <done>\n"},
        "task");

    vix::print(
        vix::options{.end = "\n\n"},
        "double newline");
  }

  void demo_stderr_and_flush()
  {
    print_title("stderr and flush");

    vix::print("normal output");

    vix::print(
        vix::options{
            .file = &std::cerr},
        "error:", "invalid input");

    vix::print(
        vix::options{
            .end = "",
            .flush = true},
        "processing...");
    vix::print();
    vix::print("done");
    vix::print();
  }

  void demo_strings()
  {
    print_title("strings");

    std::string project = "Vix";
    std::string_view mode = "stable";
    const char *raw = "const char* value";
    std::wstring wide = L"wide string";

    vix::print("project:", project, "mode:", mode);
    vix::print(raw);
    vix::print(wide);

    vix::print(
        vix::options{.raw_strings = false},
        "quoted string example");

    vix::print();
  }

  void demo_numbers_and_chars()
  {
    print_title("numbers and chars");

    vix::print(1, 2, 3, 4, 5);
    vix::print(3.14, 2.71828, -17, 0u);
    vix::print('A', 'B', 'C');
    vix::print(42ns, 10us, 15ms, 2s, 3min, 1h);
    vix::print();
  }

  void demo_optional_variant_tuple_pair()
  {
    print_title("optional variant tuple pair");

    std::optional<int> age = 25;
    std::optional<int> missing{};
    std::variant<int, std::string> state = std::string{"active"};
    auto pair = std::make_pair(std::string{"score"}, 98);
    auto tuple = std::make_tuple("tuple", 42, true, 3.14);

    vix::print(age);
    vix::print(missing);
    vix::print(state);
    vix::print(pair);
    vix::print(tuple);
    vix::print();
  }

  void demo_sequence_containers()
  {
    print_title("sequence containers");

    std::vector<int> numbers{1, 2, 3, 4, 5};
    std::vector<std::string> names{"alice", "bob", "charlie"};
    std::deque<std::string> queue_like{"first", "second", "third"};
    std::list<int> linked{10, 20, 30};
    std::forward_list<int> forward{7, 8, 9};

    vix::print(numbers);
    vix::print(names);
    vix::print(queue_like);
    vix::print(linked);
    vix::print(forward);
    vix::print();
  }

  void demo_associative_containers()
  {
    print_title("associative containers");

    std::set<int> ordered{4, 1, 3, 2};
    std::unordered_set<std::string> tags{"fast", "simple", "stable"};
    std::map<std::string, int> scores{
        {"alice", 10},
        {"bob", 20},
        {"charlie", 30}};
    std::unordered_map<std::string, double> prices{
        {"apple", 1.5},
        {"banana", 0.8},
        {"orange", 2.1}};

    vix::print(ordered);
    vix::print(tags);
    vix::print(scores);
    vix::print(prices);
    vix::print();
  }

  void demo_nested_containers()
  {
    print_title("nested containers");

    std::vector<std::map<std::string, int>> nested{
        {{"a", 1}, {"b", 2}},
        {{"x", 10}, {"y", 20}},
        {{"k", 100}, {"m", 200}}};

    std::map<std::string, std::vector<int>> grouped{
        {"evens", {2, 4, 6, 8}},
        {"odds", {1, 3, 5, 7}},
        {"primes", {2, 3, 5, 7}}};

    vix::print("compact:");
    vix::print(nested);
    vix::print(grouped);

    vix::print();
    vix::print("pretty:");
    vix::print(
        vix::options{
            .compact = false,
            .indent = "    "},
        nested);

    vix::print(
        vix::options{
            .compact = false,
            .indent = "  "},
        grouped);

    vix::print();
  }

  void demo_container_adapters()
  {
    print_title("container adapters");

    std::stack<int> st;
    st.push(1);
    st.push(2);
    st.push(3);

    std::queue<std::string> q;
    q.push("first");
    q.push("second");
    q.push("third");

    std::priority_queue<int> pq;
    pq.push(5);
    pq.push(1);
    pq.push(9);
    pq.push(3);

    vix::print(st);
    vix::print(q);
    vix::print(pq);
    vix::print();
  }

  void demo_custom_types()
  {
    print_title("custom types");

    Point point{10, 20};
    User user{"Gaspard", 25, true};
    Product product{"Keyboard", 49.99};
    std::vector<Point> points{{1, 2}, {3, 4}, {5, 6}};
    std::vector<User> users{
        {"Alice", 21, true},
        {"Bob", 27, false},
        {"Charlie", 31, true}};

    vix::print(point);
    vix::print(user);
    vix::print(product);
    vix::print(points);
    vix::print(users);
    vix::print();
  }

  void demo_enum_and_status()
  {
    print_title("enum");

    Status s1 = Status::Idle;
    Status s2 = Status::Running;
    Status s3 = Status::Failed;

    vix::print(s1, s2, s3);
    vix::print();
  }

  void demo_smart_pointers()
  {
    print_title("smart pointers");

    auto up = std::make_unique<int>(42);
    std::unique_ptr<int> up_null;

    auto sp = std::make_shared<std::string>("shared value");
    std::shared_ptr<int> sp_null;

    auto sp_num = std::make_shared<int>(100);
    std::weak_ptr<int> wp = sp_num;
    std::weak_ptr<int> wp_empty;

    vix::print(up);
    vix::print(up_null);
    vix::print(sp);
    vix::print(sp_null);
    vix::print(sp_num);
    vix::print(wp);
    vix::print(wp_empty);
    vix::print();
  }

  void demo_paths_and_time()
  {
    print_title("filesystem and time");

    namespace fs = std::filesystem;

    auto now = std::chrono::system_clock::now();

    vix::print(fs::path{"/usr/local/include"});
    vix::print(fs::path{"relative/path/file.txt"});
    vix::print(now);
    vix::print();
  }

  void demo_reference_wrapper()
  {
    print_title("reference wrapper");

    int value = 99;
    vix::print(std::ref(value), std::cref(value));
    vix::print();
  }

  void demo_multiple_options()
  {
    print_title("multiple options");

    vix::print(
        vix::options{
            .sep = " :: ",
            .end = " <end>\n",
            .flush = true},
        "alpha", "beta", "gamma");

    vix::print(
        vix::options{
            .sep = " | ",
            .compact = false,
            .indent = "    "},
        std::map<std::string, std::vector<int>>{
            {"x", {1, 2}},
            {"y", {3, 4}}});

    vix::print();
  }

  void demo_mixed_values()
  {
    print_title("mixed values");

    std::optional<int> age = 25;
    std::variant<int, std::string> status = std::string{"active"};
    auto values = std::make_tuple("tuple", 42, true);
    std::vector<std::string> tags{"fast", "stable", "simple"};
    std::map<std::string, int> stats{{"ok", 10}, {"fail", 2}};
    Point point{7, 9};
    User user{"Nina", 29, true};

    vix::print(age, status, values, tags, stats, point, user);
    vix::print();
  }

  void demo_legacy_wrappers_still_work()
  {
    print_title("legacy wrappers still work");

    vix::print_to(std::cerr, "print_to:", "still available");
    vix::eprint("eprint:", "still available");
    vix::print_inline("inline");
    vix::print();
    vix::print_named("version", "print v2 showcase");
    vix::print();
  }

  void run_all()
  {
    demo_basic();
    demo_blank_lines();
    demo_separator();
    demo_end();
    demo_stderr_and_flush();
    demo_strings();
    demo_numbers_and_chars();
    demo_optional_variant_tuple_pair();
    demo_sequence_containers();
    demo_associative_containers();
    demo_nested_containers();
    demo_container_adapters();
    demo_custom_types();
    demo_enum_and_status();
    demo_smart_pointers();
    demo_paths_and_time();
    demo_reference_wrapper();
    demo_multiple_options();
    demo_mixed_values();
    demo_legacy_wrappers_still_work();
  }

} // namespace showcase

int main()
{
  showcase::run_all();
  return 0;
}
