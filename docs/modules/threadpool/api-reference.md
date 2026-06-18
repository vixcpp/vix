# API Reference

This page gives a compact reference for the public `vix::threadpool` API.

For normal application code, include:

```cpp
#include <vix/threadpool/threadpool.hpp>
```

Namespace: `vix::threadpool`

## Main types

- `ThreadPool`,
- `ThreadPoolConfig`,
- `ThreadPoolMetrics`,
- `ThreadPoolStats`,
- `ThreadPoolErrc`,
- `Task`,
- `TaskId`,
- `TaskOptions`,
- `TaskPriority`,
- `TaskStatus`,
- `TaskResult`,
- `TaskHandle`,
- `TaskGroup`,
- `TaskQueue`,
- `CancellationToken`,
- `CancellationSource`,
- `Timeout`,
- `Deadline`,
- `Future<T>`,
- `Promise<T>`,
- `SharedState<T>`,
- `Executor`,
- `ExecutorRef`,
- `InlineExecutor`,
- `ThreadPoolExecutor`,
- `PeriodicTask`,
- `PeriodicTaskConfig`,
- `Scope`,
- `Latch`,
- `Barrier`,
- `parallel_for`,
- `parallel_for_each`,
- `parallel_map`,
- `parallel_reduce`,
- `parallel_pipeline`.

## `ThreadPool`

Header: `#include <vix/threadpool.hpp>`

### Constructors

```cpp
ThreadPool();
explicit ThreadPool(std::size_t threadCount);
explicit ThreadPool(ThreadPoolConfig config);
```

### Lifecycle

```cpp
bool start();
void shutdown() noexcept;
bool running() const noexcept;
bool idle() const;
void wait_idle();
```

### Task submission

```cpp
template <class Fn>
bool post(Fn &&fn, TaskOptions options = TaskOptions{});

template <class Fn>
auto submit(Fn &&fn, TaskOptions options = TaskOptions{})
    -> Future<std::invoke_result_t<std::decay_t<Fn> &>>;

template <class Fn>
auto handle(Fn &&fn, TaskOptions options = TaskOptions{})
    -> TaskHandle<std::invoke_result_t<std::decay_t<Fn> &>>;
```

### Queue and state

```cpp
std::size_t pending() const;
std::size_t clear();
std::size_t thread_count() const noexcept;
TaskId next_task_id() noexcept;
```

### Observability

```cpp
ThreadPoolMetrics metrics() const;
ThreadPoolStats stats() const;
const ThreadPoolConfig &config() const noexcept;
```

### Example

```cpp
vix::threadpool::ThreadPool pool(4);

auto future = pool.submit([]() { return 42; });

int value = future.get();

pool.shutdown();
```

## `ThreadPoolConfig`

Header: `#include <vix/threadpool/ThreadPoolConfig.hpp>`

### Fields

```cpp
std::size_t thread_count;
std::size_t max_thread_count;
std::size_t max_queue_size;

TaskPriority default_priority;

bool allow_dynamic_growth;
bool drain_on_shutdown;
bool swallow_task_exceptions;

std::chrono::microseconds idle_wait;
std::chrono::milliseconds default_timeout;
```

### Methods

```cpp
static std::size_t default_thread_count() noexcept;
ThreadPoolConfig normalized() const;
```

### Example

```cpp
vix::threadpool::ThreadPoolConfig config;

config.thread_count = 4;
config.max_queue_size = 1024;
config.drain_on_shutdown = true;

vix::threadpool::ThreadPool pool(config);
```

## `ThreadPoolMetrics`

Header: `#include <vix/threadpool/ThreadPoolMetrics.hpp>`

### Fields

```cpp
std::size_t worker_count;
std::size_t pending_tasks;
std::uint64_t active_tasks;

std::size_t idle_workers;
std::size_t busy_workers;

std::uint64_t submitted_tasks;
std::uint64_t completed_tasks;
std::uint64_t failed_tasks;
std::uint64_t cancelled_tasks;
std::uint64_t timed_out_tasks;
std::uint64_t rejected_tasks;
```

### Methods

```cpp
bool idle() const noexcept;
std::uint64_t finished_tasks() const noexcept;
std::uint64_t error_tasks() const noexcept;
```

