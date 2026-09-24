/**
 *
 *  @file assertions.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */

#include <vix/tests/TestRegistry.hpp>
#include <vix/tests/TestRunner.hpp>
#include <vix/tests/TestCase.hpp>
#include <vix/tests/Assert.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();

  registry.add(TestCase("is_true passes", []
                        { Assert::is_true(10 > 1); }));

  registry.add(TestCase("is_false passes", []
                        { Assert::is_false(1 > 10); }));

  registry.add(TestCase("equal passes", []
                        { Assert::equal(42, 42); }));

  registry.add(TestCase("not_equal passes", []
                        { Assert::not_equal(42, 43); }));

  registry.add(TestCase("equal fails", []
                        { Assert::equal(10, 20, "values must match"); }));

  registry.add(TestCase("forced failure", []
                        { Assert::fail("manual failure trigger"); }));

  return TestRunner::run_all_and_exit();
}
