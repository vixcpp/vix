/**
 *
 *  @file scene_demo.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/game
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix.cpp
 *
 */

#include <vix/game/JobSystem.hpp>
#include <vix/print.hpp>

int main()
{
  vix::game::JobSystem jobs;

  auto submitted = jobs.submit(
      []
      {
        vix::print("job executed");
      });

  if (!submitted)
  {
    vix::print("failed to submit job:", submitted.error().message());
    return 1;
  }

  submitted.value().wait();
  submitted.value().get();

  auto detached = jobs.submit_detached(
      []
      {
        vix::print("detached job executed");
      });

  if (!detached)
  {
    vix::print("failed to submit detached job:", detached.error().message());
    return 1;
  }

  jobs.shutdown();

  return 0;
}
