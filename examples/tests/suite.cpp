/**
 *
 *  @file suite.cpp
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
#include <vix/tests/TestSuite.hpp>
#include <vix/tests/TestCase.hpp>
#include <vix/tests/Assert.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();

  // Create a suite
  TestSuite math("math operations");

  math.add(TestCase("addition", []
                    { Assert::equal(1 + 1, 2); }));

  math.add(TestCase("subtraction", []
                    { Assert::equal(5 - 3, 2); }));

  math.add(TestCase("multiplication", []
                    { Assert::equal(3 * 3, 9); }));

  math.add(TestCase("division", []
                    { Assert::equal(10 / 2, 5); }));

  // Another suite
  TestSuite logic("logic checks");

  logic.add(TestCase("true condition", []
                     { Assert::is_true(true); }));

  logic.add(TestCase("false condition", []
                     { Assert::is_false(false); }));

  // Register suites
  registry.add(std::move(math));
  registry.add(std::move(logic));

  return TestRunner::run_all_and_exit();
}
