# メインアプリケーション用のサードパーティライブラリ定義
#
# 使用例:
#   declare_fetchcontent_with_local(fmt ext/fmt-11.2.0
#       GIT_REPOSITORY https://github.com/fmtlib/fmt.git
#       GIT_TAG 11.2.0
#   )
#   FetchContent_MakeAvailable(fmt)

add_external_package(cliconf ext/cliconf
    GIT_REPOSITORY https://github.com/mkiyooka/cliconf.git
    GIT_TAG main
)
FetchContent_MakeAvailable(cliconf)
