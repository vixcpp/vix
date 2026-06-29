/**
 *
 *  @file ui_shell.cpp
 *  @author Gaspard Kirira
 *
 *  Vix UI desktop shell example.
 *
 */

#include <vix/ui.hpp>

#include <iostream>

int main()
{
  vix::ui::ShellConfig config;

  config
      .set_name("Vix UI Shell")
      .set_title("Vix UI Shell")
      .set_url("http://127.0.0.1:8080")
      .set_width(1100)
      .set_height(760)
      .set_resizable(true)
      .set_devtools(false);

  vix::ui::AppShell shell(config);

  const auto result = shell.start();

  if (result.is_failed())
  {
    std::cerr << result.error_message() << "\n";
    return 1;
  }

  return 0;
}
