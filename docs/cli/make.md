# vix make

`vix make` generates C++ files quickly from the folder you are working in.

Use it when you want to create common C++ scaffolding such as classes, structs, enums, functions, concepts, exceptions, tests, lambdas, or JSON runtime config files.

`vix make` is a file generator. It does not create a full project layout.

Use `vix new` when you want to create a new project.

Use `vix modules` when you want structured module scaffolding.

## Usage

```bash
vix make <kind> <name> [options]
vix make:<kind> [name] [options]
```

## What it does

`vix make` generates files in the current directory by default.

If `--in` is provided, generated files are written inside that folder.

If `--dir` is provided, it becomes the project root, and `--in` is resolved from that root.

```bash
vix make class User
vix make class User --in src/domain
vix make class User --dir ./apps/api --in src/domain
```

With the last command, files are generated inside:

```txt
./apps/api/src/domain
```

## Basic usage

```bash
vix make class User
vix make struct Claims
vix make enum Status
vix make function parse_token
vix make lambda visit_all
vix make concept EqualityComparable
vix make exception InvalidToken
vix make test AuthService
vix make config app
```

## Interactive generator form

You can also use the `vix make:<kind>` form:

```bash
vix make:class
vix make:class User
vix make:config
vix make:config app
```

If the name is missing, Vix starts an interactive prompt.

Current interactive support:

| Kind        | Interactive support         |
| ----------- | --------------------------- |
| `class`     | Yes                         |
| `config`    | Yes                         |
| `struct`    | Not yet                     |
| `enum`      | Not yet                     |
| `function`  | Not yet                     |
| `lambda`    | Not yet                     |
| `concept`   | Not yet                     |
| `exception` | Not yet                     |
| `test`      | Not yet                     |
| `module`    | Redirected to `vix modules` |

## Supported kinds

| Kind        | What it generates                                                        |
| ----------- | ------------------------------------------------------------------------ |
| `class`     | A C++ class. Generates `.hpp` and `.cpp` by default.                     |
| `struct`    | A plain data struct. Header-only by design.                              |
| `enum`      | An enum class with helpers. Header-only by design.                       |
| `function`  | A free function. Generates `.hpp` and `.cpp` by default.                 |
| `lambda`    | A modern generic lambda. Header-only by design.                          |
| `concept`   | A C++20 concept. Header-only by design.                                  |
| `exception` | A `std::exception` derived type. Generates `.hpp` and `.cpp` by default. |
| `test`      | A GoogleTest skeleton.                                                   |
| `module`    | Redirects to the modules workflow.                                       |
| `config`    | A JSON runtime configuration file.                                       |

## Output behavior

By default, `vix make` behaves like a direct file generator.

It does not force generated files into `include/`, `src/`, or `tests/`.

```bash
mkdir src/domain
cd src/domain
vix make class User
```

This generates:

```txt
User.hpp
User.cpp
```

Using `--in`:

```bash
vix make class User --in src/domain
```

This generates:

```txt
src/domain/User.hpp
src/domain/User.cpp
```

Using `--dir` and `--in`:

```bash
vix make class User --dir ./apps/api --in src/domain
```

This generates:

```txt
apps/api/src/domain/User.hpp
apps/api/src/domain/User.cpp
```

## Project root and default namespace

`--dir` sets the project root.

If a `CMakeLists.txt` exists, Vix tries to detect the project name from the CMake `project(...)` declaration.

That project name is used to guess a default namespace.

Example:

```cmake
project(MyApi)
```

Then:

```bash
vix make class User --dir .
```

Can use the default namespace:

```cpp
namespace myapi
{
}
```

You can override it:

```bash
vix make class User --namespace app::domain
```

## Name validation

Names must be valid C++ identifiers.

Valid:

```bash
vix make class User
vix make struct AccessToken
vix make enum Status
```

Invalid:

```bash
vix make class 123User
vix make class user-name
vix make class class
```

C++ reserved keywords are rejected.

Namespace values are also validated:

```bash
vix make class User --namespace app::domain
```

Invalid namespace example:

```bash
vix make class User --namespace app::class
```

