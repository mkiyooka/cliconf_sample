#include <exception>
#include <vector>

#include <CLI/CLI.hpp>
#include <fmt/base.h>

#include "config/config_manager.hpp"
#include "config/config_schema.hpp"
#include "config/config_validator.hpp"

namespace {

// serve: ServeConfig 専用の ConfigManager で host/port/workers を自動マッピングする。
// RegisterOptions() をこの serve サブコマンドの CLI::App にだけ呼ぶことで、
// --host/--port/--workers はトップレベルの --help には出ない。
CLI::App *
SetupServeCommand(CLI::App &app, config::ConfigManager<ServeConfig, decltype(config::kServeSchema)> &config_manager) {
    CLI::App *serve = app.add_subcommand("serve", "Start the server");
    config_manager.RegisterOptions(*serve);
    return serve;
}

// connect: endpoint/timeout_ms は自動マッピング(kConnectSchema)。retry(RetryConfig)は
// スキーマの自動マッピング対象外のため ConnectExtraLoader で手動読み込みする
// (手動マッピング)。RegisterOptions() をこの connect サブコマンドの CLI::App にだけ
// 呼ぶことで、--endpoint/--timeout-ms はトップレベルの --help には出ない。
CLI::App *SetupConnectCommand(
    CLI::App &app,
    config::ConfigManager<ConnectConfig, decltype(config::kConnectSchema), config::ConnectExtraLoader> &config_manager
) {
    CLI::App *connect = app.add_subcommand("connect", "Connect to a remote endpoint");
    config_manager.RegisterOptions(*connect);
    return connect;
}

// nodes: nodes_csv は自動マッピング(kNodesSchema)。nodes(std::vector<NodeRecord>)は
// NodesExtraLoader::LoadCsv が nodes_csv の指す CSV ファイルを読み込んで書き込む
// (CSVを設定の一部として扱う例)。LoadCsv は Resolve() の最後に呼ばれ、結果は
// Resolve() の戻り値に直接反映されるため、subtract/connect の retry のように
// GetFileValues() から手動マージする必要はない。
CLI::App *SetupNodesCommand(
    CLI::App &app,
    config::ConfigManager<NodesConfig, decltype(config::kNodesSchema), config::NodesExtraLoader> &config_manager
) {
    CLI::App *nodes = app.add_subcommand("nodes", "List cluster nodes loaded from a CSV file");
    config_manager.RegisterOptions(*nodes);
    return nodes;
}

} // namespace

