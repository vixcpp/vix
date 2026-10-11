#include <vix/Cancellation.hpp>

#include <atomic>
#include <barrier>
#include <cassert>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

namespace
{
  void test_request_and_observation_lifetime()
  {
    vix::CancellationToken surviving_token;
    {
      vix::CancellationSource source;
      const auto copied_source = source;
      surviving_token = source.token();

      assert(!surviving_token.is_cancel_requested());
      assert(copied_source.request_cancel());
      assert(!source.request_cancel());
      assert(surviving_token.is_cancel_requested());
    }

    assert(surviving_token.is_cancel_requested());
    const vix::CancellationToken never_cancelled;
    assert(!never_cancelled.is_cancel_requested());

    vix::CancellationToken token_after_source_destruction;
    {
      vix::CancellationSource unrequested_source;
      token_after_source_destruction = unrequested_source.token();
    }
    assert(!token_after_source_destruction.is_cancel_requested());
  }

  void test_callbacks_and_late_observation()
  {
    vix::CancellationSource source;
    const auto token = source.token();
    int callback_count = 0;
    auto registration = token.on_cancel([&]
                                        { ++callback_count; });

    assert(source.request_cancel());
    assert(callback_count == 1);
    assert(!source.request_cancel());
    assert(callback_count == 1);

    int late_callback_count = 0;
    auto late = token.on_cancel([&]
                                { ++late_callback_count; });
    assert(late_callback_count == 1);
    late.reset();
    registration.reset();
  }