## `ThreadPoolStats`

Header: `#include <vix/threadpool/ThreadPoolStats.hpp>`

### Fields

```cpp
std::uint64_t accepted_tasks;
std::uint64_t rejected_tasks;
std::uint64_t completed_tasks;
std::uint64_t failed_tasks;
std::uint64_t cancelled_tasks;
std::uint64_t timed_out_tasks;

std::chrono::nanoseconds total_execution_time;
std::chrono::nanoseconds max_execution_time;

std::uint64_t idle_waits;
```

### Methods

```cpp
std::uint64_t submitted_tasks() const noexcept;
std::uint64_t finished_tasks() const noexcept;
std::uint64_t error_tasks() const noexcept;
```

## `ThreadPoolErrc`

Header: `#include <vix/threadpool/ThreadPoolError.hpp>`

### Values

```cpp
ThreadPoolErrc::none
ThreadPoolErrc::invalid_task
ThreadPoolErrc::rejected
ThreadPoolErrc::cancelled
ThreadPoolErrc::timeout
ThreadPoolErrc::shutdown
ThreadPoolErrc::broken_promise
ThreadPoolErrc::future_already_retrieved
ThreadPoolErrc::promise_already_satisfied
ThreadPoolErrc::no_state
```

### Helpers

```cpp
const char *to_string(ThreadPoolErrc error) noexcept;
std::error_code make_error_code(ThreadPoolErrc error) noexcept;
const std::error_category &threadpool_error_category() noexcept;
```

## `TaskId` / `WorkerId`

Header: `#include <vix/threadpool/TaskId.hpp>` / `#include <vix/threadpool/WorkerId.hpp>`

```cpp
using TaskId = std::uint64_t;
inline constexpr TaskId invalid_task_id = 0;
bool is_valid_task_id(TaskId id) noexcept;

using WorkerId = std::uint64_t;
inline constexpr WorkerId invalid_worker_id = 0;
bool is_valid_worker_id(WorkerId id) noexcept;
```

## `TaskPriority`

Header: `#include <vix/threadpool/TaskPriority.hpp>`

### Values

```cpp
TaskPriority::lowest
TaskPriority::low
TaskPriority::normal
TaskPriority::high
TaskPriority::highest
```

### Helpers

```cpp
std::int32_t to_priority_value(TaskPriority priority) noexcept;
bool priority_higher_than(TaskPriority lhs, TaskPriority rhs) noexcept;
const char *to_string(TaskPriority priority) noexcept;
```

## `TaskStatus`

Header: `#include <vix/threadpool/TaskStatus.hpp>`

### Values

```cpp
TaskStatus::created
TaskStatus::queued
TaskStatus::running
TaskStatus::completed
TaskStatus::failed
TaskStatus::cancelled
TaskStatus::timed_out
TaskStatus::rejected
```

### Helpers

```cpp
bool task_status_terminal(TaskStatus status) noexcept;
const char *to_string(TaskStatus status) noexcept;
```

## `TaskResult`

Header: `#include <vix/threadpool/TaskResult.hpp>`

### Values

```cpp
TaskResult::none
TaskResult::success
TaskResult::failure
TaskResult::cancelled
TaskResult::timeout
TaskResult::rejected
```

### Helpers

```cpp
bool task_result_success(TaskResult result) noexcept;
bool task_result_error(TaskResult result) noexcept;
const char *to_string(TaskResult result) noexcept;
```

## `WorkerState`

Header: `#include <vix/threadpool/WorkerState.hpp>`

```cpp
WorkerState::created
WorkerState::starting
WorkerState::idle
WorkerState::running
WorkerState::stopping
WorkerState::stopped

const char *to_string(WorkerState state) noexcept;
```

## `TaskOptions`

Header: `#include <vix/threadpool/TaskOptions.hpp>`

### Fields

```cpp
TaskPriority priority;
Timeout timeout;
Deadline deadline;
CancellationToken cancellation;
WorkerId affinity;

bool detached;
bool allow_after_stop;
std::uint32_t flags;
```

### Methods

