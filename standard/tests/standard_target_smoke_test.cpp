#include <vix/standard.hpp>

#include <cassert>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
  inline constexpr vix::ErrorIdentity file_not_found =
      vix::ErrorIdentity::define<"vix.files">(1);

  inline constexpr vix::ErrorIdentity process_not_found =
      vix::ErrorIdentity::define<"vix.process">(1);

  inline constexpr vix::ErrorIdentity file_permission_denied =
      vix::ErrorIdentity::define<"vix.files">(2);

  static_assert(file_not_found.domain() == "vix.files");
  static_assert(file_not_found.code() == 1);
  static_assert(file_not_found == file_not_found);
  static_assert(file_not_found != process_not_found);
  static_assert(file_not_found != file_permission_denied);
  static_assert(!std::is_default_constructible_v<vix::ErrorIdentity>);
  static_assert(!std::is_default_constructible_v<vix::Error>);

  void test_identity_accessors_and_comparison()
  {
    assert(file_not_found.domain() == "vix.files");
    assert(file_not_found.code() == 1);
    assert(file_not_found == vix::ErrorIdentity::define<"vix.files">(1));
    assert(file_not_found != process_not_found);
    assert(file_not_found != file_permission_denied);
  }

  void test_identity_only_error()
  {
    const vix::Error error{file_not_found};

    assert(error.identity() == file_not_found);
    assert(!error.has_message());
    assert(error.message().empty());
  }

  void test_error_with_message()
  {
    const vix::Error error{file_not_found, "settings.toml is absent"};

    assert(error.identity() == file_not_found);
    assert(error.has_message());
    assert(error.message() == "settings.toml is absent");
  }

  void test_error_context_and_cause()
  {
    const vix::Error provider_error{
        file_permission_denied,
        "permission denied by the operating system"};

    const vix::Error translated{
        process_not_found,
        "unable to load application settings",
        provider_error};

    const auto enriched = translated.with_context(
        {"path", "settings.toml"});

    assert(enriched.identity() == process_not_found);
    assert(enriched.has_cause());
    assert(enriched.cause()->identity() == file_permission_denied);
    assert(enriched.contexts().size() == 1);
    assert(enriched.contexts()[0].key == "path");
    assert(enriched.contexts()[0].value == "settings.toml");
  }

  void test_error_copy_and_move_preserve_identity()
  {
    const vix::Error original{file_not_found, "journal is absent"};
    const vix::Error copied{original};

    assert(copied.identity() == file_not_found);
    assert(copied.message() == "journal is absent");

    vix::Error moved{std::move(copied)};
    assert(moved.identity() == file_not_found);
    assert(moved.message() == "journal is absent");

    // A moved-from Error remains a valid Error value: identity is never
    // replaced by an invalid or success state through ordinary value use.
    assert(copied.identity() == file_not_found);

    vix::Error copy_assigned{process_not_found};
    copy_assigned = original;
    assert(copy_assigned.identity() == file_not_found);
    assert(copy_assigned.message() == "journal is absent");

    vix::Error move_assigned{process_not_found};
    move_assigned = std::move(copy_assigned);
    assert(move_assigned.identity() == file_not_found);
    assert(move_assigned.message() == "journal is absent");
    assert(copy_assigned.identity() == file_not_found);
  }
} // namespace

int main()
{
  test_identity_accessors_and_comparison();
  test_identity_only_error();
  test_error_with_message();
  test_error_context_and_cause();
  test_error_copy_and_move_preserve_identity();

  return 0;
}
