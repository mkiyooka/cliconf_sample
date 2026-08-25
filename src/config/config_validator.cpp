#include "config/config_validator.hpp"

#include <fmt/format.h>

namespace config {

compat::expected<void, std::string> ValidateServeConfig(const ServeConfig &conf) {
    if (conf.port < 1 || conf.port > 65535) {
        return compat::unexpected(fmt::format("serve.port must be in [1, 65535], got {}", conf.port));
    }
    if (conf.workers < 1) {
        return compat::unexpected(fmt::format("serve.workers must be positive, got {}", conf.workers));
    }
    if (conf.host.empty()) {
        return compat::unexpected(std::string{"serve.host must not be empty"});
    }
    return {};
}

compat::expected<ParsedConnectConfig, std::string> ParsedConnectConfig::Parse(const ConnectConfig &raw) {
    if (raw.endpoint.empty()) {
        return compat::unexpected(std::string{"connect.endpoint must not be empty"});
    }
    if (raw.timeout_ms < 1) {
        return compat::unexpected(fmt::format("connect.timeout_ms must be positive, got {}", raw.timeout_ms));
    }
    if (raw.retry.count < 0) {
        return compat::unexpected(fmt::format("connect.retry.count must not be negative, got {}", raw.retry.count));
    }
    if (raw.retry.interval_ms < 1) {
        return compat::unexpected(
            fmt::format("connect.retry.interval_ms must be positive, got {}", raw.retry.interval_ms)
        );
    }

    ParsedConnectConfig parsed;
    parsed.endpoint_ = raw.endpoint;
    parsed.timeout_ms_ = raw.timeout_ms;
    parsed.retry_ = raw.retry;
    return parsed;
}

} // namespace config
