add_executable(tests
        main.cpp
)

target_link_libraries(tests PRIVATE
        core
        fmt::fmt
        llama
        Tracy::TracyClient
)

install(TARGETS tests DESTINATION tools)
install(TARGETS tests
        COMPONENT tests
        RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
        DIRECTORIES $<TARGET_FILE_DIR:tests> "$ENV{CUDA_PATH}/bin"
        DESTINATION tools
)
