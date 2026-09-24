<table>
  <tr>
    <td valign="top" width="70%">

<h1>Vix.cpp</h1>

<p>
  <a href="https://x.com/vix_cpp">
    <img src="https://img.shields.io/badge/X-Follow-black?logo=x" />
  </a>
  <a href="https://www.youtube.com/@vixcpp">
    <img src="https://img.shields.io/badge/YouTube-Subscribe-red?logo=youtube" />
  </a>
</p>

<h3>モダン C++ で実用アプリケーションを構築する。</h3>

<p>
  Vix.cpp は、高速で信頼性が高く、本番運用を意識したアプリケーションを構築するための、モダン C++ ランタイムおよび開発者ツールキットです。
</p>

<p>
  <a href="https://vixcpp.com"><b>Website</b></a> ·
  <a href="https://docs.vixcpp.com"><b>Docs</b></a> ·
  <a href="https://rix.vixcpp.com"><b>Rix</b></a> ·
  <a href="https://registry.vixcpp.com"><b>Registry</b></a> ·
  <a href="https://blog.vixcpp.com"><b>Engineering notes</b></a>
</p>

</td>

<td valign="middle" width="13%" align="right">

<img
src="https://res.cloudinary.com/dwjbed2xb/image/upload/v1778607554/vix_logo_ms5lne.png"
width="150"
style="border-radius:50%; object-fit:cover;"
/>

</td>
  </tr>
</table>

---

[English](README.md)

Vix.cpp は、ネイティブ性能、明示的な制御、既存の C++ エコシステムとの互換性を保ちながら、C++ プロジェクトにモダンなアプリケーション開発ワークフローを提供します。

Vix.cpp は単なる Web フレームワークではありません。

Vix.cpp は、モダン C++ のためのアプリケーションランタイムおよび開発者プラットフォームです。

```txt
C++ source code
  -> Vix.cpp workflow
  -> CMake/Ninja when needed
  -> native executable or library
```

Vix.cpp は、プロジェクト作成、単一 C++ ファイルの実行、ターゲットビルド、テスト、フォーマット、依存関係管理、パッケージング、OpenAPI ドキュメント公開、本番サービス運用までを扱う統一されたコマンドサーフェスを提供します。

## なぜ Vix.cpp が存在するのか

C++ は強力で、移植性があり、成熟しており、高速です。

しかし多くの場合、難しいのは C++ 言語そのものではありません。難しいのは、アプリケーションの周辺にある仕組みです。

- プロジェクト構造
- ビルド設定
- 依存関係の準備
- 開発コマンド
- テスト実行
- 診断
- パッケージング
- ランタイム設定
- OpenAPI ドキュメント
- デプロイ
- 本番ログ
- ヘルスチェック
- サービス管理

本格的な C++ プロジェクトの多くは、最終的に同じ基盤を何度も作り直すことになります。

Vix.cpp は、その基盤を再利用可能で一貫したものにするために存在します。

目的は C++ を隠すことではありません。

目的は、実用的な C++ アプリケーションを作成し、実行し、テストし、パッケージ化し、ドキュメント化し、運用しやすくすることです。

## クイックスタート

`server.cpp` を作成します。

```cpp
#include <vix.hpp>

int main()
{
    vix::App app;

    app.get("/", [](vix::Request &, vix::Response &res) {
        res.text("Hello from Vix.cpp");
    });

    app.run();

    return 0;
}
```

実行します。

```bash
vix run server.cpp
```

開きます。

```txt
http://localhost:8080
```

## OpenAPI を 1 コマンドで有効化

API ドキュメントを公開するプロジェクトでは、実行時に docs を有効化できます。

```bash
vix run api --docs
```

次を開きます。

```txt
http://localhost:8080/docs
```

または、生成された OpenAPI ドキュメントを取得します。

```txt
http://localhost:8080/openapi.json
```

通常実行では docs は無効です。

```bash
vix run api
```

これにより、API ドキュメントの公開は明示的で環境を意識したものになります。

## OpenAPI ルートを手動で登録する

カスタムアプリケーションでは、OpenAPI とドキュメントルートをコードから登録できます。

```cpp
#include <vix.hpp>
#include <vix/openapi/register_docs.hpp>

int main()
{
    vix::App app;

    app.get("/status", [](vix::Request &, vix::Response &res) {
        res.text("OK");
    });

    vix::openapi::register_openapi_and_docs(
        *app.router(),
        "Example API",
        "1.0.0");

    app.run();

    return 0;
}
```

これにより、次のルートが登録されます。

