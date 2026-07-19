#pragma once

#include <tuple>

#include <cliconf/field_descriptor.hpp>

#include "config/config_loader.hpp"

namespace config {

inline constexpr auto kConfigSchema = std::make_tuple(
    FieldDescriptor{"--mode", "mode", "Operation mode", &Config::mode},
    FieldDescriptor{"--timeout", "timeout", "Timeout in seconds", &Config::timeout}
);

} // namespace config
