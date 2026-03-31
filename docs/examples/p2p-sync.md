# P2P Sync Integration (WAL + Outbox + HTTP Peers)

This example shows how to combine:

- A P2P node that can exchange messages over HTTP
- A WAL (write-ahead log) to guarantee durability
- An Outbox queue to retry delivery until an ack is received
- Idempotent apply on the receiver to ensure safe replays and duplicates

Goal: local writes are accepted offline, persisted first, then propagated to peers when the network is available.

## What you will build

Two processes (Node A and Node B) that:

1) Accept a local write: `PUT /kv/<key>` with `{ "value": "..." }`
2) Append the operation to WAL before any side effects
3) Enqueue the operation into the outbox (pending replication)
4) Attempt delivery to each peer: `POST /p2p/ops`
5) Receiver applies the operation once (idempotent) and acks
6) Sender marks the outbox record as delivered once acked

This is not a full CRDT. It is a minimal reliability pattern: durable local write + safe retry + idempotent apply.

## Invariants you should keep in mind

- Append before side effects: if the process crashes after appending, recovery can replay.
- At-least-once delivery: the sender may send the same op multiple times.
- Idempotent apply: receiver must treat duplicates safely.
- Durable acks: receiver acks only after the op is safely stored/applied.

## API

Local write:

- `PUT /kv/<key>` body: `{ "value": "<string>" }`

Replication endpoints:

- `POST /p2p/ops` body: `{ "from": "<node_id>", "ops": [ ... ] }`
- `POST /p2p/ack` body: `{ "from": "<node_id>", "acked_ids": [ ... ] }`

## Data model

An operation is the unit replicated across peers:

```cpp
struct Op
{
  std::string id;       // unique op id (uuid)
  std::string kind;     // "put"
  std::string key;
  std::string value;
  std::int64_t ts_ms;   // timestamp (monotonic-ish)
};
```

### Idempotency key

The `id` is the idempotency key.
Receiver stores applied ids in a small persistent set (or a DB table) so replays are safe.

## Single-file example (server.cpp)

This is intentionally compact.
It demonstrates the flow and the responsibilities.
Adapt storage to your real DB.

