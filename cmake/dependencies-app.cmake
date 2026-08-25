# メインアプリケーション用のサードパーティライブラリ定義
#
# 使用例:
#   declare_fetchcontent_with_local(fmt ext/fmt-11.2.0
#       GIT_REPOSITORY https://github.com/fmtlib/fmt.git
#       GIT_TAG 11.2.0
#   )
#   FetchContent_MakeAvailable(fmt)

# fmt: このプロジェクト自身が main.cpp / config_validator.cpp で使用する。
# cliconf側は fmt を「cliconf自身のサンプルアプリ(cmd)専用の依存」として
# PROJECT_IS_TOP_LEVEL 内でのみ取得する設計のため、サブプロジェクトとして
# 取り込むこちらのプロジェクトでは自前で用意する必要がある
# （cliconf の cmake/local-or-fetch.cmake の TARGET_CHECK 排他制御により、
# ここで先に fmt::fmt を用意しておけば cliconf 側は取得をスキップする）。
add_external_package(fmt third_party/fmt-12.2.0
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 12.2.0
)
FetchContent_MakeAvailable(fmt)

add_external_package(cliconf ext/cliconf
    GIT_REPOSITORY https://github.com/mkiyooka/cliconf.git
    GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(cliconf)
