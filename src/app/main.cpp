#include <cstdlib>
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

} // namespace

int main(int argc, char *argv[]) {
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

    try {
        app.parse(argc, argv);
    } catch (const CLI::CallForHelp &e) {
        std::exit(app.exit(e));
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    // 設定ファイルが存在しない・パースに失敗した場合は Resolve() が
    // std::runtime_error を送出する。
    Config resolved;
    try {
        resolved = config_manager.Resolve(config_files);

        if (*serve_cmd) {
            const ServeConfig serve_conf = serve_config_manager.Resolve(config_files);

            // パターン1: 同一型(ServeConfig)を検証する Validate 関数。
            // エラーがあれば空でない文字列が返る。
            const std::string error = config::ValidateServeConfig(serve_conf);
            if (!error.empty()) {
                fmt::print(stderr, "Error: {}\n", error);
                return 1;
            }

            fmt::print("serve.host: {}\n", serve_conf.host);
            fmt::print("serve.port: {}\n", serve_conf.port);
            fmt::print("serve.workers: {}\n", serve_conf.workers);
        } else if (*connect_cmd) {
            ConnectConfig connect_conf = connect_config_manager.Resolve(config_files);
            // retry は kConnectSchema のスキーマ外フィールド(RetryConfig)なので
            // Resolve() の戻り値には含まれない。ConnectExtraLoader が読み込んだ
            // ファイル値を GetFileValues() から取得してマージする。
            connect_conf.retry = connect_config_manager.GetFileValues().retry;

            // パターン2: Raw(ConnectConfig) -> Parsed(ParsedConnectConfig) の変換。
            // 検証を通過しない限り ParsedConnectConfig は手に入らないため、
            // 以降のコードは「検証済みの値」であることを型で保証された状態で扱える。
            const auto parsed = config::ParsedConnectConfig::Parse(connect_conf);
            if (!parsed.has_value()) {
                fmt::print(stderr, "Error: {}\n", parsed.error());
                return 1;
            }

            fmt::print("connect.endpoint: {}\n", parsed->Endpoint());
            fmt::print("connect.timeout_ms: {}\n", parsed->TimeoutMs());
            fmt::print("connect.retry.count: {}\n", parsed->Retry().count);
            fmt::print("connect.retry.interval_ms: {}\n", parsed->Retry().interval_ms);
        }
    } catch (const std::exception &e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return 1;
    }

    fmt::print("log_level: {}\n", resolved.log_level);
    fmt::print("log_output: {}\n", resolved.log_output);
    fmt::print("cluster.name: {}\n", resolved.cluster_name);
    fmt::print("cluster.node_count: {}\n", resolved.cluster_node_count);

    return 0;
}