```cpp
#include <vix.hpp>

#include <vix/utils/UUID.hpp>
#include <vix/json/json.hpp>
#include <vix/json/convert.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace vix;
using vix::json::Json;

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

struct Op
{
  std::string id;
  std::string kind;
  std::string key;
  std::string value;
  std::int64_t ts_ms{};
};

// Minimal JSON helpers
static Json op_to_json(const Op& op)
{
  return Json::object({
    {"id", op.id},
    {"kind", op.kind},
    {"key", op.key},
    {"value", op.value},
    {"ts_ms", op.ts_ms},
  });
}

static Op op_from_json(const Json& j)
{
  Op op;
  op.id = j.at("id").get<std::string>();
  op.kind = j.at("kind").get<std::string>();
  op.key = j.at("key").get<std::string>();
  op.value = j.at("value").get<std::string>();
  op.ts_ms = j.at("ts_ms").get<std::int64_t>();
  return op;
}

// Very small "WAL" sketch.
// Replace with vix::sync::wal in your project.
struct Wal
{
  std::filesystem::path path;

  void append_line(const std::string& line)
  {
    std::ofstream f(path, std::ios::app);
    f << line << "\n";
  }

  std::vector<std::string> read_all_lines() const
  {
    std::vector<std::string> out;
    std::ifstream f(path);
    std::string s;
    while (std::getline(f, s)) out.push_back(s);
    return out;
  }
};

// Outbox record
struct OutboxRec
{
  std::string op_id;
  std::string peer; // base url
  bool delivered{false};
};

// Minimal persistent sets (replace with DB)
struct Store
{
  std::unordered_map<std::string, std::string> kv;
  std::unordered_set<std::string> applied_ids; // idempotency on receiver
  std::vector<OutboxRec> outbox;
};

// Best effort HTTP POST JSON using Vix client (pseudo).
// Replace with your real http client in Vix.
static bool post_json(const std::string& url, const Json& body, Json* out_res)
{
  // Pseudocode placeholder:
  // HttpClient c;
  // auto r = c.post(url).json(body).send();
  // if (!r.ok()) return false;
  // if (out_res) *out_res = r.json();
  // return true;

  (void)url; (void)body; (void)out_res;
  return false; // replace with real implementation
}

static void enqueue_to_all_peers(Store& st, const std::string& op_id, const std::vector<std::string>& peers)
{
  for (const auto& p : peers)
    st.outbox.push_back(OutboxRec{op_id, p, false});
}

static void try_flush_outbox(Store& st, const std::unordered_map<std::string, Op>& ops_by_id, const std::string& self_id)
{
  for (auto& rec : st.outbox)
  {
    if (rec.delivered) continue;

    auto it = ops_by_id.find(rec.op_id);
    if (it == ops_by_id.end()) continue;

    const Op& op = it->second;

    Json req = Json::object({
      {"from", self_id},
      {"ops", Json::array({ op_to_json(op) })}
    });

    Json resp;
    const bool ok = post_json(rec.peer + "/p2p/ops", req, &resp);
    if (!ok) continue;

    // Expect receiver to respond with acked ids.
    // Example: { "ok": true, "acked_ids": ["..."] }
    if (resp.contains("acked_ids"))
    {
      for (const auto& x : resp["acked_ids"])
      {
        if (x.get<std::string>() == rec.op_id)
          rec.delivered = true;
      }
    }
  }
}

static bool apply_op_idempotent(Store& st, const Op& op)
{
  if (st.applied_ids.find(op.id) != st.applied_ids.end())
    return false; // already applied

  if (op.kind == "put")
    st.kv[op.key] = op.value;

  st.applied_ids.insert(op.id);
  return true;
}

int main()
{
  App app;

  // Config
  const std::string self_id = "node-a";
  const std::vector<std::string> peers = {
    "http://127.0.0.1:8082",
  };

  // Storage
  Store st;
  Wal wal{ "./.vix/wal.log" };

  // Map id -> op (in-memory index for demo)
  std::unordered_map<std::string, Op> ops_by_id;

  // Recovery: replay WAL into state and rebuild ops index/outbox.
  // In production: WAL replay reconstructs both state and outbox safely.
  for (const auto& line : wal.read_all_lines())
  {
    if (line.empty()) continue;
    Json j = Json::parse(line);
    Op op = op_from_json(j);
    ops_by_id[op.id] = op;
    apply_op_idempotent(st, op);
  }

  // Local write endpoint
  app.put("/kv/:key", [&](Request& req, Response& res)
  {
    const std::string key = req.param("key").str();
    Json body = Json::parse(req.body());

    Op op;
    op.id = vix::utils::uuid_v4();
    op.kind = "put";
    op.key = key;
    op.value = body.value("value", "");
    op.ts_ms = now_ms();

    // 1) WAL append first
    wal.append_line(op_to_json(op).dump());

    // 2) Apply locally (side effect)
    apply_op_idempotent(st, op);
    ops_by_id[op.id] = op;

    // 3) Outbox enqueue
    enqueue_to_all_peers(st, op.id, peers);

    // 4) Best effort immediate flush
    try_flush_outbox(st, ops_by_id, self_id);

    res.json({
      {"ok", true},
      {"op_id", op.id},
      {"key", key},
      {"value", op.value},
    });
  });

  // Replication receive endpoint
  app.post("/p2p/ops", [&](Request& req, Response& res)
  {
    Json body = Json::parse(req.body());
    std::vector<std::string> acked;

    for (const auto& jop : body["ops"])
    {
      Op op = op_from_json(jop);

      // Receiver durability rule: append to its WAL before apply
      wal.append_line(op_to_json(op).dump());

      (void)apply_op_idempotent(st, op);
      ops_by_id[op.id] = op;

      acked.push_back(op.id);
    }

    Json arr = Json::array();
    for (const auto& id : acked) arr.push_back(id);

    res.json({
      {"ok", true},
      {"acked_ids", arr},
    });
  });

  // Read endpoint for demo
  app.get("/kv/:key", [&](Request& req, Response& res)
  {
    const std::string key = req.param("key").str();
    auto it = st.kv.find(key);
    if (it == st.kv.end())
    {
      res.status(404).json({"ok", false,
                           "error", "not found"
                          });
      return;
    }
    res.json({"ok", true, "key", key, "value", it->second});
  });

  app.run(8081);
  return 0;
}
```

### Note about HTTP client

The `post_json()` function is a placeholder.
Plug it into your real Vix HTTP client (or whatever transport you already have in your P2P module).
The important part is the protocol and the ordering:

- Sender: WAL append -> local apply -> outbox enqueue -> retry
- Receiver: WAL append -> idempotent apply -> ack

## Run two nodes locally

Terminal A (Node A):

```bash
vix run server.cpp -- -p 8081
```

Terminal B (Node B):

- copy the same file
- change `self_id`, port, and peers accordingly

Example:

- Node A on 8081 peers to 8082
- Node B on 8082 peers to 8081

Then test:

```bash
curl -X PUT http://127.0.0.1:8081/kv/name -d '{ "value": "Alice" }'
curl http://127.0.0.1:8082/kv/name
```

If the network is down, Node A still accepts the write.
When connectivity returns, outbox retries deliver it.

## Next step

If you want deterministic convergence, you will extend this into one of:

- last-write-wins with a stable tie-breaker (node id + op id)
- CRDT register/map
- operation log with causal metadata and conflict resolution

But the reliability core stays the same: WAL + outbox + idempotent apply.

