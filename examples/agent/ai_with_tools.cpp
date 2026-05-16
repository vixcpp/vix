#include <vix/ai.hpp>
#include <vix/print.hpp>

int main()
{
  vix::ai::Agent agent;

  agent.set_model("local:llama");
  agent.set_workspace(".");

  agent.add_tool("filesystem");
  agent.add_tool("shell");
  agent.add_tool("git");

  auto result = agent.run(
      "Analyze this project. If useful, inspect files or use safe shell/git commands.");

  if (!result)
  {
    vix::print("AI error:", result.error().message());
    return 1;
  }

  vix::print(result.value().text());

  return 0;
}
