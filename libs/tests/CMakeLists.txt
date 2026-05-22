add_executable(tests
        main.cpp
)

target_link_libraries(tests PRIVATE
        core
        fmt::fmt
        llama
)

install(TARGETS tests DESTINATION tools)
install(TARGETS tests
        COMPONENT tests
        RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "cublas64_" "cudart64_"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
        DIRECTORIES $<TARGET_FILE_DIR:tests>
        DESTINATION tools
)