## Existing files

Existing files are not overwritten by default.

```bash
vix make class User
vix make class User
```

The second command fails because the files already exist.

Use `--force` to overwrite:

```bash
vix make class User --force
```

Use `--force` carefully.

## Preview and dry run

Use `--dry-run` to see what would be generated without writing files:

```bash
vix make class User --dry-run
```

Use `--print` to print the generated preview or snippet:

```bash
vix make lambda visit_all --print
vix make class User --print
```

Difference:

| Option      | Behavior                                                            |
| ----------- | ------------------------------------------------------------------- |
| `--dry-run` | Prints the kind, name, and target file paths. Does not write files. |
| `--print`   | Prints the generated preview or code snippet. Does not write files. |

## Generate a class

```bash
vix make class User
```

By default, class generation creates:

```txt
User.hpp
User.cpp
```

Default class generation includes:

```txt
default constructor
value constructor
virtual destructor
copy/move operations
getters and setters
one default field: std::string id
```

Example:

```bash
vix make class User --namespace app::domain
```

Generate in a specific folder:

```bash
vix make class User --in src/domain
```

Generate header-only:

```bash
vix make class User --header-only
```

Preview:

```bash
vix make class User --dry-run
vix make class User --print
```

Overwrite existing files:

```bash
vix make class User --force
```

## Interactive class generation

```bash
vix make:class
```

The interactive class generator asks for:

```txt
class name
namespace
number of fields
field names and types
default constructor
value constructor
getters/setters
copy/move operations
virtual destructor
header-only mode
target folder
```

Example field input:

```txt
id:string
email:std::string
age:int
```

`string` is normalized to:

```cpp
std::string
```

## Generate a struct

```bash
vix make struct Claims
```

Struct generation is header-only by design.

It creates:

```txt
Claims.hpp
```

The generated struct includes a simple data payload skeleton:

```txt
id
label
valid
version
tags
```

Examples:

```bash
vix make struct Claims --namespace auth
vix make struct Claims --in src/auth
```

## Generate an enum

```bash
vix make enum Status
```

Enum generation is header-only by design.

It creates:

```txt
Status.hpp
```

The generated enum includes:

```txt
enum class Status
to_string(Status)
is_known(Status)
```

Default enum values:

```txt
Unknown
Active
Disabled
```

Examples:

```bash
vix make enum Status --in src/domain
vix make enum Status --namespace app::domain
```

## Generate a free function

```bash
vix make function parse_token
```

By default, function generation creates:

```txt
parse_token.hpp
parse_token.cpp
```

The generated function uses `std::string_view` for lightweight input.

Example:

```bash
vix make function parse_token --in src/auth
```

Header-only:

```bash
vix make function parse_token --header-only
```

## Generate a lambda

```bash
vix make lambda visit_all
```

Lambda generation is header-only by design.

It creates:

```txt
visit_all.hpp
```

The generated lambda is a modern generic lambda:

```cpp
inline constexpr auto visit_all = []<typename T>(const T &value)
{
  return value;
};
```

For snippet usage:

```bash
vix make lambda visit_all --print
```

## Generate a concept

```bash
vix make concept EqualityComparable
```

Concept generation is header-only by design.

It creates:

```txt
EqualityComparable.hpp
```

The generated concept uses:

```txt
requires-expressions
std::convertible_to
std::is_reference_v
```

It is useful for generic C++20 constraints.

Example:

```bash
vix make concept EqualityComparable --namespace app
```

## Generate an exception

```bash
vix make exception InvalidToken
```

By default, exception generation creates:

```txt
InvalidToken.hpp
InvalidToken.cpp
```

The generated exception:

```txt
derives from std::exception
stores the message in std::string
overrides what() const noexcept
```

Examples:

```bash
vix make exception InvalidToken --in src/auth
vix make exception InvalidToken --namespace auth
vix make exception InvalidToken --header-only
```

## Generate a test

```bash
vix make test AuthService
```

Test generation creates a GoogleTest skeleton.

It creates a file like:

```txt
test_auth_service.cpp
```

The generated test contains:

