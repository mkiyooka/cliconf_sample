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
# メインアプリケーション（add / multiply / subtract サブコマンドを持つ。詳細は後述）
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
- `include/`: ヘッダーファイル
    - `myproject/core/`: プロジェクト公開API
    - `config/`: cliconf config-system 向けの `Config` 構造体・スキーマ定義
- `config/`: 設定ファイルのサンプル（TOML / JSONC / YAML）
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
    GIT_TAG main
)
FetchContent_MakeAvailable(cliconf)
```

config-system は `ConfigManager<Config, Schema, ExtraLoader>` というヘッダオンリーの
テンプレートで提供されており、アプリ固有の `Config` 構造体を自由に定義できます。
このプロジェクトでは `include/config/config_loader.hpp`（`Config` 構造体）と
`include/config/config_schema.hpp`（`kConfigSchema`）を定義し、`src/app/main.cpp` で
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

`app` は `add` / `multiply` / `subtract` の3つのサブコマンドを持ち、`--mode` / `--timeout` /
`--config` はサブコマンド共通のオプションとして機能します。3つは設定ファイル連携の要否と
方法がそれぞれ異なり、cliconf の config-system が提供する3通りのマッピング方式に対応します。

| サブコマンド | 方式 | 設定ファイルからの読み込み | 実装 |
| --- | --- | --- | --- |
| `add` | CLIオンリー | 不可 | サブコマンドの位置引数のみ。`Config` には持たせない |
| `multiply` | 自動マッピング | 可（`[multiply]` セクション） | `kConfigSchema` に `FieldDescriptor{"--multiply.a", "multiply.a", ...}` を1行登録するだけ |
| `subtract` | 手動マッピング | 可（`[subtract]` セクション） | `Config::subtract` という入れ子構造体を `ExtraLoader`（`SubtractExtraLoader`）で手動読み込み |

自動/手動を分ける基準は「設定ファイル側のネストの深さ」ではなく、**`Config` 構造体側が
フラットかどうか**です。`FieldDescriptor` の第4引数（メンバーポインタ）は `Config` 直下の
1フィールドしか指せません。`multiply_a` / `multiply_b` や `network_retry_count` は `Config`
直下のフラットなメンバーなので、設定ファイル側が何階層ネストしていても
（`[multiply]` の `a`/`b`、`[network.retry]` の `count` など）`config_key` にドット区切りの
パスを書くだけで自動マッピングできます（`ResolveDottedKey` が段数に関わらず辿るため）。
一方 `subtract` の `a` / `b` は `SubtractConfig` という入れ子構造体のメンバーで、
`FieldDescriptor` は `&Config::subtract`（`SubtractConfig` 型全体）までしか指せず `a` / `b`
を個別に指せないため、`ExtraLoader` で TOML / JSONC / YAML のパース結果から手動で
読み出す必要があります。

いずれも優先度は CLI引数 > 設定ファイル > デフォルト値です。

```bash
# add: CLIオンリー
./build/app add 10 20
./build/app --mode production add 3 4

# multiply: 自動マッピング（kConfigSchema 経由でグローバルオプション --multiply.a / --multiply.b も使える）
./build/app multiply                                  # デフォルト値 (0, 0) を使用 -> 0
./build/app --config config/example.toml multiply     # 設定ファイルの [multiply] を使用 -> 42
./build/app --config config/example.toml --multiply.a 5 multiply # CLI引数が設定ファイルを上書き -> 35

# network.retry.count: 自動マッピング（2階層ネスト [network.retry] セクションの count）
./build/app                                            # デフォルト値 3
./build/app --config config/example.toml               # 設定ファイルの [network.retry] を使用 -> 5
./build/app --config config/example.toml --network.retry.count 9 # CLI引数が上書き -> 9

# subtract: 手動マッピング（ExtraLoader、CLI引数はサブコマンドの位置引数）
./build/app subtract                                  # デフォルト値 (0, 0) を使用 -> 0
./build/app --config config/example.toml subtract     # 設定ファイルの [subtract] を使用 -> 70
./build/app --config config/example.toml subtract 5 2 # CLI引数が設定ファイルを上書き -> 3
```

## GNU make

ninjaの代わりにmakeを利用したい場合は`CMakePresets.json`を以下のように修正してください。

```diff
-            "generator": "Ninja",
+            "generator": "Unix Makefiles",
```
