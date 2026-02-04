#include <iostream>
#include <optional>

#include <vix/validation/Validate.hpp>
#include <vix/validation/Rules.hpp>

using namespace vix::validation;

int main()
{
  std::optional<int> score = std::nullopt;

  auto res = validate("score", score)
                 .rule(rules::required<int>("score is required"))
                 .result();

  std::cout << "ok=" << res.ok() << "\n";
  return res.ok() ? 0 : 1;
}
