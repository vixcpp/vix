# vix make

`vix make` generates C++ files quickly from the folder you are working in.

Use it when you want to create common C++ scaffolding such as classes, structs, enums, functions, concepts, exceptions, tests, lambdas, or config files.

## Usage

```bash
vix make <kind> <name> [options]
vix make:<kind> [name] [options]
```

## What it does

`vix make` is a file generator. It writes files into the current directory by default, or into a custom folder when `--in` is provided.

It is useful for quickly creating: classes, structs, enums, free functions, lambdas, concepts, exceptions, GoogleTest skeletons, and JSON runtime config files.

## Basic usage

```bash
vix make class User
vix make struct Claims
vix make enum Status
vix make function parse_token
vix make test AuthService
vix make config app
```

## Interactive generator form

You can also use the `vix make:<kind>` form:

```bash
vix make:class
vix make:class User
```

If the name is missing, Vix starts an interactive prompt.

## Supported kinds

| Kind | What it generates |
|------|-------------------|
| `class` | A C++ class, usually `.hpp` and `.cpp` |
| `struct` | A plain data struct, usually `.hpp` |
| `enum` | An enum class with helpers |
| `function` | A free function, usually `.hpp` and `.cpp` |
| `lambda` | A modern generic lambda snippet |
| `concept` | A C++20 concept |
| `exception` | A `std::exception`-derived type |
| `test` | A GoogleTest skeleton |
| `module` | Redirects to the modules workflow |
| `config` | A JSON runtime configuration file |

## Generate a class

```bash
vix make class User
vix make class User --in src/domain
vix make class User --namespace app::domain
vix make class User --header-only
vix make class User --dry-run
vix make class User --print
```

## Generate a struct

```bash
vix make struct Claims
vix make struct Claims --namespace auth
vix make struct Claims --in src/auth
```

## Generate an enum

```bash
vix make enum Status
vix make enum Status --in src/domain
vix make enum Status --namespace app::domain
```

## Generate a free function

```bash
vix make function parse_token
vix make function parse_token --in src/auth
vix make function parse_token --header-only
```

## Generate a lambda

```bash
vix make lambda visit_all
vix make lambda visit_all --print
```

## Generate a concept

```bash
vix make concept EqualityComparable
```

This generates a C++20 concept file, useful for expressing constraints in generic code.

## Generate an exception

```bash
vix make exception InvalidToken
vix make exception InvalidToken --in src/auth
```

## Generate a test

```bash
vix make test AuthService
```

This generates a GoogleTest skeleton.

## Generate a config file

```bash
vix make config app
vix make config app --websocket --database
vix make config app --server --logging --waf
vix make config app --no-websocket
```

### Config sections

| Option | Meaning |
|--------|---------|
| `--server` / `--no-server` | Enable/disable server section |
| `--logging` / `--no-logging` | Enable/disable logging section |
| `--waf` / `--no-waf` | Enable/disable WAF section |
| `--websocket` / `--no-websocket` | Enable/disable WebSocket section |
| `--database` / `--no-database` | Enable/disable database section |

## Output directory

By default, files are generated in the current directory.

```bash
vix make enum Status --in src/domain
vix make class User --dir ./apps/api
```

`--dir` is the project root. `--in` is the folder where generated files are written.

## Namespace

```bash
vix make class User --namespace app::domain
vix make struct Claims --namespace auth
```

## Header-only generation

```bash
vix make class User --header-only
```

## Preview and dry run

```bash
vix make lambda visit_all --print
vix make class User --dry-run
```

## Overwrite files

```bash
vix make class User --force
```

Use `--force` carefully — it replaces existing files.

## Options

| Option | Description |
|--------|-------------|
| `-d, --dir <path>` | Project root. Default is the current directory. |
| `--in <path>` | Folder where files should be generated. |
| `--namespace <ns>` | Override namespace. |
| `--header-only` | Generate only header files when supported. |
| `--print` | Print preview or snippet without writing files. |
| `--dry-run` | Show what would be generated without writing files. |
| `--force` | Overwrite existing files. |
| `--server` / `--no-server` | Enable/disable server section for config generation. |
| `--logging` / `--no-logging` | Enable/disable logging section for config generation. |
| `--waf` / `--no-waf` | Enable/disable WAF section for config generation. |
| `--websocket` / `--no-websocket` | Enable/disable WebSocket section for config generation. |
| `--database` / `--no-database` | Enable/disable database section for config generation. |
| `-h, --help` | Show command help. |

## Common workflows

```bash
# Generate a domain class
vix make class User --in src/domain --namespace app::domain

# Generate an auth struct
vix make struct Claims --in src/auth --namespace auth

# Generate an enum for status values
vix make enum Status --in src/domain

# Generate a parser function
vix make function parse_token --in src/auth

# Generate a custom exception
vix make exception InvalidToken --in src/auth

# Generate a test skeleton
vix make test AuthService

# Generate app configuration
vix make config app --websocket --database
```

## Common mistakes

### Using `vix make` instead of `vix new`

`vix make` generates files. It does not create a full project layout. Use `vix new` when starting a new app.

### Confusing `--dir` and `--in`

`--dir` tells Vix where the project root is. `--in` tells Vix where to write files.

```bash
vix make class User --dir ./apps/api --in src/domain
# project root: ./apps/api
# output folder: ./apps/api/src/domain
```

### Missing a name

```bash
# May start interactive prompt
vix make class

# Direct generation
vix make class User

# Or use interactive form intentionally
vix make:class
```

## When to use `vix make`

Use `vix make` when your project already exists and you want to generate C++ files quickly, create test skeletons, generate a runtime config file, or preview generated code before writing it.

Use `vix new` when you need a new project.

## Related commands

| Command | Purpose |
|---------|---------|
| `vix new` | Create a new project |
| `vix dev` | Run the project in development mode |
| `vix run` | Build and run |
| `vix build` | Build project |
| `vix check` | Validate project |
| `vix tests` | Run tests |
| `vix fmt` | Format generated files |

## Next step

Continue with formatting.

[Open the vix fmt guide](/cli/fmt)