```txt
GET /openapi.json
GET /docs
GET /docs/
GET /docs/index.html
GET /docs/swagger-ui.css
GET /docs/swagger-ui-bundle.js
```

ドキュメント UI は、埋め込み Swagger UI アセットを使ってローカルに提供されます。

## ルートにメタデータを付ける

OpenAPI メタデータは、ルートドキュメントとして定義できます。

```cpp
vix::router::RouteDoc doc;

doc.summary = "Service status";
doc.description = "Returns whether the service is running.";
doc.tags = {"system"};
doc.responses["200"] = {
    {"description", "OK"}
};
```

OpenAPI ジェネレータは、ルーターのメタデータと登録済みモジュールドキュメントからドキュメントを構築します。

```txt
router routes + registry docs
  -> OpenAPI 3.0.3 document
```

## プロジェクトワークフロー

新しいプロジェクトを作成します。

```bash
vix new api
cd api
vix install
vix dev
```

プロジェクトをビルドします。

```bash
vix build
```

実行します。

```bash
vix run
```

テストを実行します。

```bash
vix tests
```

プロジェクトを検証します。

```bash
vix check --tests
```

パッケージ化します。

```bash
vix pack
```

## Vix.cpp が提供するもの

Vix.cpp は 3 つのレイヤーを組み合わせています。

### 1. 開発者ワークフロー

Vix は C++ プロジェクトに一貫したコマンドサーフェスを提供します。

```bash
vix run main.cpp
vix new api
vix install
vix dev
vix build
vix tests
vix fmt
vix check
vix pack
```

すべてのプロジェクトで手作業のスクリプトを組み直す代わりに、Vix は一般的な開発ライフサイクルに予測可能な形を与えます。

### 2. ランタイムモジュール

Vix はアプリケーション開発のための再利用可能なランタイムモジュールを提供します。

```txt
agent        async        cache        cli          conversion
core         crypto       db           env          error
fs           game         io           json         kv
log          middleware   net          orm          os
p2p          p2p_http     path         process      reply
sync         template     tests        threadpool   time
utils        validation   webrpc       websocket
```

これらのモジュールは、小さなサンプルだけではなく、実用アプリケーションを支えるために設計されています。

### 3. ネイティブ C++ 統合

Vix は C++ ツールチェーンを置き換えません。

Vix は、ネイティブ C++、CMake、Ninja、コンパイラ、リンカ、通常の実行ファイルやライブラリ出力と連携します。

```txt
C++:
  language, performance, native binaries

CMake and Ninja:
  build system and ecosystem compatibility

Vix.cpp:
  application workflow, runtime modules, CLI, diagnostics, project lifecycle
```

## 何を構築できるか

Vix.cpp は次のような用途に使えます。

- バックエンドサービス
- HTTP API
- OpenAPI ドキュメント付き API
- WebSocket アプリケーション
- コマンドラインツール
- 再利用可能な C++ ライブラリ
- ローカルファーストアプリケーション
- P2P システム
- AI エージェントワークフロー
- ゲーム向けプロジェクト
- 本番サービス

## インストール

Linux と macOS:

```bash
curl -fsSL https://vixcpp.com/install.sh | bash
```

Windows PowerShell:

```powershell
irm https://vixcpp.com/install.ps1 | iex
```

その他のインストール方法:

```txt
https://vixcpp.com/install
```

## 単一 C++ ファイルを実行する

単一ファイルモードは、Vix.cpp を試す最短ルートです。

`main.cpp` を作成します。

```cpp
#include <vix.hpp>

int main()
{
    vix::print("Hello from Vix.cpp");
    return 0;
}
```

実行します。

```bash
vix run main.cpp
```

Vix はソースファイルを検出し、ネイティブ C++ ツールチェーンでビルドし、生成されたプログラムを実行します。

このモードは次の用途に向いています。

- 実験
- サンプル
- 小さなツール
- Vix API の学習
- 短い C++ スニペットの検証

コードが複数ファイル、テスト、依存関係、安定したアプリケーションへ成長したら、Vix プロジェクトへ移行します。

## プロジェクトを作成する

新しいアプリケーションを作成します。

```bash
vix new hello --app
cd hello
vix build
vix run
```

生成されるプロジェクトは通常、次のような構造になります。

```txt
hello/
├── src/
│   └── main.cpp
├── tests/
│   └── test_basic.cpp
├── .env.example
├── vix.app
├── vix.json
└── README.md
```

この構造は意図的に小さく保たれています。

複雑な構成を最初から強制せず、アプリケーションが成長するために必要な最低限の整理を提供します。

## 開発モード

アクティブな開発中は `vix dev` を使います。

```bash
vix dev
```

開発ループは次のようになります。

