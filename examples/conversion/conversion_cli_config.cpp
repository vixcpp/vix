/**
 *
 *  @file conversion_cli_config.cpp
 *  @author Gaspard Kirira
 *
 *  Vix.cpp - Conversion Patterns (CLI + Config) (examples/conversion/)
 *
 *  Goal:
 *    Real-world patterns you copy/paste:
 *      - parse CLI arguments like "--port=8080" or "--debug=true"
 *      - parse env-like strings ("PORT=8080")
 *      - parse optional values safely (fallbacks)
 *      - parse enum values with explicit mapping
 *      - print errors with code + position + input
 *
 *  Notes:
 *    - This file is intentionally dependency-free (only vix::conversion + std).
 *    - No exceptions: errors are values (expected).
 *    - Beginner friendly, but the patterns scale to production CLIs.
 *
 *  Vix.cpp
 *
 */

#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <vix/conversion/ConversionError.hpp>
#include <vix/conversion/Parse.hpp>
#include <vix/conversion/ToEnum.hpp>
#include <vix/conversion/ToString.hpp>

using namespace vix::conversion;

// ---------------------------------------------------------------------------
// Error printing
// ---------------------------------------------------------------------------

static void print_err(std::string_view label, const ConversionError &e)
{
  std::cout << label << " -> error: " << to_string(e.code)
            << " position=" << e.position
            << " input='" << e.input << "'\n";
}

// ---------------------------------------------------------------------------
// Simple helpers for parsing "--key=value" arguments
// ---------------------------------------------------------------------------

static std::pair<std::string_view, std::string_view> split_kv(std::string_view s, char sep)
{
  const std::size_t pos = s.find(sep);
  if (pos == std::string_view::npos)
    return {s, {}};
  return {s.substr(0, pos), s.substr(pos + 1)};
}

