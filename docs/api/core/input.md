
# Input

The `input` API provides simple interactive input for Vix console applications.

It is intentionally small.

```cpp
#include <vix/input.hpp>

int main()
{
  std::string name = vix::input("Enter your name: ");

  vix::print("Hello", name);

  return 0;
}
```

If you come from Python, this is the same idea as:

```python
name = input("Enter your name: ")
```

## Header

```cpp
#include <vix/input.hpp>
```

## Namespace

```cpp
namespace vix
```

The public API is:

```cpp
vix::input()
vix::input(prompt)
```

## What input does

`vix::input` reads one complete line from standard input and returns it as a `std::string`.

It has two forms:

```cpp
std::string line = vix::input();
std::string name = vix::input("Enter your name: ");
```

The prompt version prints the prompt first, without adding a newline, then waits for the user to type a line.

## Basic usage

```cpp
#include <vix/input.hpp>
#include <vix/print.hpp>

int main()
{
  std::string name = vix::input("Name: ");

  vix::print("Hello", name);

  return 0;
}
```

Example terminal session:

```txt
Name: Gaspard
Hello Gaspard
```

## Read a line without prompt

```cpp
std::string line = vix::input();
```

Example:

```cpp
#include <vix/input.hpp>
#include <vix/print.hpp>

int main()
{
  vix::print("Type something:");
  std::string line = vix::input();

  vix::print("You typed:", line);

  return 0;
}
```

Output shape:

```txt
Type something:
hello
You typed: hello
```

## Read a line with prompt

```cpp
std::string name = vix::input("Enter your name: ");
```

The prompt is printed immediately and flushed.

Example:

```cpp
#include <vix/input.hpp>
#include <vix/print.hpp>

int main()
{
  std::string name = vix::input("Enter your name: ");
  std::string city = vix::input("Enter your city: ");

  vix::print("Name:", name);
  vix::print("City:", city);

  return 0;
}
```

Example terminal session:

```txt
Enter your name: Gaspard
Enter your city: Kampala
Name: Gaspard
City: Kampala
```

## Public API overview

| API | Purpose |
| --- | --- |
| `vix::input()` | Read one full line from `std::cin`. |
| `vix::input(prompt)` | Print a prompt, then read one full line from `std::cin`. |

## `input()`

```cpp
[[nodiscard]] std::string input();
```

Reads one full line from standard input.

```cpp
std::string line = vix::input();
```

It returns the line without the trailing newline.

## `input(prompt)`

```cpp
[[nodiscard]] std::string input(std::string_view prompt);
```

Prints a prompt without a trailing newline, then reads one full line.

```cpp
std::string email = vix::input("Email: ");
```

The prompt is written to `std::cout` and flushed before reading.

## Return value

Both overloads return:

```cpp
std::string
```

The returned string contains the user input line.

It does not include the trailing newline.

Example:

```cpp
std::string value = vix::input("Value: ");
```

If the user types:

```txt
hello
```

then:

```cpp
value == "hello"
```

## Empty input

If the user presses Enter without typing anything, `input` returns an empty string.

```cpp
std::string value = vix::input("Optional name: ");

if (value.empty())
{
  vix::print("No name provided");
}
```

## Spaces are preserved

`vix::input` reads the full line.

```cpp
std::string sentence = vix::input("Sentence: ");
```

If the user types:

```txt
hello from Vix
```

the result is:

```cpp
sentence == "hello from Vix"
```

This is different from `std::cin >> value`, which stops at whitespace.

## Windows line endings

If the input line ends with `\r`, Vix removes it.

This helps with CRLF line endings.

Example internal behavior:

```txt
"hello\r" → "hello"
```

## Error handling

`vix::input` throws `std::runtime_error` when input or prompt output fails.

Possible errors:

| Situation | Error |
| --- | --- |
| Prompt cannot be written | `vix::input failed: could not write prompt to output stream` |
| Prompt cannot be flushed | `vix::input failed: could not flush prompt to output stream` |
| Input stream is closed | `vix::input failed: input stream is closed` |
| Input stream fails | `vix::input failed: could not read from input stream` |

## Handle input errors

```cpp
#include <exception>
#include <vix/input.hpp>
#include <vix/print.hpp>

int main()
{
  try
  {
    std::string name = vix::input("Name: ");
    vix::print("Hello", name);
  }
  catch (const std::exception &e)
  {
    vix::print("Input error:", e.what());
    return 1;
  }

  return 0;
}
```

## Convert input to a number

`vix::input` always returns a string.

Use standard C++ conversion functions when you need numbers.

```cpp
#include <string>
#include <vix/input.hpp>
#include <vix/print.hpp>

int main()
{
  std::string raw = vix::input("Age: ");

  int age = std::stoi(raw);

  vix::print("Age:", age);

  return 0;
}
```

## Safe number parsing

User input can be invalid.

Use try/catch:

```cpp
#include <exception>
#include <string>
#include <vix/input.hpp>
#include <vix/print.hpp>

int main()
{
  try
  {
    std::string raw = vix::input("Port: ");
    int port = std::stoi(raw);

    vix::print("Using port", port);
  }
  catch (const std::exception &e)
  {
    vix::print("Invalid input:", e.what());
    return 1;
  }

  return 0;
}
```

## Ask again until valid

```cpp
#include <exception>
#include <string>
#include <vix/input.hpp>
#include <vix/print.hpp>

static int ask_port()
{
  while (true)
  {
    try
    {
      std::string raw = vix::input("Port: ");
      int port = std::stoi(raw);

      if (port > 0 && port <= 65535)
      {
        return port;
      }

      vix::print("Port must be between 1 and 65535.");
    }
    catch (const std::exception &)
    {
      vix::print("Please enter a valid number.");
    }
  }
}

int main()
{
  int port = ask_port();

  vix::print("Selected port:", port);

  return 0;
}
```

