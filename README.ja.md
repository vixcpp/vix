# Vix.cpp

<p align="center" style="margin:0;">
  <img 
    src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1762524350/vixcpp_etndhz.png" 
    alt="Vix.cpp Banner" 
    width="100%" 
    style="
      display:block;
      height:auto;
      max-width:900px;
      margin:auto;
      object-fit:cover;
      border-radius:8px;
    ">
</p>

<h1 align="center">Vix.cpp</h1>

<p align="center">
  <img src="https://img.shields.io/badge/C++20-Standard-blue">
  <img src="https://img.shields.io/badge/License-MIT-green">
</p>

---

# 🌍 Vix とは？

**Vix** は、次世代の **オフラインファースト・ピアツーピア・超高速 C++ モダンランタイム**です。

[English](README.md)

目標は明確です。

> **Node / Deno / Bun のようなアプリを実行でき、  
> しかも不安定で低品質な「現実世界のネットワーク」を前提に設計されたランタイム**

Vix は単なるバックエンドフレームワークではありません。  
これは **モジュラー構成のランタイム**であり、分散アプリケーション、エッジシステム、オフラインデバイス、そして従来のクラウド前提フレームワークが機能しない環境向けに設計されています。

**FastAPI**, **Vue.js**, **React**、最新のランタイムに着想を得つつ、  
**C++20 によってゼロから再設計**され、圧倒的な速度と完全な制御性を実現しています。

---

# ⚡ ベンチマーク（更新版 — 二〇二五年 十二月）

すべてのベンチマークは **wrk** を使用して実行されました。  
**八スレッド / 八百接続 / 三十秒**、同一マシンで測定しています。

**環境**  
Ubuntu 二十四点〇四 — Intel Xeon — C++20 最適化ビルド — ログ無効

結果は `"OK"` を返すシンプルなエンドポイントでの **定常状態スループット**です。

---

## 🚀 秒間リクエスト数

| フレームワーク                  | Requests/sec                          | 平均レイテンシ          | 転送量/秒            |
| ------------------------------- | ------------------------------------- | ----------------------- | -------------------- |
| ⭐ **Vix.cpp (v 一・一二・三)** | **約 九万八千九百四十二**（CPU 固定） | **七・三〜一〇・八 ms** | **約 一三・八 MB/s** |
| **Vix.cpp（通常実行）**         | 八万一千三百                          | 九・七〜一〇・八 ms     | 約 一一・三 MB/s     |
| Go（Fiber）                     | 八万一千三百三十六                    | 〇・六七 ms             | 一〇・一六 MB/s      |
| **Deno**                        | 約 四万八千八百六十八                 | 一六・三四 ms           | 約 六・九九 MB/s     |
| Node.js（Fastify）              | 四千二百二十                          | 一六・〇〇 ms           | 〇・九七 MB/s        |
| PHP（Slim）                     | 二千八百四                            | 一六・八七 ms           | 〇・四九 MB/s        |
| Crow（C++）                     | 一千百四十九                          | 四一・六〇 ms           | 〇・三五 MB/s        |
| FastAPI（Python）               | 七百五十二                            | 六三・七一 ms           | 〇・一一 MB/s        |

> 🔥 **新記録**  
> 単一コアに固定（`taskset -c 2`）した場合、  
> **約 九万九千 req/s** に到達し、Go を上回り、最速クラスの C++ マイクロフレームワークに並びました。

---

## 📝 補足

### ✔ Vix.cpp が Go レベルの性能を出せる理由

- ゼロコスト抽象化
- HTTP ワークロード向けに最適化された独自 ThreadPool
- 最適化された HTTP パイプライン
- 高速パスルーティング
- Beast ベースの I/O
- 最小限のメモリアロケーション
- 予測可能なスレッドモデル

---

## 🦕 Deno ベンチマーク（参考）

```bash
wrk -t8 -c800 -d30s --latency http://127.0.0.1:8000
Requests/sec: 48,868.73
```

### ✔ Vix.cpp 推奨ベンチマークモード

Vix.cpp リポジトリ内（ビルトイン例を使用）でベンチマークを行う場合：

```bash
cd ~/vixcpp/vix
export VIX_LOG_LEVEL=critical
export VIX_LOG_ASYNC=false

# 最適化されたサンプルサーバーを起動
vix run example main
```

次に、別のターミナルで実行します：

