add_library(core
    STATIC
    core.cpp
)
target_link_libraries(core PRIVATE sanitizer_flags)
target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/include)