## Yes or no prompt

```cpp
#include <string>
#include <vix/input.hpp>
#include <vix/print.hpp>

static bool ask_yes_no(std::string_view prompt)
{
  while (true)
  {
    std::string answer = vix::input(prompt);

    if (answer == "y" || answer == "Y" || answer == "yes" || answer == "YES")
    {
      return true;
    }

    if (answer == "n" || answer == "N" || answer == "no" || answer == "NO")
    {
      return false;
    }

    vix::print("Please answer yes or no.");
  }
}

int main()
{
  bool create = ask_yes_no("Create project? [y/n]: ");

  if (create)
  {
    vix::print("Creating project...");
  }
  else
  {
    vix::print("Canceled.");
  }

  return 0;
}
```

## Build a small interactive CLI

```cpp
#include <string>
#include <vix/input.hpp>
#include <vix/print.hpp>

static void greet_user()
{
  std::string name = vix::input("Name: ");
  std::string language = vix::input("Favorite language: ");

  vix::print("Hello", name);
  vix::print("You like", language);
}

int main()
{
  greet_user();
  return 0;
}
```

Run:

```bash
vix run main.cpp
```

Example session:

```txt
Name: Gaspard
Favorite language: C++
Hello Gaspard
You like C++
```

## Use with console

`vix::input` works well with `vix::console` for JavaScript-like console apps.

```cpp
#include <vix/console.hpp>
#include <vix/input.hpp>

int main()
{
  vix::console.info("Welcome to the setup wizard");

  std::string name = vix::input("Project name: ");

  vix::console.log("Creating project", name);

  return 0;
}
```

## Use with print

For simple output, combine `input` with `print`.

```cpp
#include <vix/input.hpp>
#include <vix/print.hpp>

int main()
{
  std::string framework = vix::input("Framework: ");

  vix::print("Selected:", framework);

  return 0;
}
```

## Not for passwords

`vix::input` reads visible terminal input.

Do not use it for passwords or secrets that must be hidden while typing.

```cpp
std::string password = vix::input("Password: "); // visible input
```

For hidden passwords, use a terminal-specific secure input API.

## Not for HTTP request bodies

`vix::input` reads from `std::cin`.

It is for console programs.

Do not use it to read HTTP request bodies inside a Vix web route.

For HTTP, use the request API.

```cpp
app.post("/users", [](Request &req, Response &res) {
  // Use req body/parsing APIs, not vix::input()
});
```

## Common mistakes

### Forgetting the header

```cpp
// Wrong
std::string name = vix::input("Name: ");
```

Fix:

```cpp
#include <vix/input.hpp>
```

### Expecting a number directly

`input` returns `std::string`.

Wrong:

```cpp
int age = vix::input("Age: ");
```

Correct:

```cpp
std::string raw = vix::input("Age: ");
int age = std::stoi(raw);
```

### Expecting hidden input

`input` does not hide typed characters.

Wrong expectation:

```cpp
std::string password = vix::input("Password: ");
```

Use a secure terminal password input helper instead.

### Using `std::cin >>` before input

Mixing formatted extraction with line input can leave a newline in the stream.

Problem:

```cpp
int age{};
std::cin >> age;

std::string name = vix::input("Name: ");
```

The input call may read the leftover newline.

Prefer using `vix::input` consistently:

```cpp
std::string raw_age = vix::input("Age: ");
int age = std::stoi(raw_age);

std::string name = vix::input("Name: ");
```

### Forgetting to handle EOF

When input is piped or closed, input can throw.

```cpp
try
{
  std::string line = vix::input();
}
catch (const std::exception &e)
{
  vix::print("Input failed:", e.what());
}
```

## Best practices

Use prompts that end with a space:

```cpp
std::string name = vix::input("Name: ");
```

Keep parsing separate from reading:

```cpp
std::string raw = vix::input("Port: ");
int port = std::stoi(raw);
```

Validate user input:

```cpp
if (name.empty())
{
  vix::print("Name is required.");
}
```

Use `input` for console apps, setup wizards, small CLIs, tutorials, and examples.

Use request APIs for HTTP data.

Use a secure terminal API for hidden passwords.

## API reference

### `input`

```cpp
[[nodiscard]] std::string input();
```

Reads a full line from standard input.

```cpp
std::string line = vix::input();
```

Throws `std::runtime_error` if reading fails or the input stream is closed.

### `input(prompt)`

```cpp
[[nodiscard]] std::string input(std::string_view prompt);
```

Prints a prompt without a trailing newline, flushes `std::cout`, then reads a full line.

```cpp
std::string name = vix::input("Name: ");
```

Throws `std::runtime_error` if writing the prompt, flushing the prompt, or reading input fails.

## Internal helpers

The public API is intentionally only:

```cpp
vix::input()
vix::input(prompt)
```

Internally, Vix uses:

```cpp
vix::detail::print_prompt(prompt)
vix::detail::read_line()
```

These are implementation details and should not be used directly in application code.

## Summary

`vix::input` is the simple input API for Vix console applications.

Use it like this:

```cpp
std::string name = vix::input("Name: ");
```

Remember:

- it reads one full line,
- it returns `std::string`,
- it preserves spaces,
- it removes trailing `\r`,
- it throws `std::runtime_error` on input/output failure,
- it is for console apps,
- it does not hide passwords,
- it is not for HTTP request bodies.

Core usage:

```cpp
#include <vix/input.hpp>
#include <vix/print.hpp>

int main()
{
  std::string name = vix::input("Name: ");
  vix::print("Hello", name);
  return 0;
}
```