```cpp
TaskOptions &set_priority(TaskPriority value) noexcept;
TaskOptions &set_timeout(Timeout value) noexcept;
TaskOptions &set_deadline(Deadline value) noexcept;
TaskOptions &set_cancellation(CancellationToken value) noexcept;
TaskOptions &set_affinity(WorkerId value) noexcept;
TaskOptions &set_detached(bool value = true) noexcept;
TaskOptions &set_allow_after_stop(bool value = true) noexcept;
TaskOptions &set_flags(std::uint32_t value) noexcept;

bool has_affinity() const noexcept;
bool cancelled() const noexcept;
bool expired() const noexcept;
```

### Example

```cpp
vix::threadpool::TaskOptions options;

options.set_priority(vix::threadpool::TaskPriority::high);
options.set_timeout(vix::threadpool::Timeout::milliseconds(100));

auto future = pool.submit(fn, options);
```

## `Task`

Header: `#include <vix/threadpool/Task.hpp>`

```cpp
using TaskFunction = detail::MoveOnlyFunction<void()>;

Task();
Task(TaskId id, TaskFunction function, TaskOptions options, std::uint64_t sequence);

TaskId id() const noexcept;
std::uint64_t sequence() const noexcept;

TaskPriority priority() const noexcept;
const TaskOptions &options() const noexcept;

TaskStatus status() const noexcept;
TaskResult result() const noexcept;
ThreadPoolErrc error() const noexcept;

bool valid() const noexcept;
bool terminal() const noexcept;
bool cancelled() const noexcept;
bool expired() const noexcept;

TaskResult run();

std::exception_ptr exception() const noexcept;

clock::time_point created_at() const noexcept;
clock::time_point started_at() const noexcept;
clock::time_point finished_at() const noexcept;
std::chrono::nanoseconds execution_duration() const noexcept;
```

---

## `TaskQueue`

Header: `#include <vix/threadpool/TaskQueue.hpp>`

```cpp
TaskQueue();
explicit TaskQueue(std::size_t maxSize);

bool push(Task task);
std::optional<Task> pop();
const Task *peek() const;

std::size_t clear();

bool empty() const;
bool full() const;
bool bounded() const;

std::size_t size() const;
std::size_t max_size() const;
void set_max_size(std::size_t value);
```

## `TaskHandle<T>`

Header: `#include <vix/threadpool/TaskHandle.hpp>`

```cpp
TaskId id() const noexcept;

bool valid() const noexcept;
explicit operator bool() const noexcept;

void cancel() noexcept;
bool cancelled() const noexcept;

void wait();

T get();     // for non-void T
void get();  // for void

TaskStatus status() const noexcept;
TaskResult result() const noexcept;
ThreadPoolErrc error() const noexcept;

Future<T> &future() noexcept;
const Future<T> &future() const noexcept;
```

### Example

```cpp
auto handle = pool.handle([]() { return 42; });

handle.cancel();

try { int value = handle.get(); }
catch (const std::exception &e) {}
```

## `TaskGroup`

Header: `#include <vix/threadpool/TaskGroup.hpp>`

```cpp
bool add_task(TaskId id);

void finish_task(
    TaskStatus status,
    TaskResult result,
    std::exception_ptr exception = nullptr);

void close();
bool closed() const;

void wait();
void wait_and_rethrow();

void cancel();
bool cancelled() const noexcept;
CancellationToken cancellation_token() const noexcept;

bool empty() const;
bool done() const;

std::uint64_t total_tasks() const;
std::uint64_t pending_tasks() const;
std::uint64_t completed_tasks() const;
std::uint64_t failed_tasks() const;
std::uint64_t cancelled_tasks() const;
std::uint64_t timed_out_tasks() const;
std::uint64_t rejected_tasks() const;

bool has_failure() const;
bool has_error() const;

std::vector<TaskId> task_ids() const;
std::exception_ptr first_exception() const;
```

## `CancellationToken`

Header: `#include <vix/threadpool/CancellationToken.hpp>`

```cpp
CancellationToken();

bool can_cancel() const noexcept;
bool cancelled() const noexcept;
bool is_cancelled() const noexcept;
bool stop_requested() const noexcept;
bool can_continue() const noexcept;

void reset() noexcept;
```

## `CancellationSource`

Header: `#include <vix/threadpool/CancellationSource.hpp>`

