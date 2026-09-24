/**
 *
 *  @file basic.cpp
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

  // Register simple tests
  TestRegistry::instance().add(
      TestCase("basic true check", []
               { Assert::is_true(true); }));

  TestRegistry::instance().add(
      TestCase("basic equality", []
               { Assert::equal(2 + 2, 4); }));

  TestRegistry::instance().add(
      TestCase("basic failure example", []
               { Assert::equal(2 * 2, 5, "intentional failure"); }));

  // Run all tests
  return TestRunner::run_all_and_exit();
}
