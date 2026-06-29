#ifndef VIX_EXAMPLES_REQUESTS_EXAMPLE_ENV_HPP
#define VIX_EXAMPLES_REQUESTS_EXAMPLE_ENV_HPP

#include <cstddef>
#include <cstdlib>
#include <string>

namespace vix_examples::requests
{
  [[nodiscard]] inline std::string env_or_empty(const char *name)
  {
    if (name == nullptr)
    {
      return {};
    }

#if defined(_WIN32)
    char *buffer = nullptr;
    std::size_t length = 0;

    if (_dupenv_s(&buffer, &length, name) != 0 || buffer == nullptr)
    {
      return {};
    }

    std::string value(buffer);
    std::free(buffer);

    return value;
#else
    const char *value = std::getenv(name);

    if (value == nullptr)
    {
      return {};
    }

    return std::string(value);
#endif
  }

  [[nodiscard]] inline std::string env_or(
      const char *name,
      std::string fallback)
  {
    std::string value = env_or_empty(name);

    if (value.empty())
    {
      return fallback;
    }

    return value;
  }
}

#endif // VIX_EXAMPLES_REQUESTS_EXAMPLE_ENV_HPP
