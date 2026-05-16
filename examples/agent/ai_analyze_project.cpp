#include <vix/ai.hpp>
#include <vix/print.hpp>

int main()
{
  vix::ai::Agent agent;

  agent.set_model("local:llama");
  agent.set_workspace(".");
  agent.add_tool("filesystem");

  auto result = agent.run(
      "Analyze this C++ project and explain the most important files.");

  if (!result)
  {
    vix::print("AI error:", result.error().message());
    return 1;
  }

  vix::print(result.value().text());
  vix::print();
  vix::print("Run id:", result.value().run_id());
  vix::print("From cache:", result.value().from_cache());

  return 0;
}
