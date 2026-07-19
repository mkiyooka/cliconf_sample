add_executable(app main.cpp)

target_link_libraries(app
    PRIVATE core
    PRIVATE cliconf::config
    PRIVATE command_lib
)

# プリコンパイル済みヘッダーの設定
if(USE_PCH)
    target_precompile_headers(app PRIVATE
        <memory>
        <string>
    )
    message(STATUS "Precompiled headers enabled for executables")
endif()
