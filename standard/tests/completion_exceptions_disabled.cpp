#include <vix/Completion.hpp>

#include <memory>

int main()
{
  vix::Completion<std::unique_ptr<int>> completion{std::make_unique<int>(7)};
  auto value = completion.take();

  return completion.status() == vix::CompletionStatus::Success && value && *value == 7
             ? 0
             : 1;
}
