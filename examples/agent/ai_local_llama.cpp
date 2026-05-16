#include <vix/ai.hpp>
#include <vix/print.hpp>

int main()
{
  vix::ai::Agent agent;

  agent
      .set_model("local:llama")
      .set_workspace(".")
      .add_tool("filesystem")
      .add_memory(".vix/ai/memory");

  auto result = agent.run(
      "Analyse ce projet C++ et dis-moi pourquoi la compilation échoue.");

  if (!result)
  {
    vix::print("AI error:", result.error().message());
    return 1;
  }

  vix::print(result.value().text());

  return 0;
}