```txt
edit
save
detect change
rebuild
restart
continue
```

コンパイルだけを確認したい場合:

```bash
vix build
```

アプリケーションを手動で起動したい場合:

```bash
vix run
```

コードを編集しながら開発する場合:

```bash
vix dev
```

## ビルドシステムモデル

Vix は 2 つのプロジェクトモデルをサポートします。

```txt
CMakeLists.txt
vix.app
```

解決順序:

```txt
1. CMakeLists.txt
2. vix.app
```

`CMakeLists.txt` が存在する場合、Vix はそれを使用します。

`CMakeLists.txt` がなく、`vix.app` が存在する場合、Vix はマニフェストを読み込み、内部 CMake プロジェクトを生成します。

```txt
Project with CMakeLists.txt
  -> Vix uses the existing CMake project

Project with vix.app
  -> Vix generates an internal CMake project
  -> Vix builds the target
```

これにより、高度な CMake プロジェクトは主導権を保ちつつ、より単純なアプリケーションには読みやすいマニフェストワークフローを提供できます。

## ビルド例

現在のプロジェクトをビルドします。

```bash
vix build
```

詳細出力付きでビルドします。

```bash
vix build -v
```

Release ビルドを行います。

```bash
vix build --preset release
```

特定ターゲットをビルドします。

```bash
vix build --build-target vix
```

すべてをビルドします。

```bash
vix build --build-target all
```

単一 C++ ファイルをビルドします。

```bash
vix build main.cpp
```

## 実行ターゲット

`vix run` は Web アプリケーションに限定されません。

次のものを実行できます。

- 現在のプロジェクト
- 名前付きプロジェクトまたはターゲット
- 単一 C++ ファイル
- `.vix` マニフェスト
- コンパイル済みバイナリ
- Docker イメージ
- コンテナイメージ
- SSH ターゲット
- HTTP または HTTPS ターゲット
- Vix umbrella examples

例:

```bash
vix run
vix run api
vix run main.cpp
vix run app.vix
vix run ./app
vix run docker://nginx
vix run ssh://user@host
vix run https://example.com
```

## コマンドマップ

| Command            | Purpose                                |
| ------------------ | -------------------------------------- |
| `vix run main.cpp` | 単一 C++ ファイルを実行する            |
| `vix new <name>`   | 新しいプロジェクトを作成する           |
| `vix install`      | プロジェクト依存関係をインストールする |
| `vix dev`          | 開発モードを開始する                   |
| `vix build`        | 設定してビルドする                     |
| `vix run`          | 必要ならビルドして実行する             |
| `vix tests`        | テストを実行する                       |
| `vix fmt`          | ソースファイルをフォーマットする       |
| `vix check`        | プロジェクトを検証する                 |
| `vix pack`         | プロジェクトをパッケージ化する         |
| `vix registry`     | レジストリメタデータを管理する         |
| `vix replay`       | 記録済み実行を再現する                 |
| `vix agent`        | AI 支援開発ワークフローを実行する      |
| `vix game export`  | ゲームプロジェクトをエクスポートする   |

## アーキテクチャ

Vix.cpp はモジュラーなフレームワークとして構成されています。

```txt
vix/
├── modules/
│   ├── agent/
│   ├── async/
│   ├── cache/
│   ├── cli/
│   ├── core/
│   ├── crypto/
│   ├── db/
│   ├── env/
│   ├── error/
│   ├── fs/
│   ├── game/
│   ├── io/
│   ├── json/
│   ├── kv/
│   ├── log/
│   ├── middleware/
│   ├── net/
│   ├── orm/
│   ├── p2p/
│   ├── p2p_http/
│   ├── path/
│   ├── process/
│   ├── reply/
│   ├── sync/
│   ├── template/
│   ├── tests/
│   ├── threadpool/
│   ├── time/
│   ├── utils/
│   ├── validation/
│   ├── websocket/
│   └── webrpc/
├── docs/
├── tests/
└── benchmarks/
```

各モジュールは独立して進化でき、同時に同じアプリケーションプラットフォームの一部として機能します。

## HTTP サーバー例

```cpp
#include <vix.hpp>

int main()
{
    vix::App app;

    app.get("/", [](vix::Request &, vix::Response &res) {
        res.text("Hello from Vix.cpp");
    });

    app.get("/health", [](vix::Request &, vix::Response &res) {
        res.text("OK");
    });

    app.run();

    return 0;
}
```

実行します。

```bash
vix run server.cpp
```

