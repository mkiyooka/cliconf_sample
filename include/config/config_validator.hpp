#pragma once

#include <string>

#include <cliconf/compat/expected.hpp>

#include "config/config_loader.hpp"

namespace config {

// --- パターン1: 同一型を検証する Validate 関数（cliconf 本体の Validate(const Config&)
// と同じ形）---
// ServeConfig をそのまま検証する。正常なら成功、不正ならエラーメッセージを
// compat::expected<void, std::string> で返す（cliconf v0.1.0 の Validate と同じ戻り値型）。
// 呼び出し側は Resolve() 直後にこの関数へ通すだけでよく、型を分けないぶん実装コストが低い。
// ただし「検証済みかどうか」は型からは分からず、呼び出し側が Validate() を呼び忘れても検出できない。
compat::expected<void, std::string> ValidateServeConfig(const ServeConfig &conf);

// --- パターン2: 検証済みであることを型で表現する Raw -> Parsed 変換 ---
// ConnectConfig（Resolve() 直後の未検証の値）を受け取り、検証を通過した場合のみ
// ParsedConnectConfig を返す。ParsedConnectConfig は ConnectConfig と実質同じ
// フィールドを持つが別の型で、コンストラクタを非公開にしているため
// Parse() を経ずに生成することはできない。呼び出し側が検証を忘れるミスを
// 型レベルで防げる一方、型を1つ増やす実装コストがある。
class ParsedConnectConfig {
public:
    static compat::expected<ParsedConnectConfig, std::string> Parse(const ConnectConfig &raw);

    const std::string &Endpoint() const { return endpoint_; }
    int TimeoutMs() const { return timeout_ms_; }
    const RetryConfig &Retry() const { return retry_; }

private:
    ParsedConnectConfig() = default;

    std::string endpoint_;
    int timeout_ms_ = 0;
    RetryConfig retry_;
};

} // namespace config