```cpp
CancellationSource();

CancellationToken token() const noexcept;

void request_cancel() noexcept;
bool cancelled() const noexcept;
bool is_cancelled() const noexcept;

void reset();
```

## `Timeout`

Header: `#include <vix/threadpool/Timeout.hpp>`

```cpp
Timeout() noexcept;

static Timeout disabled() noexcept;
static Timeout milliseconds(std::int64_t value) noexcept;
static Timeout seconds(std::int64_t value) noexcept;

bool enabled() const noexcept;
bool disabled_value() const noexcept;
std::int64_t count() const noexcept;

bool expired(std::chrono::milliseconds elapsed) const noexcept;

std::chrono::milliseconds duration() const noexcept;
```

## `Deadline`

Header: `#include <vix/threadpool/Deadline.hpp>`

```cpp
using clock = std::chrono::steady_clock;
using time_point = clock::time_point;

Deadline() noexcept;

static Deadline disabled() noexcept;
static Deadline at(time_point value) noexcept;
static Deadline after(clock::duration value) noexcept;
static Deadline from_timeout(Timeout timeout) noexcept;

bool enabled() const noexcept;
bool disabled_value() const noexcept;

bool expired() const noexcept;
bool expired_at(time_point now) const noexcept;

clock::duration remaining() const noexcept;
time_point value() const noexcept;
```

## `Future<T>`

Header: `#include <vix/threadpool/Future.hpp>`

```cpp
Future();

bool valid() const noexcept;
explicit operator bool() const noexcept;

bool ready() const;
void wait() const;

T get();     // for non-void T
void get();  // for void

TaskStatus status() const noexcept;
TaskResult result() const noexcept;
ThreadPoolErrc error() const noexcept;
```

`Future<T>` is move-only:

```cpp
Future(const Future &) = delete;
Future &operator=(const Future &) = delete;

Future(Future &&) noexcept = default;
Future &operator=(Future &&) noexcept = default;
```

## `Promise<T>`

Header: `#include <vix/threadpool/Promise.hpp>`

```cpp
Promise();

Future<T> get_future();

void set_value(T value);  // for non-void T
void set_value();         // for void

void set_exception(std::exception_ptr exception);
void set_current_exception();

void set_status(TaskStatus status);
void set_result(TaskResult result);
void set_error(ThreadPoolErrc error);
```

## `Executor`

Header: `#include <vix/threadpool/Executor.hpp>`

```cpp
class Executor
{
public:
  using Task = std::function<void()>;

  virtual ~Executor() = default;

  virtual bool post(Task task, TaskOptions options = TaskOptions{}) = 0;

  virtual void shutdown() noexcept = 0;
  virtual void wait_idle() = 0;

  virtual bool running() const noexcept = 0;
  virtual bool idle() const = 0;

  virtual ThreadPoolMetrics metrics() const = 0;
  virtual ThreadPoolStats stats() const = 0;
};
```

## `ExecutorRef`

Header: `#include <vix/threadpool/Executor.hpp>`

```cpp
ExecutorRef() noexcept;
explicit ExecutorRef(Executor &executor) noexcept;

bool valid() const noexcept;
explicit operator bool() const noexcept;

bool post(Executor::Task task, TaskOptions options = TaskOptions{});

void shutdown() noexcept;
void wait_idle();

bool running() const noexcept;
bool idle() const;

ThreadPoolMetrics metrics() const;
ThreadPoolStats stats() const;
```

## `InlineExecutor`

Header: `#include <vix/threadpool/InlineExecutor.hpp>`

Runs tasks immediately on the caller thread.

```cpp
bool post(Executor::Task task, TaskOptions options = TaskOptions{}) override;
void shutdown() noexcept override;
void wait_idle() override;
bool running() const noexcept override;
bool idle() const override;
ThreadPoolMetrics metrics() const override;
ThreadPoolStats stats() const override;
```

## `ThreadPoolExecutor`

Header: `#include <vix/threadpool/ThreadPoolExecutor.hpp>`

Non-owning adapter. The referenced `ThreadPool` must outlive the executor.