テストします。

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/health
```

## API ドキュメントワークフロー

プロフェッショナルな API は、検査可能であるべきです。

Vix は、明示的に有効化できる OpenAPI ドキュメントルートをサポートします。

開発時に docs を有効化して実行します。

```bash
vix run api --docs
```

通常実行では docs を無効にします。

```bash
vix run api
```

明示的に docs を無効化します。

```bash
vix run api --no-docs
```

環境変数でも制御できます。

```bash
VIX_DOCS=1 vix run api
VIX_DOCS=0 vix run api
```

推奨される本番ポリシー:

```txt
disable docs completely
protect /docs behind auth
serve docs only in internal environments
serve /openapi.json only in CI or staging
```

## レジストリとパッケージ

Vix は、パッケージベースの C++ プロジェクト向けにレジストリワークフローを提供します。

```bash
vix registry sync
vix add <package>
vix install
```

レジストリモデル:

```txt
Registry index       -> package metadata
Store                -> fetched package Git checkouts
vix.json             -> declared project dependency requirements
vix.lock             -> exact resolved dependency versions
.vix/deps            -> project-local dependency links or copies
.vix/vix_deps.cmake  -> generated dependency integration
```

レジストリは、パッケージメタデータをプロジェクトソースから分離します。

## Rix

Rix は、Vix.cpp の統一された userland ライブラリレイヤーです。

Vix が提供するもの:

```txt
runtime
CLI
build workflow
registry integration
core modules
```

Rix が提供するもの:

```txt
optional userland libraries
a unified facade
independent packages
```

統一 facade をインストールします。

```bash
vix add @rix/rix
vix install
```

使用例:

```cpp
#include <rix.hpp>

int main()
{
    rix.debug.print("Hello", "Rix");

    auto table = rix.csv.parse("name,language\nAda,C++\n");

    rix.debug.log("loaded {} rows", table.size());

    return 0;
}
```

## 本番ワークフロー

Vix は、本番サービスを検査可能で運用しやすくするために設計されています。

本番向けワークフローの例:

```bash
vix build --preset release
vix service install
vix service start
vix service status
vix service logs
vix proxy nginx check
vix doctor production
```

目的は、本番状態を見えるようにすることです。

```txt
App status
Binary path
Service status
HTTP port
WebSocket port
Public URL
Proxy state
TLS state
Healthcheck result
Logs
```

C++ アプリケーションは、デプロイ後にブラックボックスになるべきではありません。

## 失敗した実行を再現する

`vix replay` は、記録済みの実行を再現するための機能です。

実行を記録します。

```bash
vix run api --replay
```

最後に記録された実行を再現します。

```bash
vix replay last
```

最後に失敗した実行を再現します。

```bash
vix replay failed
```

これは、実行が失敗した、クラッシュした、または予期しない挙動をした場合に、同じコンテキストを再現したいときに役立ちます。

## AI 支援ワークフロー

Vix は、ローカルプロジェクト解析や開発支援のための AI エージェントコマンドを提供します。

```bash
vix agent ask "Explain this project"
vix agent analyze .
vix agent scan .
```

エージェントは許可されたワークスペースを検査し、プロジェクト構造を要約し、開発タスクを支援できます。

## ゲームプロジェクト

Vix は `vix game` 名前空間を通じて、ゲーム向けツールを提供します。

```bash
vix game export
```

これにより、ゲームプロジェクトをアセットとメタデータ付きの配布用ディレクトリへエクスポートできます。

## ベンチマーク

Vix.cpp は、性能を単なるマーケティング文句ではなく、エンジニアリング上の関心事として扱います。

Core モジュールには、リリース間の性能を追跡し、マージまたはリリース前にリグレッションを検出するための公式ベンチマークスイートが含まれています。

Benchmark source:

- [Core benchmarks](https://github.com/vixcpp/vix/tree/main/modules/core/benchmarks)
- [Core benchmarks README](https://github.com/vixcpp/vix/blob/main/modules/core/benchmarks/README.md)
- [v2.6.3 benchmark baseline](https://github.com/vixcpp/vix/tree/main/modules/core/benchmarks/baselines/v2.6.3)
- [Engineering note: Vix Core v2.6.3 benchmark baseline](https://blog.vixcpp.com/posts/vix-core/vix-core-benchmark-baseline-v263)

例:

```bash
vix build --build-target benchmarks
```

Core ベンチマーク領域:

```txt
runtime.task
runtime.queue
runtime.scheduler
runtime.worker

executor.submit
executor.post
executor.metrics

router.match
router.registration

http.request
http.response

session.fake_transport