  void test_unregister_prevents_future_callback()
  {
    vix::CancellationSource source;
    int callback_count = 0;
    for (int index = 0; index < 1'000; ++index)
    {
      auto registration = source.token().on_cancel([&]
                                                    { ++callback_count; });
      registration.reset();
    }

    assert(source.request_cancel());
    assert(callback_count == 0);
  }

  void test_request_race_and_reentrancy()
  {
    constexpr int requester_count = 12;
    for (int round = 0; round < 128; ++round)
    {
      vix::CancellationSource source;
      std::atomic<int> winners{0};
      std::atomic<int> callback_count{0};

      auto registration = source.token().on_cancel([&]
                                                    {
                                                      ++callback_count;
                                                      assert(!source.request_cancel());
                                                    });
      std::vector<std::thread> requesters;
      for (int index = 0; index < requester_count; ++index)
      {
        requesters.emplace_back([&]
                                {
                                  if (source.request_cancel())
                                  {
                                    ++winners;
                                  }
                                });
      }
      for (auto &requester : requesters)
      {
        requester.join();
      }

      assert(winners == 1);
      assert(callback_count == 1);
      assert(source.is_cancel_requested());
    }
  }

  void test_reentrant_late_registration()
  {
    vix::CancellationSource source;
    const auto token = source.token();
    int outer_count = 0;
    int inner_count = 0;
    auto registration = token.on_cancel([&]
                                        {
                                          ++outer_count;
                                          auto nested = token.on_cancel([&]
                                                                        { ++inner_count; });
                                          (void)nested;
                                        });
    assert(source.request_cancel());
    assert(outer_count == 1);
    assert(inner_count == 1);
  }

  void test_callback_can_destroy_its_own_registration()
  {
    vix::CancellationSource source;
    vix::CancellationRegistration registration;
    int callback_count = 0;
    registration = source.token().on_cancel([&]
                                             {
                                               ++callback_count;
                                               registration = {};
                                             });

    assert(source.request_cancel());
    assert(callback_count == 1);
  }

  void test_callback_can_unregister_another_registration()
  {
    vix::CancellationSource first_source;
    vix::CancellationSource second_source;
    int second_callback_count = 0;
    auto second_registration = second_source.token().on_cancel([&]
                                                                 { ++second_callback_count; });
    auto first_registration = first_source.token().on_cancel([&]
                                                               { second_registration.reset(); });

    assert(first_source.request_cancel());
    assert(second_source.request_cancel());
    assert(second_callback_count == 0);
    (void)first_registration;
  }

  void test_concurrent_callbacks_can_unregister_each_other()
  {
    vix::CancellationSource first_source;
    vix::CancellationSource second_source;
    vix::CancellationRegistration first_registration;
    vix::CancellationRegistration second_registration;
    std::barrier rendezvous{2};
    std::atomic<int> callback_count{0};

    first_registration = first_source.token().on_cancel([&]
                                                         {
                                                           ++callback_count;
                                                           rendezvous.arrive_and_wait();
                                                           second_registration.reset();
                                                         });
    second_registration = second_source.token().on_cancel([&]
                                                           {
                                                             ++callback_count;
                                                             rendezvous.arrive_and_wait();
                                                             first_registration.reset();
                                                           });

    std::thread first_requester([&]
                                { (void)first_source.request_cancel(); });
    std::thread second_requester([&]
                                 { (void)second_source.request_cancel(); });
    first_requester.join();
    second_requester.join();
    assert(callback_count == 2);
  }

  void test_registration_and_token_keep_state_valid_after_source_destruction()
  {
    vix::CancellationRegistration registration;
    vix::CancellationToken token;
    int callback_count = 0;
    {
      vix::CancellationSource source;
      token = source.token();
      registration = token.on_cancel([&]
                                     { ++callback_count; });
    }

    registration.reset();
    assert(!token.is_cancel_requested());
    assert(callback_count == 0);
  }

  void test_concurrent_registration_and_request()
  {
    vix::CancellationSource source;
    std::atomic<int> invoked{0};
    constexpr int registration_count = 32;
    std::vector<vix::CancellationRegistration> registrations;
    registrations.reserve(registration_count);

    std::thread requester([&]
                          { (void)source.request_cancel(); });
    for (int index = 0; index < registration_count; ++index)
    {
      registrations.push_back(source.token().on_cancel([&]
                                                        { ++invoked; }));
    }
    requester.join();

    assert(invoked == registration_count);
  }

  void test_parent_to_child_propagation_is_explicit_and_one_way()
  {
    vix::CancellationSource parent;
    vix::CancellationSource child;
    const auto bridge = parent.token().on_cancel([child]
                                                 { (void)child.request_cancel(); });

    assert(child.request_cancel());
    assert(!parent.is_cancel_requested());

    vix::CancellationSource second_parent;
    vix::CancellationSource second_child;
    const auto second_bridge = second_parent.token().on_cancel([second_child]
                                                                { (void)second_child.request_cancel(); });
    assert(second_parent.request_cancel());
    assert(second_child.is_cancel_requested());
    (void)bridge;
    (void)second_bridge;
  }

  void test_registration_destruction_waits_for_inflight_callback()
  {
    vix::CancellationSource source;
    std::atomic<bool> entered{false};
    std::atomic<bool> release{false};
    auto registration = source.token().on_cancel([&]
                                                  {
                                                    entered.store(true, std::memory_order_release);
                                                    while (!release.load(std::memory_order_acquire))
                                                    {
                                                      std::this_thread::yield();
                                                    }
                                                  });

    std::atomic<bool> request_returned{false};
    std::thread requester([&]
                          {
                            (void)source.request_cancel();
                            request_returned.store(true, std::memory_order_release);
                          });
    while (!entered.load(std::memory_order_acquire))
    {
      std::this_thread::yield();
    }

    assert(!request_returned.load(std::memory_order_acquire));

    std::atomic<bool> reset_returned{false};
    std::atomic<bool> reset_started{false};
    std::thread resetter([&]
                         {
                           reset_started.store(true, std::memory_order_release);
                           registration.reset();
                           reset_returned.store(true, std::memory_order_release);
                         });
    while (!reset_started.load(std::memory_order_acquire))
    {
      std::this_thread::yield();
    }
    assert(!reset_returned.load(std::memory_order_acquire));
    release.store(true, std::memory_order_release);
    resetter.join();
    requester.join();
    assert(reset_returned.load(std::memory_order_acquire));
    assert(request_returned.load(std::memory_order_acquire));
  }
} // namespace

int main()
{
  test_request_and_observation_lifetime();
  test_callbacks_and_late_observation();
  test_unregister_prevents_future_callback();
  test_request_race_and_reentrancy();
  test_reentrant_late_registration();
  test_callback_can_destroy_its_own_registration();
  test_callback_can_unregister_another_registration();
  test_concurrent_callbacks_can_unregister_each_other();
  test_registration_and_token_keep_state_valid_after_source_destruction();
  test_concurrent_registration_and_request();
  test_parent_to_child_propagation_is_explicit_and_one_way();
  test_registration_destruction_waits_for_inflight_callback();

  return 0;
}