```cpp
ThreadPoolExecutor() noexcept;
explicit ThreadPoolExecutor(ThreadPool &pool) noexcept;

void reset(ThreadPool &pool) noexcept;
void reset() noexcept;

bool valid() const noexcept;
explicit operator bool() const noexcept;

bool post(Task task, TaskOptions options = TaskOptions{}) override;
void shutdown() noexcept override;
void wait_idle() override;
bool running() const noexcept override;
bool idle() const override;
ThreadPoolMetrics metrics() const override;
ThreadPoolStats stats() const override;

ThreadPool *pool() noexcept;
const ThreadPool *pool() const noexcept;
```

## `PeriodicTaskConfig`

Header: `#include <vix/threadpool/PeriodicTask.hpp>`

```cpp
std::chrono::milliseconds interval;  // default: 1000ms
TaskOptions task_options;
bool run_immediately;                // default: false
bool stop_on_post_failure;           // default: true

static PeriodicTaskConfig every(std::chrono::milliseconds interval);
static std::chrono::milliseconds normalize_interval(
    std::chrono::milliseconds interval) noexcept;

PeriodicTaskConfig normalized() const noexcept;
```

## `PeriodicTask`

Header: `#include <vix/threadpool/PeriodicTask.hpp>`

```cpp
PeriodicTask();

template <class Fn>
PeriodicTask(Executor &executor, Fn &&callback,
             PeriodicTaskConfig config = PeriodicTaskConfig{});

template <class Fn>
PeriodicTask(ThreadPool &pool, Fn &&callback,
             PeriodicTaskConfig config = PeriodicTaskConfig{});

bool start();
void stop() noexcept;
void join() noexcept;

bool running() const noexcept;
bool joinable() const noexcept;

std::uint64_t submitted_ticks() const noexcept;
std::uint64_t failed_posts() const noexcept;
```

## `Scope`

Header: `#include <vix/threadpool/Scope.hpp>`

```cpp
explicit Scope(ThreadPool &pool) noexcept;

template <class Fn>
bool spawn(Fn &&fn, TaskOptions options = TaskOptions{});

void close();
bool closed() const;

void cancel() noexcept;
bool cancelled() const noexcept;
CancellationToken cancellation_token() const noexcept;

void wait();
void wait_and_rethrow();

bool empty() const;
std::size_t size() const;
```

## `Latch`

Header: `#include <vix/threadpool/Latch.hpp>`

```cpp
explicit Latch(std::size_t count) noexcept;

void count_down();
void count_down(std::size_t amount);

void arrive_and_wait();
void wait() const;

bool ready() const;
bool is_ready() const;

std::size_t count() const;
```

## `Barrier`

Header: `#include <vix/threadpool/Barrier.hpp>`

```cpp
explicit Barrier(std::size_t participants) noexcept;

void arrive_and_wait();
void arrive();
void wait();
void release();

std::size_t participants() const;
std::size_t remaining() const;
std::size_t generation() const;
```

## `parallel_for`

Header: `#include <vix/threadpool/ParallelFor.hpp>`

```cpp
struct ParallelForOptions
{
  std::size_t chunk_size;
  TaskOptions task_options;

  static ParallelForOptions with_chunk_size(std::size_t value);
};

template <class Index, class Fn>
void parallel_for(ThreadPool &pool, Index first, Index last, Fn &&fn,
                  ParallelForOptions options = ParallelForOptions{});

template <class Index, class Fn>
void parallel_for(Index first, Index last, Fn &&fn,
                  ParallelForOptions options = ParallelForOptions{});

std::size_t compute_parallel_chunk_size(
    std::size_t totalItems, std::size_t workerCount,
    std::size_t requestedChunkSize);
```

## `parallel_for_each`

Header: `#include <vix/threadpool/ParallelForEach.hpp>`

```cpp
struct ParallelForEachOptions
{
  std::size_t chunk_size;
  TaskOptions task_options;

  static ParallelForEachOptions with_chunk_size(std::size_t value);
};

template <class Iterator, class Fn>
void parallel_for_each(ThreadPool &pool, Iterator first, Iterator last, Fn &&fn,
                       ParallelForEachOptions options = ParallelForEachOptions{});

template <class Container, class Fn>
void parallel_for_each(ThreadPool &pool, Container &container, Fn &&fn,
                       ParallelForEachOptions options = ParallelForEachOptions{});

// Temporary pool overloads:
template <class Iterator, class Fn>
void parallel_for_each(Iterator first, Iterator last, Fn &&fn,
                       ParallelForEachOptions options = ParallelForEachOptions{});

template <class Container, class Fn>
void parallel_for_each(Container &container, Fn &&fn,
                       ParallelForEachOptions options = ParallelForEachOptions{});
```

