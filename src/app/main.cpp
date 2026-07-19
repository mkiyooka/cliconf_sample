#include <cstdlib>
#include <exception>
#include <vector>

#include <CLI/CLI.hpp>
#include <fmt/base.h>

#include "config/config_manager.hpp"
#include "config/config_schema.hpp"
#include "myproject/core/core.hpp"

namespace {

// 各 Setup*Command はサブコマンドと引数を登録するだけで、演算・出力は行わない。
// subtract の被演算子は設定ファイル(Resolve())で上書きされ得るため、演算はすべて
// parse() 完了・Resolve() 後に main 側でまとめて行う(callback は使わない)。

// add: 純粋なCLI引数。設定ファイルから渡す値ではないため Config に持たせない（CLIオンリー）。
CLI::App *SetupAddCommand(CLI::App &app, int &a, int &b) {
    CLI::App *add = app.add_subcommand("add", "Add two integers (CLI arguments only)");
    add->add_option("a", a, "First operand")->required();
    add->add_option("b", b, "Second operand")->required();
    return add;
}

// multiply: kConfigSchema (config_schema.hpp) が自動マッピングを担うため、
// サブコマンド自体は演算対象の値を持たず、追加の引数登録も不要（自動マッピング）。
CLI::App *SetupMultiplyCommand(CLI::App &app) {
    return app.add_subcommand("multiply", "Multiply two integers (auto-mapped via kConfigSchema)");
}

// subtract: 自動マッピングできない Config::subtract を CLI引数と紐付けるため、
// 位置引数を config.subtract.a/b に直接束縛する（手動マッピング。詳細は
// config_schema.hpp の SubtractExtraLoader を参照）。
CLI::App *SetupSubtractCommand(CLI::App &app, Config &config) {
    CLI::App *subtract = app.add_subcommand("subtract", "Subtract two integers (manually mapped via ExtraLoader)");
    subtract->add_option("a", config.subtract.a, "Minuend (overrides config file)");
    subtract->add_option("b", config.subtract.b, "Subtrahend (overrides config file)");
    return subtract;
}

} // namespace

int main(int argc, char *argv[]) {
    CLI::App app{"cliconf config-system sample"};
    app.require_subcommand(0, 1);

    std::vector<std::string> config_files;
    app.add_option("-c,--config", config_files, "Configuration file(s)");

    using AppConfigManager =
        config::ConfigManager<Config, decltype(config::kConfigSchema), config::SubtractExtraLoader>;
    AppConfigManager config_manager{config::kConfigSchema, config::SubtractExtraLoader{}};
    config_manager.RegisterOptions(app);

    Config config;
    int add_a = 0;
    int add_b = 0;
    const CLI::App *const add_cmd = SetupAddCommand(app, add_a, add_b);
    const CLI::App *const multiply_cmd = SetupMultiplyCommand(app);
    const CLI::App *const subtract_cmd = SetupSubtractCommand(app, config);

    try {
        app.parse(argc, argv);
    } catch (const CLI::CallForHelp &e) {
        std::exit(app.exit(e));
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    // スキーマフィールド(mode/timeout/multiply.a/multiply.b/network.retry.count)は
    // CLI > ファイル > デフォルトで解決済み。設定ファイルが存在しない・パースに
    // 失敗した場合は Resolve() が std::runtime_error を送出する。
    Config resolved;
    try {
        resolved = config_manager.Resolve(config_files);
    } catch (const std::exception &e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return 1;
    }
    config.mode = resolved.mode;
    config.timeout = resolved.timeout;
    config.multiply_a = resolved.multiply_a;
    config.multiply_b = resolved.multiply_b;
    config.network_retry_count = resolved.network_retry_count;

    // subtract: CLI引数が明示指定されていなければ、ExtraLoader が読み込んだファイル値を使う。
    if (subtract_cmd->count("a") == 0) {
        config.subtract.a = config_manager.GetFileValues().subtract.a;
    }
    if (subtract_cmd->count("b") == 0) {
        config.subtract.b = config_manager.GetFileValues().subtract.b;
    }

    if (*add_cmd) {
        fmt::print("{}\n", Add(add_a, add_b));
    } else if (*multiply_cmd) {
        fmt::print("{}\n", config.multiply_a * config.multiply_b);
    } else if (*subtract_cmd) {
        fmt::print("{}\n", config.subtract.a - config.subtract.b);
    }

    fmt::print("mode: {}\n", config.mode);
    fmt::print("timeout: {}\n", config.timeout);
    fmt::print("network.retry.count: {}\n", config.network_retry_count);

    return 0;
}