static bool starts_with(std::string_view s, std::string_view prefix)
{
  return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

// Normalize:
//  "--port=8080" -> key="port", value="8080"
//  "--debug"     -> key="debug", value=""  (treated as "true" by convention)
static std::pair<std::string_view, std::string_view> parse_arg_kv(std::string_view arg)
{
  if (!starts_with(arg, "--"))
    return {{}, {}};

  arg.remove_prefix(2);

  auto [k, v] = split_kv(arg, '=');
  return {k, v};
}

// ---------------------------------------------------------------------------
// A small "Config" struct, as you would do in a real CLI
// ---------------------------------------------------------------------------

enum class Mode
{
  Dev,
  Prod,
};

static constexpr EnumEntry<Mode> modes[] = {
    {"dev", Mode::Dev},
    {"prod", Mode::Prod},
};

static const char *mode_name(Mode m)
{
  switch (m)
  {
  case Mode::Dev:
    return "dev";
  case Mode::Prod:
    return "prod";
  default:
    return "unknown";
  }
}

struct AppConfig
{
  int port{8080};
  bool debug{false};
  Mode mode{Mode::Dev};
  int workers{4};

  void print() const
  {
    std::cout << "Config:\n";
    std::cout << "  port    = " << port << "\n";
    std::cout << "  debug   = " << (debug ? "true" : "false") << "\n";
    std::cout << "  mode    = " << mode_name(mode) << "\n";
    std::cout << "  workers = " << workers << "\n";
  }
};

// ---------------------------------------------------------------------------
// Parsing patterns (beginner friendly)
// ---------------------------------------------------------------------------

static bool parse_bool_flag(std::string_view raw, bool default_if_empty)
{
  if (raw.empty())
    return default_if_empty;

  auto r = parse<bool>(raw);
  if (!r)
    return default_if_empty;

  return r.value();
}

static int parse_int_or(std::string_view raw, int fallback)
{
  auto r = parse<int>(raw);
  if (!r)
    return fallback;
  return r.value();
}

// ---------------------------------------------------------------------------
// Apply CLI args into config (expert pattern but readable)
// ---------------------------------------------------------------------------

static bool apply_cli_args(AppConfig &cfg, const std::vector<std::string_view> &args)
{
  // Returns false if a fatal parsing error happened.
  // In a real CLI you would accumulate errors; here we keep it simple and strict.

  for (auto a : args)
  {
    auto [key, value] = parse_arg_kv(a);
    if (key.empty())
      continue;

    if (key == "port")
    {
      auto r = parse<int>(value);
      if (!r)
      {
        print_err("--port", r.error());
        return false;
      }
      cfg.port = r.value();
    }
    else if (key == "debug")
    {
      // "--debug" alone => true
      cfg.debug = parse_bool_flag(value, true);
    }
    else if (key == "workers")
    {
      auto r = parse<int>(value);
      if (!r)
      {
        print_err("--workers", r.error());
        return false;
      }
      cfg.workers = r.value();
    }
    else if (key == "mode")
    {
      auto r = to_enum<Mode>(value, modes); // case-insensitive default=true
      if (!r)
      {
        print_err("--mode", r.error());
        return false;
      }
      cfg.mode = r.value();
    }
    else
    {
      std::cout << "warning: unknown flag '--" << key << "'\n";
    }
  }

  return true;
}

// ---------------------------------------------------------------------------
// Parse "env-like" variables (PORT=..., DEBUG=..., MODE=..., WORKERS=...)
// ---------------------------------------------------------------------------

static bool apply_env_map(AppConfig &cfg, const std::unordered_map<std::string, std::string> &env)
{
  if (auto it = env.find("PORT"); it != env.end())
  {
    auto r = parse<int>(it->second);
    if (!r)
    {
      print_err("ENV PORT", r.error());
      return false;
    }
    cfg.port = r.value();
  }

  if (auto it = env.find("DEBUG"); it != env.end())
  {
    auto r = parse<bool>(it->second);
    if (!r)
    {
      print_err("ENV DEBUG", r.error());
      return false;
    }
    cfg.debug = r.value();
  }

  if (auto it = env.find("WORKERS"); it != env.end())
  {
    auto r = parse<int>(it->second);
    if (!r)
    {
      print_err("ENV WORKERS", r.error());
      return false;
    }
    cfg.workers = r.value();
  }

  if (auto it = env.find("MODE"); it != env.end())
  {
    auto r = to_enum<Mode>(it->second, modes);
    if (!r)
    {
      print_err("ENV MODE", r.error());
      return false;
    }
    cfg.mode = r.value();
  }

  return true;
}

// ---------------------------------------------------------------------------
// Demo runner
// ---------------------------------------------------------------------------

static void run_demo(std::vector<std::string_view> cli_args,
                     std::unordered_map<std::string, std::string> env)
{
  std::cout << "Vix.cpp - Conversion CLI/Config Pattern Demo\n";

  AppConfig cfg;

  std::cout << "\n== Defaults\n";
  cfg.print();

  std::cout << "\n== Apply ENV\n";
  if (!apply_env_map(cfg, env))
  {
    std::cout << "fatal: failed to parse env\n";
    return;
  }
  cfg.print();

  std::cout << "\n== Apply CLI\n";
  if (!apply_cli_args(cfg, cli_args))
  {
    std::cout << "fatal: failed to parse cli args\n";
    return;
  }
  cfg.print();

  std::cout << "\nDone.\n";
}

int main()
{
  // Simulated CLI args:
  //   --port=9090 --debug --mode=prod --workers=8
  //
  // Try breaking them to see errors:
  //   --port=12x
  //   --mode=unknown
  //   --workers=999999999999999999999
  std::vector<std::string_view> cli = {
      "--port=9090",
      "--debug",
      "--mode=prod",
      "--workers=8",
  };

  // Simulated environment:
  // These are typically strings in the real OS environment.
  std::unordered_map<std::string, std::string> env = {
      {"PORT", "8081"},
      {"DEBUG", "false"},
      {"MODE", "dev"},
      {"WORKERS", "4"},
  };

  run_demo(cli, env);
  return 0;
}
