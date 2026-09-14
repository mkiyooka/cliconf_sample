# Zed Local Configuration for Remote Development

Zedのremote開発用設定を説明します。

## Remote側設定（このディレクトリ）

- `settings.json`: LSP（clangd）、言語設定、ファイル除外設定
- `tasks.json`: CMake/pixiタスク（remote側で実行）

## Local側設定（ユーザーグローバル設定）

以下の設定は、ローカルマシンの `~/.config/zed/settings.json` に追加してください。

### デバッグ設定

プロジェクトルートの `.zed/settings.json` に以下を追加（またはグローバル設定）:

```json
{
    "terminal": {
        "copy_on_select": true
    }
}
```

デバッグ設定は、Zedのコマンドパレット（Cmd+Shift+P）から "debug: ..." で実行できます。

### CodeLLDBアダプタ設定（ローカル開発時のみ）

ローカル開発時にデバッグを使用する場合は `.zed/debug.json` を使用してください:

- `build/cmd`: メインの実行バイナリ
- `build/tests/test_config_manager`: ConfigManager のテスト
- `build/tests/test_doctest_usage`: doctest サンプルのテスト
- `build/tests/test_yyjson_wrapper`: yyjson ラッパーのテスト

**注意**: `debug.json` は `.gitignore` に追加することをお勧めします。

## Remote開発のワークフロー

1. Zedで "Remote: Connect to Server" を実行
2. SSH接続設定を入力
3. プロジェクトを開く
4. タスク実行: Cmd+Shift+P -> "task: spawn"
5. デバッグは通常remote側では使用しません（必要に応じてlocal側で設定）

## Local開発のワークフロー

通常のZedの使用方法と同じです:

1. プロジェクトを開く
2. タスク実行: Cmd+Shift+P -> "task: spawn"
3. デバッグ実行: Cmd+Shift+P -> "debug: ..." （debug.jsonが存在する場合）
