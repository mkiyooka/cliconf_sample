#pragma once

#include <string>

// subtract サブコマンドの被演算子。スキーマの自動マッピングは Config 直下の
// フラットなメンバーポインタしか扱えず、この入れ子構造体のメンバーには到達できない。
// そのため設定ファイルの [subtract] セクションから ExtraLoader で手動読み込みする
// (config_schema.hpp の SubtractExtraLoader を参照)。
struct SubtractConfig {
    int a = 0;
    int b = 0;
};

struct Config {
    std::string mode = "default";
    int timeout = 30;

    // multiply サブコマンドの被演算子。Config 直下のフラットなメンバーなので、
    // 設定ファイル側の [multiply] セクション(a/b)に対応する "multiply.a" /
    // "multiply.b" という config_key を FieldDescriptor に登録するだけで
    // 自動マッピングされる(kConfigSchema, 手動コード不要)。
    int multiply_a = 0;
    int multiply_b = 0;

    // ドット区切りキーは段数に関わらず解決されるため(config_file_loader.hpp の
    // ResolveDottedKey)、"network.retry.count" のように設定ファイル側が
    // [network.retry] セクションの count という2階層ネストでも、Config 側は
    // このフラットな1メンバーのままで自動マッピングできる。
    int network_retry_count = 3;

    SubtractConfig subtract;
};
