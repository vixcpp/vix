#include <vix/ai.hpp>
#include <vix/print.hpp>

int main()
{
  vix::ai::Agent agent;

  agent.set_model("local:llama");

  auto result = agent.run(
      "Explain what Vix.cpp is in simple words.");

  if (!result)
  {
    vix::print("AI error:", result.error().message());
    return 1;
  }

  vix::print(result.value().text());

  return 0;
}
