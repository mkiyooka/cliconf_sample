# C++ Project Template

モダンC++開発のためのクロスプラットフォーム対応テンプレートプロジェクトです。CMake FetchContentを使用してライブラリを管理しています。

## プロジェクト構成

- **ビルドシステム**: CMake 4.1.1 + Ninja
- **環境管理**: Pixi (クロスプラットフォーム対応)
- **テストフレームワーク**: doctest
- **C++標準**: C++17

## 必要条件

このプロジェクトではpixiを利用します。

- [pixi](https://prefix.dev/)

### pixi のインストール

```bash
# Linux / macOS
curl -fsSL https://pixi.sh/install.sh | bash
# インストール後にシェルを再起動するか、以下を実行する
source ~/.bashrc   # bash の場合
source ~/.zshrc    # zsh の場合
```

## セットアップ

```bash
# Pixi環境のインストール
pixi install

# CMake設定とビルド
pixi run config
pixi run build

# テスト実行
pixi run test
```

## 実行

```bash
# メインアプリケーション（serve / connect / nodes サブコマンドを持つ。詳細は後述）
./build/app --help

# テスト個別実行
./build/tests/test_core
```

## 開発ツール

```bash
# コードフォーマット
pixi run format

# 静的解析
pixi run lint

# 全チェック実行
pixi run fullcheck
```

## サニタイザ（Linux）

AddressSanitizer と UndefinedBehaviorSanitizer を有効にしてテストを実行します。

```bash
pixi run asan
```

## カバレッジ（Linux）

Clang のソースベースカバレッジを使用してレポートを生成します。

```bash
pixi run coverage
```

HTML レポートは `build-coverage/coverage-html/index.html` に生成されます。

### カバレッジレポートの対象

カバレッジレポートには **プロダクションコードのみ** を含めることが推奨されます。

**テストコードを除外する方法（フィルタ）:**

`cmake/coverage.cmake` の `--ignore-filename-regex` オプションで制御します。

```cmake
"--ignore-filename-regex=.*/build-coverage/.*|.*/third_party/.*|.*/.pixi/.*|.*/tests/.*"
```

| パターン | 除外対象 |
| --- | --- |
| `.*/build-coverage/.*` | ビルド生成物 |
| `.*/third_party/.*` | FetchContent で取得したサードパーティライブラリ |
| `.*/.pixi/.*` | pixi 環境のヘッダー |
| `.*/tests/.*` | テストコード |

**カバレッジ対象に含める方法:**

上記 `--ignore-filename-regex` から除外したいパスのパターンを削除してください。
たとえばテストコードもレポートに含めたい場合は `.*/tests/.*` を削除します。

## valgrind（Linux）

valgrind はシステムの apt で導入してください（pixi 依存には含まれません）。

```bash
sudo apt install valgrind
```

導入後は以下で実行できます。

```bash
pixi run valgrind
```

## ディレクトリ構成

- `src/`: ソースコード
    - `core/`: 共通ロジック
    - `app/`: 実行ファイル
    - `config/`: `config_validator.cpp`（バリデーション実装）
- `include/`: ヘッダーファイル
    - `myproject/core/`: プロジェクト公開API
    - `config/`: cliconf config-system 向けの `Config` 構造体・スキーマ・バリデーション定義
- `config/`: 設定ファイルのサンプル（TOML / JSONC / YAML）、`nodes` サブコマンド用の CSV サンプル
- `tests/`: テストコード
- `cmake/`: CMake設定ファイル
    - `local-or-fetch.cmake`: FetchContentヘルパー
    - `dependencies-app.cmake`: アプリ用ライブラリ
    - `dependencies-test.cmake`: テスト用ライブラリ
    - `custom-targets.cmake`: カスタムターゲット
    - `quality-setup.cmake`: コード品質設定
    - `quality-tools.cmake`: コード品質ツール

## cliconf（config-system）の導入

[cliconf](https://github.com/mkiyooka/cliconf) を FetchContent で取り込み、TOML / JSONC / YAML 設定ファイルと
CLI11 を統合する config-system を利用しています。設定は `cmake/dependencies-app.cmake` に記述しています。

```cmake
add_external_package(cliconf ext/cliconf
    GIT_REPOSITORY https://github.com/mkiyooka/cliconf.git
    GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(cliconf)
```

cliconf v0.1.0 では `ConfigManager::Resolve()` は例外を投げず、`config::LoadResult<T>`
（`compat::expected<T, config::LoadError>`）を返します。設定ファイルが開けない・構文エラー・
型不一致などは `LoadError` として返り、`Format()` で `"<file>: key '<key>': <message>"` 形式の
文字列が得られます。`code`（`config::LoadErrc`: `IoError` / `ParseError` / `TypeMismatch` /
`UnsupportedFormat` / `AmbiguousDefault` / `ManifestCycle`）で分類も判定できます。

```cpp
const auto resolved = config_manager.Resolve(config_files);
if (!resolved) {
    fmt::print(stderr, "Error: {}\n", resolved.error().Format());
    return 1;
}
// *resolved / resolved->field で値にアクセスする
```

ExtraLoader（利用者側のコード）が投げた例外はローダで捕捉されずそのまま伝播するため、
このサンプルでは `main()` で `std::exception` を受け止めて `Fatal:` と終了コード 2 で終えています。

config-system は `ConfigManager<Config, Schema, ExtraLoader>` というヘッダオンリーの
テンプレートで提供されており、アプリ固有の `Config` 構造体を自由に定義できます。
このプロジェクトでは `include/config/config_loader.hpp`（`Config` 構造体群）と
`include/config/config_schema.hpp`（スキーマ・`ExtraLoader`）を定義し、`src/app/main.cpp` で
`ConfigManager` を組み立てています。

```cmake
target_link_libraries(app
    PRIVATE CLI11::CLI11
    PRIVATE fmt::fmt
    PRIVATE tomlplusplus::tomlplusplus
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE fkYAML_target
    PRIVATE cliconf::cliconf
)
```

`cliconf::config`（cliconf本体のサンプル用ターゲット、`config_validator.cpp` を含む）は
アプリ固有の `Config` とは別物のためリンクしません。ヘッダオンリー部分だけを使う
`cliconf::cliconf` をリンクし、CLI11 / toml++ / nlohmann_json / fkYAML は
config-system が内部でインクルードするため個別にリンクする必要があります。

### 設定の階層と対応する `ConfigManager`

設定ファイルには、性質の異なる値が混在しがちです。このサンプルではそれぞれに
対応する構造体・`ConfigManager` を分けています。計算処理などの機能は持たせず、
設定値を構造体に読み込んで表示するだけの最小構成です。

| 種類 | 設定ファイルの例 | 対応する構造体 | `ConfigManager` を登録する `CLI::App` |
| --- | --- | --- | --- |
| アプリ全体設定 | `[app]` の `log_level` / `log_output` | `Config`（`kConfigSchema`） | ルートの `app`（トップレベル `--help` に常に表示） |
| モジュール単位設定 | `[cluster]` の `name` / `node_count` | 同上 | 同上 |
| サブコマンド固有設定（自動マッピング） | `[serve]` の `host` / `port` / `workers` | `ServeConfig`（`kServeSchema`） | `serve` サブコマンドの `CLI::App` のみ |
| サブコマンド固有設定（手動マッピング） | `[connect]` の `endpoint` / `timeout_ms` / `retry` | `ConnectConfig`（`kConnectSchema` + `ConnectExtraLoader`） | `connect` サブコマンドの `CLI::App` のみ |
| サブコマンド固有設定（CSVを設定として扱う） | `[nodes]` の `csv`（CSVファイルパス） | `NodesConfig`（`kNodesSchema` + `NodesExtraLoader::LoadCsv`） | `nodes` サブコマンドの `CLI::App` のみ |

`ConfigManager::RegisterOptions()` はどの `CLI::App` に対して呼ぶかで、生成される
オプションの所属スコープが決まります。ルートの `app` に登録すればグローバルオプションに、
`app.add_subcommand(...)` が返す `CLI::App*` に登録すればそのサブコマンド専用の
オプションになり、トップレベルの `--help` には出ません。

```cpp
// アプリ全体設定はルートに登録 -> トップレベル --help に表示される
config::ConfigManager<Config, decltype(config::kConfigSchema)> config_manager{config::kConfigSchema};
config_manager.RegisterOptions(app);

// serve 固有の設定は serve サブコマンドに登録 -> serve --help にのみ表示される
CLI::App *serve = app.add_subcommand("serve", "...");
config::ConfigManager<ServeConfig, decltype(config::kServeSchema)> serve_config_manager{config::kServeSchema};
serve_config_manager.RegisterOptions(*serve);
```

### サブコマンド: `serve` / `connect` / `nodes`

- `serve`: サーバー起動を模したサブコマンド。`host`/`port`/`workers` はいずれも
  `ServeConfig` のフラットなメンバーなので、そのまま `kServeSchema`（`Owner = ServeConfig`）
  に登録するだけで自動マッピングされます。
- `connect`: リモート接続を模したサブコマンド。`endpoint`/`timeout_ms` は自動マッピング
  しますが、リトライ設定 `retry`（`RetryConfig`、`count`/`interval_ms` を持つ入れ子構造体）は
  スキーマの自動マッピング対象外（`FieldDescriptor` はスカラー値1個しか扱えず、
  `toml::table::value<T>()` 等は集約型を受け付けないため）なので、`ConnectExtraLoader`
  で手動読み込みします。`Resolve()` の戻り値にはスキーマ外フィールドは含まれないため、
  `GetFileValues().retry` から明示的に取得してマージする必要がある点に注意してください。
- `nodes`: クラスタのノード一覧を CSV ファイルから読み込むサブコマンド。CSV ファイルパス
  `nodes_csv` は `NodesConfig` のフラットなメンバーなので `kNodesSchema` に登録するだけで
  自動マッピングされ、CLI（`--nodes-csv`）や設定ファイルの `[nodes]` セクションで
  切り替えられます。ノード一覧 `nodes`（`std::vector<NodeRecord>`）はスキーマの自動
  マッピング対象外のため、`NodesExtraLoader::LoadCsv` が `nodes_csv` の指す CSV
  ファイルを読み込んで書き込みます。`LoadCsv` は他の `Load{Toml,Json,Yaml}` と異なり
  `ConfigManager::Resolve()` の最後（スキーマ・CLI 解決が完了した後）に一度だけ
  呼ばれるため、`nodes_csv` は CLI 引数や設定ファイルで上書きされた最終的な値が
  確定した状態で読み込めます。また `Resolve()` の戻り値に直接書き込まれるため、
  `connect` の `retry` と異なり `GetFileValues()` からの手動マージは不要です。

いずれのサブコマンドも計算や通信は一切行わず、設定値を検証（後述）した上でそのまま
出力するだけです。優先度は CLI引数 > 設定ファイル > デフォルト値です。

```bash
# serve: デフォルト値を使用
./build/app serve

# 設定ファイルの [serve] セクション(host/port/workers)を使用
./build/app --config config/example.toml serve

# serve サブコマンド固有のオプションで port を上書き（CLI引数が最優先）
./build/app --config config/example.toml serve --port 12345

# connect: デフォルト値を使用
./build/app connect

# 設定ファイルの [connect] / [connect.retry] セクションを使用
./build/app --config config/example.toml connect

# nodes: CLI引数で CSV ファイルパスを指定
./build/app nodes --nodes-csv config/nodes.csv

# 設定ファイルの [nodes] セクション(csv)経由でパスを指定
./build/app --config config/example.toml nodes
```

### `--help` の階層

`serve`/`connect`/`nodes` 固有のオプション（`--host`/`--port`/`--workers`、
`--endpoint`/`--timeout-ms`、`--nodes-csv`）は、対応する
`ConfigManager::RegisterOptions()` をそのサブコマンドの `CLI::App` にのみ
呼んでいるため、トップレベルの `--help` には出ません。

```text
$ ./build/app --help
OPTIONS:
  -h,     --help              Print this help message and exit
  -c,     --config TEXT ...   Configuration file(s)
          --log-level TEXT    Log level (debug/info/warn/error)
          --log-output TEXT   Log output destination (stdout/file)
          --cluster.name TEXT Cluster name
          --cluster.node-count INT
                              Number of nodes in the cluster

SUBCOMMANDS:
  serve                       Start the server
  connect                     Connect to a remote endpoint
  nodes                       List cluster nodes loaded from a CSV file

$ ./build/app serve --help
OPTIONS:
  -h,     --help              Print this help message and exit
          --host TEXT         Bind address
          --port INT          Listen port
          --workers INT       Number of worker threads

$ ./build/app nodes --help
OPTIONS:
  -h,     --help              Print this help message and exit
          --nodes-csv TEXT    Path to a CSV file listing cluster nodes
```

### バリデーション（`include/config/config_validator.hpp`）

`ConfigManager::Resolve()` はスキーマの型（`int`/`double`/`std::string` 等）や
ファイルのパース可否は検証しますが、`port` が有効な範囲か・`endpoint` が空でないかと
いった「値の意味」までは検証しません。この種のバリデーションは `Resolve()` の後、
アプリ側で行う必要があります。このサンプルでは2つのパターンを実演しています。

| パターン | 対象 | 型 | 特徴 |
| --- | --- | --- | --- |
| 同一型を検証する `Validate` 関数 | `serve` | `ValidateServeConfig(const ServeConfig&) -> compat::expected<void, std::string>` | cliconf 本体の `Validate(const Config&)` と同じ形。成功なら値なし、失敗なら `unexpected` にメッセージ。実装コストが低いが、`Validate()` の呼び忘れを型では防げない |
| `Raw` → `Parsed` の変換 | `connect` | `ParsedConnectConfig::Parse(const ConnectConfig&) -> compat::expected<ParsedConnectConfig, std::string>` | 検証を通過しない限り `ParsedConnectConfig` を作れない（コンストラクタが非公開）。以降のコードは「検証済みの値」であることを型で保証された状態で扱える |

```cpp
// パターン1: serve
if (const auto valid = config::ValidateServeConfig(serve_conf); !valid) {
    fmt::print(stderr, "Error: {}\n", valid.error());
    return 1;
}

// パターン2: connect（cliconf::cliconf が提供する compat::expected を使用）
const auto parsed = config::ParsedConnectConfig::Parse(connect_conf);
if (!parsed.has_value()) {
    fmt::print(stderr, "Error: {}\n", parsed.error());
    return 1;
}
fmt::print("connect.endpoint: {}\n", parsed->Endpoint());
```

```bash
./build/app serve --port 99999    # Error: serve.port must be in [1, 65535], got 99999
./build/app connect --endpoint "" # Error: connect.endpoint must not be empty
```

## GNU make

ninjaの代わりにmakeを利用したい場合は`CMakePresets.json`を以下のように修正してください。

```diff
-            "generator": "Ninja",
+            "generator": "Unix Makefiles",
```