```cpp
#include <gtest/gtest.h>

TEST(AuthService, DefaultCase)
{
  EXPECT_TRUE(true);
}
```

Replace `EXPECT_TRUE(true)` with real assertions.

Example:

```bash
vix make test AuthService --in tests
```

## Generate a config file

```bash
vix make config app
```

Config generation creates a JSON runtime configuration file.

For `app`, it creates:

```txt
app.json
```

By default, config generation includes:

```txt
server
logging
waf
```

By default, it does not include:

```txt
websocket
database
```

Examples:

```bash
vix make config app
vix make config app --websocket --database
vix make config app --server --logging --waf
vix make config app --no-websocket
```

## Config sections

| Option           | Meaning                    |
| ---------------- | -------------------------- |
| `--server`       | Enable server section.     |
| `--no-server`    | Disable server section.    |
| `--logging`      | Enable logging section.    |
| `--no-logging`   | Disable logging section.   |
| `--waf`          | Enable WAF section.        |
| `--no-waf`       | Disable WAF section.       |
| `--websocket`    | Enable WebSocket section.  |
| `--no-websocket` | Disable WebSocket section. |
| `--database`     | Enable database section.   |
| `--no-database`  | Disable database section.  |

At least one config section must be enabled.

This is invalid:

```bash
vix make config app \
  --no-server \
  --no-logging \
  --no-waf \
  --no-websocket \
  --no-database
```

## Default config content

Default command:

```bash
vix make config app
```

Generates a JSON config with sections like:

```json
{
  "server": {
    "port": 8080,
    "request_timeout": 2000,
    "io_threads": 0,
    "session_timeout_sec": 300,
    "bench_mode": true
  },
  "logging": {
    "async": true,
    "queue_max": 20000,
    "drop_on_overflow": true
  },
  "waf": {
    "mode": "off",
    "max_target_len": 4096,
    "max_body_bytes": 1048576
  }
}
```

With WebSocket:

```bash
vix make config app --websocket
```

Adds:

```json
"websocket": {
  "port": 9090,
  "max_message_size": 65536,
  "idle_timeout": 60,
  "ping_interval": 30,
  "enable_deflate": true,
  "auto_ping_pong": true
}
```

With database:

```bash
vix make config app --database
```

Adds:

```json
"database": {
  "default": {
    "host": "localhost",
    "port": 3306,
    "name": "mydb",
    "user": "myuser",
    "password": ""
  }
}
```

## Generate a module

```bash
vix make module auth
```

Module generation does not belong to `vix make`.

Vix returns an error and points you to the modules workflow.

Use:

```bash
vix modules add auth
```

`vix make` is for direct file generation.

`vix modules` is for structured module scaffolding.

## Options

| Option             | Description                                         |
| ------------------ | --------------------------------------------------- |
| `-d, --dir <path>` | Project root. Default is the current directory.     |
| `--in <path>`      | Folder where files should be generated.             |
| `--namespace <ns>` | Override namespace.                                 |
| `--header-only`    | Generate only header files when supported.          |
| `--print`          | Print preview or snippet without writing files.     |
| `--dry-run`        | Show what would be generated without writing files. |
| `--force`          | Overwrite existing files.                           |
| `--server`         | Enable server section for config generation.        |
| `--no-server`      | Disable server section for config generation.       |
| `--logging`        | Enable logging section for config generation.       |
| `--no-logging`     | Disable logging section for config generation.      |
| `--waf`            | Enable WAF section for config generation.           |
| `--no-waf`         | Disable WAF section for config generation.          |
| `--websocket`      | Enable WebSocket section for config generation.     |
| `--no-websocket`   | Disable WebSocket section for config generation.    |
| `--database`       | Enable database section for config generation.      |
| `--no-database`    | Disable database section for config generation.     |
| `-h, --help`       | Show command help.                                  |

## Common workflows

### Generate a domain class

```bash
vix make class User --in src/domain --namespace app::domain
```

### Generate a header-only class

```bash
vix make class User --in include/domain --namespace app::domain --header-only
```

### Generate an auth struct

```bash
vix make struct Claims --in src/auth --namespace auth
```

