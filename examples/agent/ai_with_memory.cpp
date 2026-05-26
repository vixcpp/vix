#include <vix/agent.hpp>
#include <vix/print.hpp>

int main()
{
  vix::ai::Agent agent;

  agent.set_model("local:llama");
  agent.set_workspace(".");
  agent.add_tool("filesystem");
  agent.add_memory(".vix/ai/memory");

  auto result = agent.run(
      "Remember the structure of this project and summarize it.");

  if (!result)
  {
    vix::print("AI error:", result.error().message());
    return 1;
  }

  vix::print(result.value().text());
  vix::print();
  vix::print("Run id:", result.value().run_id());

  return 0;
}
