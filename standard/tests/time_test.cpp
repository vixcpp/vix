#include <vix/Deadline.hpp>
#include <vix/Duration.hpp>
#include <vix/MonotonicClock.hpp>
#include <vix/WallClock.hpp>

#include <array>
#include <cassert>
#include <chrono>
#include <limits>
#include <optional>
#include <thread>
#include <type_traits>
#include <vector>

namespace
{
  constexpr auto second = vix::Duration::seconds(1);
  static_assert(second.nanosecond_count() == 1'000'000'000);
  static_assert(vix::Duration::milliseconds(500) + vix::Duration::milliseconds(500) == second);
  static_assert(vix::Duration::seconds(-2) < vix::Duration::zero());

  void test_duration_value_and_checked_arithmetic()
  {
    const auto ninety_minutes = vix::Duration::minutes(90);
    assert(ninety_minutes == vix::Duration::hours(1) + vix::Duration::minutes(30));
    assert(vix::Duration::seconds(-4) / 2 == vix::Duration::seconds(-2));
    assert(vix::Duration::milliseconds(25) * 4 == vix::Duration::milliseconds(100));
    assert(vix::Duration::from_chrono(std::chrono::nanoseconds{73}).nanosecond_count() == 73);
    assert(vix::Duration::nanoseconds(73).to_chrono() == std::chrono::nanoseconds{73});

    assert(!vix::Duration::max().try_add(vix::Duration::nanoseconds(1)));
    assert(!vix::Duration::min().try_subtract(vix::Duration::nanoseconds(1)));
    assert(!vix::Duration::max().try_multiply(2));
    assert(!vix::Duration::seconds(1).try_divide(0));

    const auto thirty_years = vix::Duration::days(365 * 30LL);
    assert(thirty_years > vix::Duration::zero());
    assert(std::is_trivially_copyable_v<vix::Duration>);
  }

  void test_wall_instant_interoperability()
  {
    constexpr auto epoch = vix::WallInstant::from_unix_nanoseconds(0);
    constexpr auto later = epoch + vix::Duration::seconds(2);
    static_assert(later - epoch == vix::Duration::seconds(2));

    const auto persisted = vix::WallInstant::from_unix_nanoseconds(1'700'000'000'123'456'789LL);
    assert(vix::WallInstant::from_chrono(persisted.to_chrono()) == persisted);
    assert((persisted + vix::Duration::seconds(1)).unix_nanosecond_count() ==
           1'700'000'001'123'456'789LL);

    const auto first = vix::WallClock::now();
    const auto second_observation = vix::WallClock::now();
    // Wall observations intentionally make no ordering claim.
    [[maybe_unused]] const bool comparable = first <= second_observation;
  }

  void test_monotonic_elapsed_time_and_concurrent_access()
  {
    const auto start = vix::MonotonicClock::now();
    const auto later = start + vix::Duration::seconds(3);
    assert(later - start == vix::Duration::seconds(3));
    assert(vix::MonotonicClock::now() >= start);
    assert(!vix::MonotonicInstant::max().try_add(vix::Duration::nanoseconds(1)));
    assert(!vix::MonotonicInstant::min().try_subtract(vix::Duration::nanoseconds(1)));
    static_assert(std::is_trivially_copyable_v<vix::MonotonicInstant>);

    std::vector<std::thread> readers;
    std::array<vix::MonotonicInstant, 4> observations{};
    for (std::size_t index = 0; index < observations.size(); ++index)
    {
      readers.emplace_back([&, index]
                           {
                             observations[index] = vix::MonotonicClock::now();
                           });
    }
    for (auto &reader : readers)
    {
      reader.join();
    }
    for (const auto instant : observations)
    {
      assert(instant >= start);
    }
  }

  void test_deadline_boundary_and_propagation()
  {
    const auto start = vix::MonotonicClock::now();
    const auto parent = vix::Deadline::after(start, vix::Duration::seconds(5));
    assert(!parent.reached(start));
    assert(parent.remaining(start) == vix::Duration::seconds(5));

    const auto after_dns = start + vix::Duration::milliseconds(300);
    const auto after_tcp = after_dns + vix::Duration::milliseconds(500);
    const auto after_tls = after_tcp + vix::Duration::milliseconds(700);
    assert(parent.remaining(after_tls) == vix::Duration::milliseconds(3'500));

    // DNS/TCP/TLS/write/read all receive the same absolute parent boundary.
    const auto dns_limit = vix::Deadline::after(start, vix::Duration::seconds(1));
    const auto effective_dns = vix::Deadline::earlier(parent, dns_limit);
    assert(effective_dns == dns_limit);
    assert(vix::Deadline::earlier(parent, effective_dns) == effective_dns);

    const auto reached = parent.boundary();
    assert(parent.reached(reached));
    assert(parent.remaining(reached) == vix::Duration::zero());
    assert(parent.reached(reached + vix::Duration::nanoseconds(1)));

    const auto zero = vix::Deadline::after(start, vix::Duration::zero());
    assert(zero.reached(start));
    const auto past = vix::Deadline::after(start, vix::Duration::seconds(-1));
    assert(past.reached(start));

    const std::optional<vix::Deadline> absent;
    const std::optional<vix::Deadline> effective = vix::Deadline::earliest(absent, parent);
    assert(effective == parent);
    assert(!vix::Deadline::earliest(absent, absent));
  }

  void test_retry_and_fallback_stay_within_parent_boundary()
  {
    const auto start = vix::MonotonicClock::now();
    const auto parent = vix::Deadline::after(start, vix::Duration::seconds(5));

    const auto after_first_attempt = start + vix::Duration::seconds(2);
    const auto after_backoff = after_first_attempt + vix::Duration::seconds(1);
    const auto retry_deadline = vix::Deadline::earlier(
        parent,
        vix::Deadline::after(after_backoff, vix::Duration::seconds(10)));
    assert(retry_deadline == parent);
    assert(retry_deadline.remaining(after_backoff) == vix::Duration::seconds(2));

    const auto fallback_deadline = vix::Deadline::earlier(
        parent,
        vix::Deadline::after(after_backoff, vix::Duration::seconds(1)));
    assert(fallback_deadline.remaining(after_backoff) == vix::Duration::seconds(1));
  }
} // namespace

int main()
{
  test_duration_value_and_checked_arithmetic();
  test_wall_instant_interoperability();
  test_monotonic_elapsed_time_and_concurrent_access();
  test_deadline_boundary_and_propagation();
  test_retry_and_fallback_stay_within_parent_boundary();

  return 0;
}
