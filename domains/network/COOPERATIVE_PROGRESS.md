# Network progress modes

`Provider::create()` keeps the existing background-driven contract: the
provider owns a reactor thread and suspended operation continuations are
dispatched to the Network continuation workers.

`Provider::create_cooperative()` creates the same provider/reactor without
starting either worker system. Operations are started and suspended in the
usual way, but make no forward progress until the owner calls `Provider::poll()`.
Each call is nonblocking and advances queued commands, expired deadlines,
ready native events, and finally the coroutine resumptions earned by that
turn. A call with no work is valid and returns zero. Callers must continue
polling while they have outstanding operations they expect to finish.

A cooperative provider is confined to its creating thread. Its associated
listeners and connections must be operated on and destroyed on that thread.
Cancellation requested on another thread is queued and observed by a later
poll. Deadlines likewise become observable only while progress is driven.
Cancellation does not interrupt a poll in progress. `poll()` rejects calls
from another thread and recursive calls; a coroutine resumed by a poll may
start another operation, which advances on a subsequent poll. `shutdown()`
must be called on the owner thread and continues to reject providers with live
operations or resources.

The existing awaiter lifetime rule is unchanged: destroying an incomplete
started awaiter is a contract violation (the current implementation terminates)
because its borrowed buffers and transport state must remain alive. If an
operation has already terminalized and only its queued resumption remains, the
awaiter may be abandoned; its resume ticket is atomically retired and a later
poll will not resume its destroyed coroutine.

The two modes share the operation state machine, reactor readiness dispatch,
deadline arbitration, and exactly-once terminal completion. Cooperative mode
does not initialize the continuation worker pool. It is a Network-owned
progress facility, not a general-purpose task scheduler.

The pinned wasi-sdk 34 `wasm32-wasip2` build selects a private wasi-libc
socket/poll provider. On WASI, `Provider::create()` is unsupported and callers
must use `create_cooperative()`; the provider does not start a reactor or
continuation thread. The source implementation awaits build and Wasmtime
conformance evidence, detailed in `WASI_P2_INTEGRATION.md`.

## Top-level operation runner

`vix::network::run(provider, provider.connect(endpoint))` is a blocking entry
point for programs that need to start and observe one operation without
writing a coroutine task type. It returns `Result<Completion<T>>`: the outer
`Result` reports runner setup/reentrancy failure, while the `Completion<T>` is
the operation's terminal outcome, including cancellation and deadline errors.

For a background provider, `run` waits on terminal publication while the
existing provider reactor and continuation workers make progress. For a
cooperative provider, `run` must be called on the provider's owner thread and
blocks that thread while performing blocking reactor waits and provider-owned
progress turns. It does not busy-poll and does not create another thread or
scheduler. Because it blocks, it is not suitable inside a provider progress
callback; such calls are rejected before the operation is started. The
provider must remain alive for the duration of `run`.

On WASI Preview 2, the existing cooperative reactor intentionally supports
only zero-timeout polling. The blocking runner therefore returns the explicit
`unsupported` error before starting the operation; callers must use the
existing `poll()` progress contract there.

The runner owns the operation awaiter until terminal completion and returns
the existing `Completion<T>` exactly once. It does not change `co_await`
semantics; applications that already compose operations asynchronously should
continue to do so. It does not make abandoning an incomplete operation valid.
