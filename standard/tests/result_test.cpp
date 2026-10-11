#include <vix/Result.hpp>

#include <cassert>
#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
  inline constexpr vix::ErrorIdentity not_found =
      vix::ErrorIdentity::define<"vix.files">(1);
  inline constexpr vix::ErrorIdentity dns_lookup_failed =
      vix::ErrorIdentity::define<"vix.network">(1);
  inline constexpr vix::ErrorIdentity tcp_connect_failed =
      vix::ErrorIdentity::define<"vix.network">(2);
  inline constexpr vix::ErrorIdentity http_request_failed =
      vix::ErrorIdentity::define<"vix.http">(1);
  inline constexpr vix::ErrorIdentity database_query_failed =
      vix::ErrorIdentity::define<"vix.database">(1);

  vix::Result<int> successful_integer()
  {
    return 42;
  }

  vix::Result<int> failed_integer()
  {
    return vix::Error{not_found, "settings.toml is absent"};
  }

  vix::Result<void> successful_void()
  {
    return vix::Result<void>::success();
  }

  vix::Result<void> failed_void()
  {
    return vix::Error{not_found};
  }

  vix::Result<std::unique_ptr<int>> connect_to_database()
  {
    const vix::Error provider_failure{
        dns_lookup_failed,
        "getaddrinfo returned EAI_AGAIN"};

    return vix::Error{
               tcp_connect_failed,
               "database TCP connection could not be established",
               provider_failure}
        .with_context({"host", "db.internal"});
  }

  vix::Result<void> request_database_health()
  {
    auto connection = connect_to_database();
    if (!connection)
    {
      return vix::Error{
          http_request_failed,
          "health request could not reach the database service",
          connection.error()};
    }

    return vix::Result<void>::success();
  }

  vix::Result<std::string> load_database_catalog()
  {
    auto health = request_database_health();
    if (!health)
    {
      return vix::Error{
          database_query_failed,
          "catalog query could not begin",
          health.error()};
    }

    return std::string{"catalog"};
  }

  struct LargeValue
  {
    std::array<std::byte, 64 * 1024> bytes{};
    int marker{0};
  };

  struct ThrowingMoveValue
  {
    ThrowingMoveValue() = default;
    ThrowingMoveValue(const ThrowingMoveValue &) = delete;
    ThrowingMoveValue(ThrowingMoveValue &&)
    {
      throw std::runtime_error("move construction failed");
    }
  };

  void test_success_and_failure_observation()
  {
    const auto success = successful_integer();
    assert(success);
    assert(success.value() == 42);

    const auto failure = failed_integer();
    assert(!failure);
    assert(failure.error().identity() == not_found);
    assert(failure.error().message() == "settings.toml is absent");
  }

  void test_error_propagation_construction()
  {
    const auto failure = failed_integer();

    const vix::Result<std::string> propagated = failure.error();
    assert(!propagated);
    assert(propagated.error().identity() == not_found);
    assert(propagated.error().message() == "settings.toml is absent");
  }

  void test_nested_result_is_an_ordinary_success_value()
  {
    vix::Result<vix::Result<int>> nested{successful_integer()};
    assert(nested);
    assert(nested.value());
    assert(nested.value().value() == 42);

    const auto lower_failure = failed_integer();
    vix::Result<vix::Result<int>> nested_failure{lower_failure};
    assert(nested_failure);
    assert(!nested_failure.value());
    assert(nested_failure.value().error().identity() == not_found);
  }

  void test_multilayer_translation_preserves_diagnostics()
  {
    const auto catalog = load_database_catalog();
    assert(!catalog);
    assert(catalog.error().identity() == database_query_failed);
    assert(catalog.error().cause()->identity() == http_request_failed);
    assert(catalog.error().cause()->cause()->identity() == tcp_connect_failed);
    assert(catalog.error().cause()->cause()->cause()->identity() == dns_lookup_failed);
  }

  void test_move_only_value()
  {
    vix::Result<std::unique_ptr<int>> result{std::make_unique<int>(7)};
    assert(result);

    std::unique_ptr<int> value = result.take();
    assert(value);
    assert(*value == 7);
    assert(result);
  }

  void test_const_access_and_large_value_extraction()
  {
    const vix::Result<std::string> text{std::string{"settings"}};
    static_assert(std::is_same_v<decltype(text.value()), const std::string &>);
    assert(text.value() == "settings");

    LargeValue initial;
    initial.marker = 99;
    vix::Result<LargeValue> result{std::move(initial)};
    LargeValue extracted = result.take();
    assert(extracted.marker == 99);
  }

  void test_throwing_value_construction_is_not_result_failure()
  {
    bool exception_observed = false;

    try
    {
      ThrowingMoveValue value;
      [[maybe_unused]] vix::Result<ThrowingMoveValue> result{std::move(value)};
    }
    catch (const std::runtime_error &)
    {
      exception_observed = true;
    }

    assert(exception_observed);
  }

  void test_void_result()
  {
    const auto success = successful_void();
    assert(success);

    const auto failure = failed_void();
    assert(!failure);
    assert(failure.error().identity() == not_found);
  }

  void test_type_properties()
  {
    static_assert(!std::is_default_constructible_v<vix::Result<int>>);
    static_assert(!std::is_default_constructible_v<vix::Result<void>>);
    static_assert(std::is_copy_constructible_v<vix::Result<int>>);
    static_assert(std::is_move_constructible_v<vix::Result<int>>);
    static_assert(!std::is_copy_assignable_v<vix::Result<int>>);
    static_assert(!std::is_move_assignable_v<vix::Result<int>>);
    static_assert(!std::is_copy_constructible_v<vix::Result<std::unique_ptr<int>>>);
    static_assert(std::is_move_constructible_v<vix::Result<std::unique_ptr<int>>>);
    static_assert(!std::is_copy_assignable_v<vix::Result<void>>);
    static_assert(!std::is_move_assignable_v<vix::Result<void>>);
  }
} // namespace

int main()
{
  test_success_and_failure_observation();
  test_error_propagation_construction();
  test_nested_result_is_an_ordinary_success_value();
  test_multilayer_translation_preserves_diagnostics();
  test_move_only_value();
  test_const_access_and_large_value_extraction();
  test_throwing_value_construction_is_not_result_failure();
  test_void_result();
  test_type_properties();

  return 0;
}