### Generate an enum for status values

```bash
vix make enum Status --in src/domain
```

### Generate a parser function

```bash
vix make function parse_token --in src/auth
```

### Generate a reusable lambda snippet

```bash
vix make lambda visit_all --print
```

### Generate a C++20 concept

```bash
vix make concept EqualityComparable --in src/core
```

### Generate a custom exception

```bash
vix make exception InvalidToken --in src/auth
```

### Generate a test skeleton

```bash
vix make test AuthService --in tests
```

### Generate app configuration

```bash
vix make config app --websocket --database
```

### Preview before writing

```bash
vix make class User --in src/domain --dry-run
```

### Print generated snippet

```bash
vix make lambda visit_all --print
```

## Recommended workflow

```bash
# 1. Generate the file
vix make class User --in src/domain --namespace app::domain

# 2. Format generated files
vix fmt

# 3. Build
vix build

# 4. Run tests
vix tests
```

For tests:

```bash
vix make test AuthService --in tests
vix fmt
vix tests
```

For config:

```bash
vix make config app --websocket --database
vix run
```

## Common mistakes

### Using `vix make` instead of `vix new`

`vix make` generates files.

It does not create a full project layout.

Wrong:

```bash
vix make class api
```

Correct for a new project:

```bash
vix new api
```

Then generate files inside the project:

```bash
cd api
vix make class User --in src/domain
```

### Using `vix make module` instead of `vix modules`

`vix make module` redirects to the modules workflow.

Use:

```bash
vix modules add auth
```

### Confusing `--dir` and `--in`

`--dir` is the project root.

`--in` is the output folder.

```bash
vix make class User --dir ./apps/api --in src/domain
```

This means:

```txt
project root: ./apps/api
output folder: ./apps/api/src/domain
```

### Expecting files to be generated in `include/` and `src/`

`vix make` does not force `include/` or `src/`.

It writes into the current folder by default.

```bash
cd src/domain
vix make class User
```

Generates:

```txt
User.hpp
User.cpp
```

To write into a specific folder, use:

```bash
vix make class User --in src/domain
```

### Missing a name

This starts interactive mode when supported:

```bash
vix make class
vix make:class
```

Direct generation:

```bash
vix make class User
```

Interactive mode is currently implemented for:

```txt
class
config
```

For other kinds, provide the name directly:

```bash
vix make struct Claims
vix make enum Status
vix make function parse_token
```

### Using an invalid C++ name

Wrong:

```bash
vix make class 123User
vix make class user-name
vix make class class
```

Correct:

```bash
vix make class User
vix make class UserProfile
vix make function parse_token
```

### Overwriting files accidentally

Vix does not overwrite existing files unless `--force` is provided.

```bash
vix make class User --force
```

Use `--dry-run` first when unsure:

```bash
vix make class User --dry-run
```

### Disabling every config section

Wrong:

```bash
vix make config app \
  --no-server \
  --no-logging \
  --no-waf \
  --no-websocket \
  --no-database
```

At least one section must remain enabled.

## When to use `vix make`

Use `vix make` when:

```txt
your project already exists
you want to generate one or more C++ files quickly
you want a class, struct, enum, function, concept, exception, lambda, or test skeleton
you want to generate a runtime JSON config file
you want to preview generated code before writing files
```

Use `vix new` when:

```txt
you need a new project
you need a full project layout
you need templates such as app, backend, web, vue, game, or library
```

Use `vix modules` when:

```txt
you need structured module scaffolding
you want module-level folders and integration
you are adding a module to an existing Vix project
```

## Related commands

| Command       | Purpose                              |
| ------------- | ------------------------------------ |
| `vix new`     | Create a new project.                |
| `vix modules` | Manage structured modules.           |
| `vix fmt`     | Format generated files.              |
| `vix build`   | Build the project.                   |
| `vix dev`     | Run the project in development mode. |
| `vix run`     | Build and run.                       |
| `vix check`   | Validate the project.                |
| `vix tests`   | Run tests.                           |

## Next step

Continue with formatting.

[Open the vix fmt guide](/cli/fmt)
