# Config API

This page documents the configuration loader in Vix.

Header:

``` cpp
#include <vix/config/Config.hpp>
```

Core type:

``` cpp
vix::config::Config
```

The configuration system is:

-   JSON-based
-   Strongly typed
-   Path-addressable via dot notation
-   Safe with fallback defaults

------------------------------------------------------------------------

# Constructor

``` cpp
vix::config::Config cfg{"config.json"};
```

Loads and parses the JSON file at construction.

If the file cannot be loaded, behavior depends on implementation
configuration. In production, always validate at boot time.

------------------------------------------------------------------------

# Getters

All getters accept:

-   dot path string
-   fallback value

------------------------------------------------------------------------

## get_string

``` cpp
std::string value = cfg.get_string("server.host", "127.0.0.1");
```

------------------------------------------------------------------------

## get_int

``` cpp
int port = cfg.get_int("server.port", 8080);
```

------------------------------------------------------------------------

## get_bool

``` cpp
bool debug = cfg.get_bool("runtime.debug", false);
```

------------------------------------------------------------------------

## get_double

``` cpp
double ratio = cfg.get_double("limits.ratio", 0.5);
```

------------------------------------------------------------------------

# Dot Path Access

Nested JSON can be accessed using dot notation.

Given:

``` json
{
  "database": {
    "host": "localhost",
    "port": 5432
  }
}
```

Access:

``` cpp
cfg.get_string("database.host", "127.0.0.1");
cfg.get_int("database.port", 3306);
```

------------------------------------------------------------------------

# Minimal Usage Example

``` cpp
#include <vix/config/Config.hpp>
#include <iostream>

int main()
{
  vix::config::Config cfg{"config.json"};

  std::string host = cfg.get_string("server.host", "0.0.0.0");
  int port = cfg.get_int("server.port", 8080);

  std::cout << host << ":" << port << "\n";

  return 0;
}
```

------------------------------------------------------------------------

# Environment Override Pattern

Configuration files should not contain secrets in production.

Example override:

``` cpp
#include <vix/config/Config.hpp>
#include <cstdlib>

int main()
{
  vix::config::Config cfg{"config.json"};

  int port = cfg.get_int("server.port", 8080);

  if (const char* env = std::getenv("PORT"))
    port = std::atoi(env);

  return port;
}
```

------------------------------------------------------------------------

# Design Notes

-   Configuration is read at startup.
-   No global mutable config state.
-   Safe access always requires fallback.
-   Deterministic behavior: no implicit environment merging.

The Config API is intentionally small and predictable.

