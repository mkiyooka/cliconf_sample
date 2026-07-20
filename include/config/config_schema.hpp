#pragma once

#include <tuple>

#include <cliconf/field_descriptor.hpp>
#include <cliconf/utility/csv_wrapper.hpp>
#include <fkYAML/node.hpp>
#include <nlohmann/json.hpp>
#include <toml++/toml.hpp>

#include "config/config_loader.hpp"

namespace config {

// アプリ全体・モジュール単位の設定。各エントリを1行登録するだけで CLI11 オプション登録・
// TOML/JSONC/YAML 読み込み・優先度解決(CLI > 設定ファイル > デフォルト)が
// ConfigManager によって自動化される(自動マッピング)。
// config_key はドット区切りで何段でもネストできるため、[app] セクションの log_level と
// [cluster] セクションの name/node_count のように論理的な区分が違っても、Config は
// どちらもフラットなメンバーのままでよい。
inline constexpr auto kConfigSchema = std::make_tuple(
    FieldDescriptor{"--log-level", "app.log_level", "Log level (debug/info/warn/error)", &Config::log_level},
    FieldDescriptor{"--log-output", "app.log_output", "Log output destination (stdout/file)", &Config::log_output},
    FieldDescriptor{"--cluster.name", "cluster.name", "Cluster name", &Config::cluster_name},
    FieldDescriptor{
        "--cluster.node-count", "cluster.node_count", "Number of nodes in the cluster", &Config::cluster_node_count
    }
);

// serve サブコマンド専用スキーマ。ServeConfig を Owner とする専用の
// ConfigManager<ServeConfig, decltype(kServeSchema)> を main.cpp で用意し、
// その RegisterOptions() を serve サブコマンドの CLI::App にだけ呼ぶことで、
// --serve.host 等のオプションをトップレベルの --help から隠せる(自動マッピング)。
inline constexpr auto kServeSchema = std::make_tuple(
    FieldDescriptor{"--host", "serve.host", "Bind address", &ServeConfig::host},
    FieldDescriptor{"--port", "serve.port", "Listen port", &ServeConfig::port},
    FieldDescriptor{"--workers", "serve.workers", "Number of worker threads", &ServeConfig::workers}
);

// connect サブコマンド専用スキーマ。endpoint/timeout_ms は自動マッピングするが、
// retry(RetryConfig)はスキーマ外フィールドのため ConnectExtraLoader で
// 手動読み込みする(手動マッピング。詳細は下記コメント参照)。
inline constexpr auto kConnectSchema = std::make_tuple(
    FieldDescriptor{"--endpoint", "connect.endpoint", "Remote endpoint", &ConnectConfig::endpoint},
    FieldDescriptor{"--timeout-ms", "connect.timeout_ms", "Connection timeout (ms)", &ConnectConfig::timeout_ms}
);

// なぜ retry を FieldDescriptor で自動マッピングできないか:
// FieldDescriptor<Owner, T> の T は最終的に toml::table::value<T>() 等で解決される
// (config_file_loader.hpp の ResolveDottedKey)。toml++ の value<T>() は
// string/int64_t/double/bool 等のネイティブ型しか受け付けず、T が RetryConfig の
// ような集約型だと static_assert でコンパイルエラーになる。そのためネストした
// 構造体はスキーマの対象外とし、ExtraLoader で生のパース結果から読み出す。
struct ConnectExtraLoader {
    void LoadToml(const toml::table &tbl, ConnectConfig &conf) const {
        if (const auto *retry = tbl["connect"]["retry"].as_table()) {
            conf.retry.count = (*retry)["count"].value_or(conf.retry.count);
            conf.retry.interval_ms = (*retry)["interval_ms"].value_or(conf.retry.interval_ms);
        }
    }

    void LoadJson(const nlohmann::json &j, ConnectConfig &conf) const {
        if (j.contains("connect") && j.at("connect").contains("retry")) {
            const auto &retry = j.at("connect").at("retry");
            conf.retry.count = retry.value("count", conf.retry.count);
            conf.retry.interval_ms = retry.value("interval_ms", conf.retry.interval_ms);
        }
    }

    void LoadYaml(const fkyaml::node &root, ConnectConfig &conf) const {
        if (root.is_mapping() && root.contains("connect")) {
            const auto &connect = root.at("connect");
            if (connect.is_mapping() && connect.contains("retry")) {
                const auto &retry = connect.at("retry");
                if (retry.is_mapping()) {
                    if (retry.contains("count")) {
                        conf.retry.count = retry.at("count").get_value<int>();
                    }
                    if (retry.contains("interval_ms")) {
                        conf.retry.interval_ms = retry.at("interval_ms").get_value<int>();
                    }
                }
            }
        }
    }

    // connect は CSV を扱わないため no-op。ExtraLoader は4メソッドすべての実装が
    // 必須(NoExtraLoader のような既定実装は提供されない)。
    void LoadCsv(ConnectConfig & /*conf*/) const {}
};

// nodes サブコマンド専用スキーマ。nodes_csv は Config 直下のフラットなメンバーなので
// kNodesSchema に登録するだけで自動マッピングされる(CLI --nodes-csv でも上書き可能)。
inline constexpr auto kNodesSchema = std::make_tuple(
    FieldDescriptor{"--nodes-csv", "nodes.csv", "Path to a CSV file listing cluster nodes", &NodesConfig::nodes_csv}
);

// nodes(std::vector<NodeRecord>)はスキーマの自動マッピング対象外のため、
// ExtraLoader::LoadCsv で nodes_csv が指す CSV ファイルを読み込む。
// LoadCsv は Resolve() の最後、スキーマ・CLI 解決が完了した後に呼ばれるため、
// nodes_csv は CLI / 設定ファイルで上書きされた最終的な値が確定した状態で読み込める。
// TOML/JSONC/YAML はこのサブコマンドでは扱わないため LoadToml/LoadJson/LoadYaml は no-op。
struct NodesExtraLoader {
    void LoadToml(const toml::table & /*tbl*/, NodesConfig & /*conf*/) const {}
    void LoadJson(const nlohmann::json & /*j*/, NodesConfig & /*conf*/) const {}
    void LoadYaml(const fkyaml::node & /*root*/, NodesConfig & /*conf*/) const {}

    void LoadCsv(NodesConfig &conf) const {
        if (conf.nodes_csv.empty()) {
            return;
        }
        utility::CsvReader reader(conf.nodes_csv);
        auto hosts = reader.ReadFilteredAsStrings(
            [](const csv::CSVRow &row) { return row["enabled"].get<int>() == 1; }, {"host"}
        );
        auto weights =
            reader.ReadFiltered([](const csv::CSVRow &row) { return row["enabled"].get<int>() == 1; }, {"weight"});
        if (!hosts.has_value() || !weights.has_value() || hosts->size() != weights->size()) {
            return;
        }
        conf.nodes.clear();
        for (std::size_t i = 0; i < hosts->size(); ++i) {
            conf.nodes.push_back(NodeRecord{(*hosts)[i], (*weights)[i]});
        }
    }
};

} // namespace config
