# Test executable
add_executable(test_core
    test_core.cpp
)

# テストバイナリはbuild/tests/配下に配置
set_target_properties(test_core PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

target_include_directories(test_core PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_core PRIVATE core doctest::doctest PRIVATE sanitizer_flags)

# Register the test with CTest
add_test(
    NAME test_core
    COMMAND $<TARGET_FILE:test_core>
)