```bash
wrk -t8 -c800 -d30s --latency http://127.0.0.1:8080/bench
```

より安定した結果を得たい場合は、CPU コアを固定してください：

```bash
taskset -c 2 ./build/main
wrk -t8 -c800 -d30s --latency http://127.0.0.1:8080/bench
```

#### 🏁 結果：約 98,942 req/s

✔ Fast-path ルーティングにより 1〜3% の性能向上
✔ /fastbench を使用すると RequestHandler のオーバーヘッドを回避できます

---

# 🧭 クイック例

```cpp
#include <vix.hpp>
using namespace vix;

int main() {
    App app;

    app.get("/", [](auto&, auto& res) {
        res.json({ "message", "Hello world" });
    });

    app.run(8080);
}
```

### QueryBuilder ORM

```cpp
QueryBuilder qb;
qb.raw("UPDATE users SET age=? WHERE email=?")
  .param(29)
  .param("zoe@example.com");
```

### 最小構成の HTTP + WebSocket サーバー

この例は 完全に動作する最小の **HTTP + WebSocket ハイブリッドサーバーを示しています。**

### 特徴

- 基本的な GET ルート
- シンプルな WebSocket 接続処理
- サーバーの自動起動

### サンプルコード（要約）

```cpp
#include <vix.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

using namespace vix;

int main()
{
    auto bundle = vix::make_http_and_ws("config/config.json");
    auto &[app, ws] = bundle;

    app.get("/", [](const Request &, Response &res)
            { res.json({"framework", "Vix.cpp",
                        "message", "HTTP + WebSocket example (basic) 🚀"}); });

    ws.on_open([&ws](auto &session)
               {
        (void)session;

        ws.broadcast_json("chat.system", {
            "user", "server",
            "text", "Welcome to Vix WebSocket! 👋"
        }); });

    vix::run_http_and_ws(app, ws, 8080);

    return 0;
}
```

## 最小 WebSocket クライアント

```cpp
auto client = Client::create("localhost", "9090", "/");

client->on_open([] {
    std::cout << "Connected!" << std::endl;
});

client->send("chat.message", {"text", "Hello world!"});
```

## 1. Hello World (JSON)

```cpp
app.get("/", [](Request req, Response res) {
    return json::o("message", "Hello from Vix");
});
```

---

## 2. ルートパラメータ

```cpp
app.get("/users/{id}", [](Request req, Response res) {
    auto id = req.param("id");
    return json::o("user_id", id);
});
```

---

## 3. クエリパラメータ

```cpp
app.get("/search", [](Request req, Response res) {
    auto q = req.query_value("q", "none");
    auto page = req.query_value("page", "1");

    return json::o(
        "query", q,
        "page", page
    );
});
```

---

## 4. ステータス + ペイロード自動返却（FastAPI 風）

```cpp
app.get("/missing", [](Request req, Response res) {
    return std::pair{
        404,
        json::o("error", "Not found")
    };
});
```

---

## 5. リダイレクト

```cpp
app.get("/go", [](Request req, Response res) {
    res.redirect("https://vixcpp.com");
});
```

---

## 6. ステータスメッセージのみ返却

```cpp
app.get("/forbidden", [](Request req, Response res) {
    res.status(403).send();
});
```

---

## 7. POST JSON ボディ

```cpp
app.post("/echo", [](Request req, Response res) {
    return json::o(
        "received", req.json()
    );
});
```

---

## 8. 型付き JSON パース

```cpp
struct UserInput {
    std::string name;
    int age;
};

app.post("/users", [](Request req, Response res) {
    UserInput input = req.json_as<UserInput>();

    return std::pair{
        201,
        json::o(
            "name", input.name,
            "age", input.age
        )
    };
});
```

---

## 9. ヘッダー操作

```cpp
app.get("/headers", [](Request req, Response res) {
    res.header("X-App", "Vix")
       .type("text/plain")
       .send("Hello headers");
});
```

---

## 10. リクエストスコープの状態管理

```cpp
app.get("/state", [](Request req, Response res) {
    req.set_state<int>(42);

    return json::o(
        "value", req.state<int>()
    );
});
```

---

## 11. Void ハンドラ

```cpp
app.get("/manual", [](Request req, Response res) {
    res.status(200)
       .json(json::o("ok", true));
});
```

---

## 12. Params マップへのアクセス

