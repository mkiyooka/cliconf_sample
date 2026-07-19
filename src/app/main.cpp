#include <cstdlib>
#include <vector>

#include <CLI/CLI.hpp>
#include <fmt/base.h>

#include "config/config_manager.hpp"
#include "config/config_schema.hpp"
#include "myproject/core/core.hpp"

namespace {

void SetupAddCommand(CLI::App &app, Config &config) {
    CLI::App *add = app.add_subcommand("add", "Add two integers");
    add->add_option("a", config.add.a, "First operand")->required();
    add->add_option("b", config.add.b, "Second operand")->required();
    add->callback([&config] { fmt::print("{}\n", Add(config.add.a, config.add.b)); });
}

} // namespace

int main(int argc, char *argv[]) {
    CLI::App app{"cliconf config-system sample"};
    app.require_subcommand(0, 1);

    std::vector<std::string> config_files;
    app.add_option("-c,--config", config_files, "Configuration file(s)");

    config::ConfigManager<Config, decltype(config::kConfigSchema)> config_manager{config::kConfigSchema};
    config_manager.RegisterOptions(app);

    Config config;
    SetupAddCommand(app, config);

    try {
        app.parse(argc, argv);
    } catch (const CLI::CallForHelp &e) {
        std::exit(app.exit(e));
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    const Config resolved = config_manager.Resolve(config_files);
    config.mode = resolved.mode;
    config.timeout = resolved.timeout;

    fmt::print("mode: {}\n", config.mode);
    fmt::print("timeout: {}\n", config.timeout);

    return 0;
}
