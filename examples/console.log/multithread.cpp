#include <vix/console.hpp>
#include <thread>
#include <vector>
#include <string>

static void run_worker(int id)
{
  for (int i = 0; i < 5; ++i)
  {
    vix::console.info("worker", id, "iteration", i);
  }
}

int main()
{
  std::vector<std::thread> threads;

  for (int i = 0; i < 4; ++i)
  {
    threads.emplace_back(run_worker, i + 1);
  }

  for (auto &thread : threads)
  {
    thread.join();
  }

  vix::console.info("All workers finished");

  return 0;
}