## `parallel_map`

Header: `#include <vix/threadpool/ParallelMap.hpp>`

```cpp
struct ParallelMapOptions
{
  std::size_t chunk_size;
  TaskOptions task_options;

  static ParallelMapOptions with_chunk_size(std::size_t value);
};

template <class Iterator, class Fn>
auto parallel_map(ThreadPool &pool, Iterator first, Iterator last, Fn &&fn,
                  ParallelMapOptions options = ParallelMapOptions{})
    -> std::vector<std::invoke_result_t<Fn &, decltype(*first)>>;

template <class Container, class Fn>
auto parallel_map(ThreadPool &pool, Container &container, Fn &&fn,
                  ParallelMapOptions options = ParallelMapOptions{})
    -> std::vector</* mapped result type */>;

// Temporary pool overloads also available.
```

## `parallel_reduce`

Header: `#include <vix/threadpool/ParallelReduce.hpp>`

```cpp
struct ParallelReduceOptions
{
  std::size_t chunk_size;
  TaskOptions task_options;

  static ParallelReduceOptions with_chunk_size(std::size_t value);
};

template <class Iterator, class T, class ReduceFn>
T parallel_reduce(ThreadPool &pool, Iterator first, Iterator last,
                  T initial, ReduceFn &&reduce,
                  ParallelReduceOptions options = ParallelReduceOptions{});

template <class Container, class T, class ReduceFn>
T parallel_reduce(ThreadPool &pool, Container &container,
                  T initial, ReduceFn &&reduce,
                  ParallelReduceOptions options = ParallelReduceOptions{});

// Temporary pool overloads also available.
```

## `parallel_pipeline`

Header: `#include <vix/threadpool/ParallelPipeline.hpp>`

```cpp
struct ParallelPipelineOptions { TaskOptions task_options; };

template <class... Stages>
void parallel_pipeline(ThreadPool &pool, ParallelPipelineOptions options,
                       Stages &&...stages);

template <class... Stages>
void parallel_pipeline(ThreadPool &pool, Stages &&...stages);

// Temporary pool overloads also available.
```

### Pipeline builder

```cpp
class Pipeline
{
public:
  using Stage = std::function<void()>;

  Pipeline();
  explicit Pipeline(ParallelPipelineOptions options);

  template <class Fn>
  Pipeline &add(Fn &&fn);

  void clear();

  std::size_t size() const noexcept;
  bool empty() const noexcept;

  const ParallelPipelineOptions &options() const noexcept;
  void set_options(ParallelPipelineOptions options);

  void run(ThreadPool &pool) const;
  void run() const;
};
```

## `parallel` namespace

Header: `#include <vix/threadpool/Parallel.hpp>`

Namespace: `vix::threadpool::parallel`

```cpp
parallel::for_range(...);
parallel::for_each(...);
parallel::map(...);
parallel::reduce(...);
parallel::pipeline(...);
```

These forward to the main public parallel helpers:

```cpp
vix::threadpool::parallel::for_range(pool, 0, 100,
    [](int index) { do_work(index); });
```

## `this_worker`

Header: `#include <vix/threadpool/this_worker.hpp>`

```cpp
WorkerId id() noexcept;
std::size_t index() noexcept;
bool inside_worker() noexcept;
```

These expose worker-local information when code runs inside a Vix worker thread.

## Lower-level types

### `Worker`

Header: `#include <vix/threadpool/Worker.hpp>`

Most application code should use `ThreadPool`. Worker is a lower-level type used by the scheduler.

```cpp
WorkerId id() const noexcept;
std::size_t index() const noexcept;
std::string name() const;

bool submit(Task task);
std::optional<Task> try_pop();
std::size_t clear();

bool start();
void stop() noexcept;
void join() noexcept;

bool joinable() const noexcept;
bool empty() const;
bool full() const;
std::size_t size() const;
std::size_t max_queue_size() const;

WorkerMetrics metrics() const;
```