int RunApp(int argc, char *argv[]) {
    CLI::App app{"cliconf config-system sample"};
    app.require_subcommand(0, 1);

    std::vector<std::string> config_files;
    app.add_option("-c,--config", config_files, "Configuration file(s)");

    // アプリ全体([app])・モジュール単位([cluster])の設定は Config 直下の
    // フラットなメンバーなので kConfigSchema に自動マッピングされ、トップレベルの
    // --help に常に表示される(serve/connect 固有のオプションとは異なる)。
    config::ConfigManager<Config, decltype(config::kConfigSchema)> config_manager{config::kConfigSchema};
    config_manager.RegisterOptions(app);

    config::ConfigManager<ServeConfig, decltype(config::kServeSchema)> serve_config_manager{config::kServeSchema};
    const CLI::App *const serve_cmd = SetupServeCommand(app, serve_config_manager);

    config::ConfigManager<ConnectConfig, decltype(config::kConnectSchema), config::ConnectExtraLoader>
        connect_config_manager{config::kConnectSchema, config::ConnectExtraLoader{}};
    const CLI::App *const connect_cmd = SetupConnectCommand(app, connect_config_manager);

    config::ConfigManager<NodesConfig, decltype(config::kNodesSchema), config::NodesExtraLoader> nodes_config_manager{
        config::kNodesSchema, config::NodesExtraLoader{}
    };
    const CLI::App *const nodes_cmd = SetupNodesCommand(app, nodes_config_manager);

    // ParseError は --help(CallForHelp)と使い方エラー(必須引数不足・未知オプション等)の
    // 共通基底。app.exit() がメッセージ表示と終了コードの決定を行う(help は 0、エラーは非 0)。
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    // cliconf v0.1.0: Resolve() は例外を投げず LoadResult<T>（compat::expected<T, LoadError>）を返す。
    // 設定ファイルが開けない・構文エラー・型不一致などは LoadError として返り、
    // Format() で "<file>: key '<key>': <message>" 形式の文字列が得られる。
    // code（config::LoadErrc）で分類も判定できる（IoError / ParseError / TypeMismatch 等）。
    const auto resolved = config_manager.Resolve(config_files);
    if (!resolved) {
        fmt::print(stderr, "Error: {}\n", resolved.error().Format());
        return 1;
    }

    if (*serve_cmd) {
        const auto serve_conf = serve_config_manager.Resolve(config_files);
        if (!serve_conf) {
            fmt::print(stderr, "Error: {}\n", serve_conf.error().Format());
            return 1;
        }

        // パターン1: 同一型(ServeConfig)を検証する Validate 関数。
        // cliconf v0.1.0 の Validate と同じく compat::expected<void, std::string> を返す。
        if (const auto valid = config::ValidateServeConfig(*serve_conf); !valid) {
            fmt::print(stderr, "Error: {}\n", valid.error());
            return 1;
        }

        fmt::print("serve.host: {}\n", serve_conf->host);
        fmt::print("serve.port: {}\n", serve_conf->port);
        fmt::print("serve.workers: {}\n", serve_conf->workers);
    } else if (*connect_cmd) {
        auto connect_conf = connect_config_manager.Resolve(config_files);
        if (!connect_conf) {
            fmt::print(stderr, "Error: {}\n", connect_conf.error().Format());
            return 1;
        }
        // retry は kConnectSchema のスキーマ外フィールド(RetryConfig)なので
        // Resolve() の戻り値には含まれない。ConnectExtraLoader が読み込んだ
        // ファイル値を GetFileValues() から取得してマージする。
        connect_conf->retry = connect_config_manager.GetFileValues().retry;

        // パターン2: Raw(ConnectConfig) -> Parsed(ParsedConnectConfig) の変換。
        // 検証を通過しない限り ParsedConnectConfig は手に入らないため、
        // 以降のコードは「検証済みの値」であることを型で保証された状態で扱える。
        const auto parsed = config::ParsedConnectConfig::Parse(*connect_conf);
        if (!parsed.has_value()) {
            fmt::print(stderr, "Error: {}\n", parsed.error());
            return 1;
        }

        fmt::print("connect.endpoint: {}\n", parsed->Endpoint());
        fmt::print("connect.timeout_ms: {}\n", parsed->TimeoutMs());
        fmt::print("connect.retry.count: {}\n", parsed->Retry().count);
        fmt::print("connect.retry.interval_ms: {}\n", parsed->Retry().interval_ms);
    } else if (*nodes_cmd) {
        // nodes は NodesExtraLoader::LoadCsv が Resolve() の最後に nodes_csv を
        // 読み込んで書き込むため、戻り値の nodes にそのまま CSV の内容が入っている。
        const auto nodes_conf = nodes_config_manager.Resolve(config_files);
        if (!nodes_conf) {
            fmt::print(stderr, "Error: {}\n", nodes_conf.error().Format());
            return 1;
        }
        fmt::print("nodes.csv: {}\n", nodes_conf->nodes_csv);
        for (const auto &node : nodes_conf->nodes) {
            fmt::print("  host={} weight={}\n", node.host, node.weight);
        }
    }

    fmt::print("log_level: {}\n", resolved->log_level);
    fmt::print("log_output: {}\n", resolved->log_output);
    fmt::print("cluster.name: {}\n", resolved->cluster_name);
    fmt::print("cluster.node_count: {}\n", resolved->cluster_node_count);

    return 0;
}

// 想定内の失敗(CLI の使い方・設定ファイルの内容・バリデーション)は RunApp が
// expected / CLI11 の経路で処理する。ここでは想定外の例外(ExtraLoader などユーザーコードが
// 投げた例外、資源枯渇)を最後に受け止め、core dump ではなくメッセージと終了コードで終える。
int main(int argc, char *argv[]) {
    try {
        return RunApp(argc, argv);
    } catch (const std::exception &e) {
        fmt::print(stderr, "Fatal: {}\n", e.what());
        return 2;
    }
}
