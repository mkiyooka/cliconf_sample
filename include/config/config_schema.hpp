#pragma once

#include <tuple>

#include <cliconf/field_descriptor.hpp>
#include <fkYAML/node.hpp>
#include <nlohmann/json.hpp>
#include <toml++/toml.hpp>

#include "config/config_loader.hpp"

namespace config {

// 各エントリを1行登録するだけで CLI11 オプション登録・TOML/JSONC/YAML 読み込み・
// 優先度解決(CLI > 設定ファイル > デフォルト)が ConfigManager によって自動化される。
// config_key はドット区切りで何段でもネストできる(multiply.a、network.retry.count 等)。
// 詳細は Config (config_loader.hpp) 側のコメントを参照。
inline constexpr auto kConfigSchema = std::make_tuple(
    FieldDescriptor{"--mode", "mode", "Operation mode", &Config::mode},
    FieldDescriptor{"--timeout", "timeout", "Timeout in seconds", &Config::timeout},
    FieldDescriptor{"--multiply.a", "multiply.a", "Multiplicand (overrides config file)", &Config::multiply_a},
    FieldDescriptor{"--multiply.b", "multiply.b", "Multiplier (overrides config file)", &Config::multiply_b},
    FieldDescriptor{
        "--network.retry.count", "network.retry.count", "Retry count (2-level nested key)", &Config::network_retry_count
    }
);

// subtract サブコマンドの a/b はスキーマで自動マッピングできない入れ子構造体のため、
// ExtraLoader で設定ファイルの [subtract] セクションから読み込む。
//
// なぜ FieldDescriptor で SubtractConfig 全体（&Config::subtract）を指定できないか:
// 1) ConfigManager::RegisterOptions / Resolve は常に "Config型インスタンス.*field.member"
//    という形で参照するため、field.member は T Config::* でなければコンパイルできない
//    （FieldDescriptor<Owner, T> の Owner を SubtractConfig にしても Config に対して
//    使えない）。
// 2) 仮に(1)を回避できても、ResolveDottedKey は最終的に toml::table::value<T>() 等を
//    呼ぶが、toml++ の value<T>() は string/int64_t/double/bool 等のネイティブ型しか
//    受け付けず、T が SubtractConfig のような集約型だと static_assert でコンパイル
//    エラーになる。
// つまり自動マッピングの単位は常にスカラー値1個であり、構造体をまるごとマッピングする
// ことはできない。
struct SubtractExtraLoader {
    void LoadToml(const toml::table &tbl, Config &conf) const {
        if (const auto *sub = tbl["subtract"].as_table()) {
            conf.subtract.a = (*sub)["a"].value_or(0);
            conf.subtract.b = (*sub)["b"].value_or(0);
        }
    }

    void LoadJson(const nlohmann::json &j, Config &conf) const {
        if (j.contains("subtract") && j.at("subtract").is_object()) {
            const auto &sub = j.at("subtract");
            conf.subtract.a = sub.value("a", 0);
            conf.subtract.b = sub.value("b", 0);
        }
    }

    void LoadYaml(const fkyaml::node &root, Config &conf) const {
        if (root.is_mapping() && root.contains("subtract")) {
            const auto &sub = root.at("subtract");
            if (sub.is_mapping()) {
                if (sub.contains("a")) {
                    conf.subtract.a = sub.at("a").get_value<int>();
                }
                if (sub.contains("b")) {
                    conf.subtract.b = sub.at("b").get_value<int>();
                }
            }
        }
    }
};

// divide サブコマンドの被演算子(DivideConfig)を自動マッピングするための専用スキーマ。
// FieldDescriptor<Owner, T> の Owner を Config ではなく DivideConfig にできるため、
// DivideConfig を主語にした ConfigManager<DivideConfig, decltype(kDivideSchema)> を
// main.cpp で別途 Resolve() すれば、config_key のドット区切りパス("divide.a")経由で
// [divide] セクションを ExtraLoader なしで自動マッピングできる。
// (subtract との違い: subtract は Config::subtract という「Config のメンバー」を
//  スキーマ化しようとして失敗する一方、divide は DivideConfig それ自体を主語にした
//  別のスキーマ・ConfigManager を用意することで自動マッピングを実現している。)
inline constexpr auto kDivideSchema = std::make_tuple(
    FieldDescriptor{"--divide.a", "divide.a", "Dividend (overrides config file)", &DivideConfig::a},
    FieldDescriptor{"--divide.b", "divide.b", "Divisor (overrides config file)", &DivideConfig::b}
);

} // namespace config
