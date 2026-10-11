# Resource and Activity Lifecycle Conformance Probes

These probes evaluate observable lifecycle semantics. They do not require a
universal `Resource`, `Owner`, `Borrower`, or `Activity` C++ base type.

## Ownership and borrowing

An implementation must demonstrate all of the following for every resource
contract where lifecycle authority matters:

| Probe | Required observation |
|---|---|
| Factory-created resource | Successful result identifies whether the caller receives lifecycle authority. |
| Resource moved into a component | The receiving component gains the documented authority; the prior owner no longer acts as exclusive owner. |
| Resource supplied by reference, handle, or documented borrow | The component may use it but does not close, stop, or otherwise invalidate it during its own shutdown. |
| Shared pointer supplied to a component | Retaining allocation lifetime does not by itself grant lifecycle authority. |
| Explicit adoption | Any transfer from creator to orchestrator is visible in the API contract. |
| Shared lifecycle authority | It exists only when the resource contract explicitly defines coordination and conflicting lifecycle actions. |

## Operations using resources

| Probe | Required observation |
|---|---|
| Cancelled borrowed socket read | The read may complete Cancelled, but the socket remains usable unless its own resource contract says otherwise. |
| Cancelled process wait | The wait may complete Cancelled while the process continues to exist. |
| Active operation during owner close | The resource contract deterministically specifies drain, cancellation request, failure, or permitted normal completion. |
| Asynchronous borrow | The operation retains, copies, receives transferred ownership of, or is bounded by a borrow guaranteed to outlive the operation. |
| Detached work | Detachment explicitly establishes an independent lifetime; it does not retain an implicit borrow of parent-owned state. |
| Completion carrying a resource | The operation contract states whether the successful value transfers, borrows, or shares lifecycle authority. |

## Close and destruction

| Probe | Required observation |
|---|---|
| Explicit close with observable cleanup failure | The domain provides a finite lifecycle Operation returning `Result` or `Completion`; a destructor is not the only reporting path. |
| RAII cleanup | Owning destruction performs only the documented fallback cleanup and does not pretend to report an unobservable operational Error. |
| Repeated close or stop | The domain explicitly documents whether it is idempotent, rejected as a Fault, or has another defined state transition. |
| Use after closed state | The domain classifies it as an operational condition when externally possible, or as a Fault when it violates a documented precondition. |

## Activities and shutdown

| Probe | Required observation |
|---|---|
| Server with internal listener | Server shutdown may close the listener it owns. |
| Server with caller-supplied listener | Server stops using the borrowed listener but does not close it. |
| Activity stop | Stop request is distinct from final termination; the Activity stops intake, drains or requests cancellation of dependent Operations according to contract, then terminates. |
| Activity failure after long service | The Activity exposes a terminal failure observation without classifying every earlier child Operation as failed. |
| Activity child Operations | The Activity does not report final termination while non-independent children still access Activity-owned resources. |
| Shutdown deadline | It constrains shutdown work but does not itself grant lifecycle authority or define terminal completion. |

## Concurrency declarations

Resource contracts must state, where relevant, whether concurrent use,
read/write overlap, lifecycle action from another thread, and close-versus-
operation races are supported. The Vix Standard does not infer any of these
properties from resource value semantics, pointers, references, or reference
counting.
