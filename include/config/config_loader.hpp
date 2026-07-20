#pragma once

#include <string>
#include <vector>

// nodes サブコマンド専用の設定。クラスタのノード一覧を CSV ファイルから読み込む例。
// nodes_csv は Config 直下のフラットなメンバーなので kNodesSchema に登録して
// 自動マッピングできる。一方 nodes(std::vector<NodeRecord>) はスキーマの
// 自動マッピング対象外のため、ExtraLoader::LoadCsv（NodesExtraLoader、
// config_schema.hpp 参照）で nodes_csv が指すファイルを読み込んで書き込む。
// LoadCsv は ConfigManager::Resolve() の最後（スキーマ・CLI 解決が完了した後）に
// 呼ばれるため、nodes_csv は CLI 引数や設定ファイルで上書きされた最終的な
// パスが確定した状態で読み込める。
struct NodeRecord {
    std::string host;
    double weight = 0.0;
};

struct NodesConfig {
    std::string nodes_csv;
    std::vector<NodeRecord> nodes;
};

// serve サブコマンド専用の設定。ServeConfig 自体を Owner とする専用スキーマ
// (kServeSchema)・専用の ConfigManager<ServeConfig, ...> を用意し、その
// RegisterOptions() を serve サブコマンドの CLI::App にだけ呼ぶことで、
// --serve.host 等のオプションがトップレベルの --help に出ないようにする
// (config_schema.hpp の kServeSchema、main.cpp を参照)。
struct ServeConfig {
    std::string host = "0.0.0.0";
    int port = 8080;
    int workers = 4;
};

// connect サブコマンド専用の設定。endpoint はフラットなメンバーなので自動マッピング
// できるが、retry(RetryConfig)は入れ子構造体のため ExtraLoader で手動読み込みする
// (手動マッピング。config_schema.hpp の ConnectExtraLoader を参照)。
struct RetryConfig {
    int count = 3;
    int interval_ms = 500;
};

struct ConnectConfig {
    std::string endpoint = "localhost:9000";
    int timeout_ms = 3000;
    RetryConfig retry;
};

struct Config {
    // [app] セクション: アプリ全体設定。Config 直下のフラットなメンバーなので
    // kConfigSchema に config_key "app.log_level" 等を1行登録するだけで自動マッピングされる。
    std::string log_level = "info";
    std::string log_output = "stdout";

    // [cluster] セクション: モジュール単位の設定。アプリ全体設定と同様に
    // Config 直下のフラットなメンバーとして扱える(自動マッピング)。
    std::string cluster_name = "default";
    int cluster_node_count = 1;
};
