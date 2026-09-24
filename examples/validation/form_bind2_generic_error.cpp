#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <vix/validation/Form.hpp>
#include <vix/validation/Validate.hpp>

struct SimpleForm
{
  std::string email;

  static bool bind(
      SimpleForm &out,
      const std::vector<std::pair<std::string_view, std::string_view>> &in)
  {
    for (const auto &kv : in)
    {
      if (kv.first == "email")
      {
        out.email.assign(kv.second);
        return true;
      }
    }
    return false;
  }

  static vix::validation::Schema<SimpleForm> schema()
  {
    return vix::validation::schema<SimpleForm>()
        .field("email", &SimpleForm::email,
               [](std::string_view f, const std::string &v)
               {
                 return vix::validation::validate(f, v).email();
               });
  }
};

int main()
{
  using Input = std::vector<std::pair<std::string_view, std::string_view>>;

  Input in = {
      {"x", "y"},
  };

  auto r = vix::validation::Form<SimpleForm>::validate(in);

  std::cout << "ok=" << static_cast<bool>(r) << "\n";
  if (!r)
  {
    for (const auto &e : r.errors().all())
      std::cout << " - field=" << e.field << " message=" << e.message << "\n";
  }
}