```cpp
app.get("/items/{id}", [](Request req, Response res) {
    const auto& params = req.params();
    return json::o("id", params.at("id"));
});
```

---

## 13. 204 No Content

```cpp
app.delete("/items/{id}", [](Request req, Response res) {
    res.status(204).send();
});
```

---

# 🧱 なぜ Vix が存在するのか

クラウドファーストなフレームワークは次を前提としています。

- 安定したネットワーク
- 予測可能なレイテンシ
- 常時オンライン接続

**しかし、世界の多くの環境ではそれは現実ではありません。**

Vix は次のために設計されています。

### ✔ オフラインファースト

インターネットがなくてもアプリは動作し続けます。

### ✔ ピアツーピア

中央サーバーなしで、ノード同士がローカル同期・通信可能。

### ✔ 超高速ネイティブ実行

C++20 + Asio + ゼロオーバーヘッド抽象化。

---

# 🧩 主な特徴

- 🌍 オフラインファーストランタイム
- 🔗 P2P 対応通信モデル
- ⚙️ 非同期 HTTP サーバー
- 🧭 表現力の高いルーティング
- 💾 MySQL / SQLite 用 ORM
- 🧠 ミドルウェアシステム
- 📡 WebSocket エンジン
- 🧰 モジュラー設計
- 🚀 Node / Deno / Bun に近い開発体験
- ⚡ 秒間 80,000 件超の処理性能

---

## 🚀 はじめ方

Vix.cpp をお使いの環境にセットアップするには、以下の手順を実行してください：

```bash
git clone https://github.com/vixcpp/vix.git
cd vix
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/hello_routes
```

---

## 🧰 CLI プロジェクト例

インストールが完了したら、CLI を使用して新しいプロジェクトを作成できます：

```bash
vix new myapp
cd myapp
vix build
vix run

vix dev file.cpp
vix run file.cpp
vix orm migrate
```

---

# 🎯 スクリプトモード — `.cpp` を直接実行

Vix は、単一の `.cpp` ファイルを スクリプトのように 実行できます。

```bash
vix run file.cpp
vix dev file.cpp
```

### 仕組み

- `./.vix-scripts/<ファイル名>/` に一時 CMake プロジェクトを生成
- `.cpp` を単体実行ファイルとしてコンパイル
- 即座に実行
- Ctrl+C でクリーンに終了（gmake のノイズなし）

### 実行例

```bash
~/myapp/test$ vix run server.cpp
Script mode: compiling server.cpp
Using script build directory:
  • .vix-scripts/server

✔ Build succeeded
[I] Server running on port 8080
^C
ℹ Server interrupted by user (SIGINT)
```

---

## 📚 ドキュメント

- 🧭 [Introduction](docs/introduction.md)
- ⚡ [Quick Start](docs/quick-start.md)
- 🧱 [Architecture & Modules](docs/architecture.md)
- 💾 [ORM Overview](docs/orm/overview.md)
- 📈 [Benchmarks](docs/benchmarks.md)
- 🧰 [Examples](docs/examples/overview.md)
- 🛠️ [Build & Installation](docs/build.md)
- ⚙️ [CLI Options](docs/options.md)

## 📦 モジュールドキュメント一覧

- 🧩 **Core Module** — [docs/modules/core.md](docs/modules/core.md)
- 📡 **WebSocket Module** — [docs/modules/websocket.md](docs/modules/websocket.md)
- 🗃️ **ORM Module** — [docs/modules/orm.md](docs/modules/orm.md)
- 🔧 **JSON Module** — [docs/modules/json.md](docs/modules/json.md)
- 🛠️ **Utils Module** — [docs/modules/utils.md](docs/modules/utils.md)
- 🧰 **CLI Module** — [docs/modules/cli.md](docs/modules/cli.md)
- ⚙️ **Rix Library (Essential C++ utilities)** — [docs/modules/rix.md](docs/modules/rix.md)

📊 サマリー

Vix.cpp は、Go Fiber に匹敵、あるいはそれを超える性能を持ち、
Deno、Node、PHP、Python、さらには Crow などの複数の C++ フレームワークをも上回る
モダンバックエンドランタイムの最前線に位置しています。

Vix.cpp = 境界を押し広げる C++ ランタイム

---

## 🤝 コントリビュート

コントリビューションは歓迎です。
詳細はコントリビューティングガイドをご覧ください。

## 🪪 ライセンス

**MIT License** で提供されています。