app.route_registration
app.group_registration
```

公式 Core benchmark baseline は次に保存されています。

```txt
modules/core/benchmarks/baselines/v2.6.3/
```

Baseline files:

```txt
core_app_group_registration_bench.json
core_app_route_registration_bench.json
core_executor_metrics_bench.json
core_executor_post_bench.json
core_executor_submit_bench.json
core_http_request_bench.json
core_http_response_bench.json
core_router_match_bench.json
core_router_registration_bench.json
core_runtime_queue_bench.json
core_runtime_scheduler_bench.json
core_runtime_task_bench.json
core_runtime_worker_bench.json
core_session_fake_transport_bench.json
```

公式ベンチマーク値は Release ビルドから生成する必要があります。

```txt
dev/debug = compile, test, debug
release   = measure performance
```

`modules/core` からベンチマークスイートをビルドします。

```bash
vix build --clean --preset release --build-target all -v -- \
  -DVIX_CORE_BUILD_BENCHMARKS=ON \
  -DVIX_CORE_BUILD_TESTS=ON
```

単一ベンチマークを実行します。

```bash
./build-release/benchmarks/core/core_router_match_bench
```

Core benchmark suite 全体を実行します。

```bash
./scripts/run_core_benchmarks.sh \
  --bin-dir build-release/benchmarks/core \
  --out-dir benchmarks/results/dev \
  --version dev \
  --runner local \
  --machine local
```

公式 baseline と比較します。

```bash
./scripts/compare_core_benchmarks.py \
  benchmarks/baselines/v2.6.3 \
  benchmarks/results/current
```

デフォルトでは、benchmark comparison は次を使用します。

```txt
median_ops_per_sec
```

値が高いほど良い結果です。

Default thresholds:

```txt
WARN = -5%
FAIL = -10%
```

## エンジニアリング原則

Vix.cpp は次の原則に基づいて構築されています。

- ネイティブ C++ を保つ
- ツールチェーンを隠さない
- CMake と競合せず連携する
- 一般的なワークフローを再利用可能にする
- 必要なときに OpenAPI で API を公開する
- 診断を読みやすくする
- 本番環境を検査可能にする
- モジュールを組み合わせ可能にする
- 性能を推測ではなく測定する
- 隠れた魔法より明示的な API を優先する
- 単純なプロジェクトは簡単に、複雑なプロジェクトは可能にする

## Vix.cpp ではないもの

Vix.cpp は C++ を置き換えません。

C++ をスクリプト言語に変えるものではありません。

高度なビルドシステムにおける CMake を置き換えるものではありません。

すべてのプロジェクトに単一の構造を強制するものではありません。

すべての環境でドキュメントをデフォルト公開するものではありません。

Vix.cpp は、ネイティブ制御を保ちながら、C++ アプリケーションに一貫したワークフローとモジュラーなランタイム基盤を提供します。

## プロジェクト状態

Vix.cpp は現在も積極的に開発されています。

一部の領域は安定しており、実用的なアプリケーション開発に使用できます。

一部の領域はまだ進化中であり、本番利用前に慎重に評価する必要があります。

このプロジェクトは次を重視して開発されています。

- 実用アプリケーションワークフロー
- テスト
- ベンチマーク
- OpenAPI ドキュメント
- 本番診断
- モジュラー設計

## ドキュメント

Start here:

- [Website](https://vixcpp.com)
- [Documentation](https://docs.vixcpp.com)
- [Registry](https://registry.vixcpp.com)
- [Rix](https://rix.vixcpp.com)
- [Engineering notes](https://blog.vixcpp.com)

Recommended path:

1. Vix.cpp をインストールする
2. 最初の C++ ファイルを実行する
3. 最初のプロジェクトを作成する
4. 最初の HTTP サーバーを構築する
5. ローカルで OpenAPI docs を有効化する
6. CLI コマンドを学ぶ
7. ランタイムモジュールを探索する
8. レジストリからパッケージを追加する
9. ローカル開発から本番運用へ進む

## Vix.cpp で構築されたもの

| Project            | Purpose                                              |
| ------------------ | ---------------------------------------------------- |
| Rix                | Vix.cpp の統一 userland ライブラリレイヤー           |
| Cnerium            | Vix 上に構築された durable backend reliability layer |
| Vix Game           | ゲーム向けランタイムプロジェクト                     |
| PulseGrid          | 本番スタイルのサービス監視プロジェクト               |
| Softadastra Runner | Vix.cpp で構築されたコマンドランナー                 |

## コントリビュート

コントリビューションは歓迎です。

Pull Request を開く前に実行してください。

```bash
vix fmt --check
vix check --tests
vix build --preset release
```

性能に関わるコードを変更する場合は、ベンチマークスイートを実行し、現在の baseline と比較してください。

## ライセンス

MIT License.