### `WorkerThread`

Header: `#include <vix/threadpool/WorkerThread.hpp>`

RAII wrapper around `std::thread`. Most users do not use it directly.

```cpp
bool start(Fn &&fn);
void request_stop() noexcept;
void join() noexcept;
bool joinable() const noexcept;
bool stop_requested() const noexcept;
```

### `Scheduler`

Header: `#include <vix/threadpool/Scheduler.hpp>`

Distributes tasks to workers. Most users do not use it directly.

```cpp
bool submit(Task task);
std::size_t pending() const;
std::size_t clear();

ThreadPoolMetrics metrics() const;
ThreadPoolStats stats() const;
```

### Policies

```cpp
// SchedulingPolicy (vix/threadpool/SchedulingPolicy.hpp):
SchedulingPolicy::round_robin
SchedulingPolicy::least_loaded
SchedulingPolicy::affinity
SchedulingPolicy::affinity_then_least_loaded

// QueuePolicy (vix/threadpool/QueuePolicy.hpp):
QueuePolicy::priority
QueuePolicy::fifo
QueuePolicy::lifo

// RejectionPolicy (vix/threadpool/RejectionPolicy.hpp):
RejectionPolicy::reject
RejectionPolicy::caller_runs
RejectionPolicy::discard
```

## Version

Header: `#include <vix/threadpool/version.hpp>`

```cpp
inline constexpr int version_major = 0;
inline constexpr int version_minor = 1;
inline constexpr int version_patch = 0;

inline constexpr const char *version = "0.1.0";
```

## Recommended minimal API

For most users, this is enough:

```cpp
vix::threadpool::ThreadPool pool(4);

pool.post(fn);

auto future = pool.submit(fn);
auto value = future.get();

vix::threadpool::parallel_for(pool, first, last, fn);
vix::threadpool::parallel_map(pool, values, fn);
vix::threadpool::parallel_reduce(pool, values, initial, reduce);

pool.wait_idle();
pool.shutdown();
```

## Header map

| Header | Contents |
|---|---|
| `<vix/threadpool.hpp>` | Complete public API |
| `<vix/threadpool/ThreadPool.hpp>` | Main pool |
| `<vix/threadpool/TaskOptions.hpp>` | Task options |
| `<vix/threadpool/Future.hpp>` | Future result |
| `<vix/threadpool/Promise.hpp>` | Promise producer |
| `<vix/threadpool/TaskHandle.hpp>` | Cancellable task handle |
| `<vix/threadpool/TaskGroup.hpp>` | Manual task coordination |
| `<vix/threadpool/Scope.hpp>` | Structured concurrency |
| `<vix/threadpool/PeriodicTask.hpp>` | Periodic submission |
| `<vix/threadpool/ParallelFor.hpp>` | Parallel index range |
| `<vix/threadpool/ParallelForEach.hpp>` | Parallel container iteration |
| `<vix/threadpool/ParallelMap.hpp>` | Parallel transformation |
| `<vix/threadpool/ParallelReduce.hpp>` | Parallel reduction |
| `<vix/threadpool/ParallelPipeline.hpp>` | Parallel independent stages |
| `<vix/threadpool/Executor.hpp>` | Executor interface |
| `<vix/threadpool/InlineExecutor.hpp>` | Inline executor |
| `<vix/threadpool/ThreadPoolExecutor.hpp>` | ThreadPool executor adapter |
| `<vix/threadpool/Latch.hpp>` | One-shot synchronization |
| `<vix/threadpool/Barrier.hpp>` | Reusable synchronization |

## Stability note

The preferred user-facing API stays simple:

```cpp
pool.post(fn);
pool.submit(fn);
pool.handle(fn);
parallel_for(pool, first, last, fn);
parallel_map(pool, values, fn);
parallel_reduce(pool, values, initial, fn);
pool.shutdown();
```

Lower-level types such as `Worker`, `Scheduler`, `TaskQueue`, and `WorkerThread` exist so Vix can keep the implementation explicit, testable, observable, and extensible.
