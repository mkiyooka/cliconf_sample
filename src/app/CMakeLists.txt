add_executable(app main.cpp)

target_link_libraries(app
    PRIVATE core
    PRIVATE CLI11::CLI11
    PRIVATE fmt::fmt
    PRIVATE tomlplusplus::tomlplusplus
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE fkYAML_target
    PRIVATE cliconf::cliconf
)

# プリコンパイル済みヘッダーの設定
if(USE_PCH)
    target_precompile_headers(app PRIVATE
        <memory>
        <string>
    )
    message(STATUS "Precompiled headers enabled for executables")
endif()
