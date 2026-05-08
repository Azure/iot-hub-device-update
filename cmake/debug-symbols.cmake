# debug-symbols.cmake
# Strips debug symbols from binaries and saves them as separate .debug files
# for symbol server upload.
#
# Usage:
#   cmake -DADUC_STRIP_SYMBOLS=ON ...
#   ninja
#   ninja strip_symbols  # produces .debug files alongside binaries
#
# Output: For each binary/lib, creates:
#   - Original binary (stripped)
#   - .debug file (full symbols)
#   - .build-id link (for debuginfod/symsrv)

option(ADUC_STRIP_SYMBOLS "Strip debug symbols into separate .debug files" OFF)

if(ADUC_STRIP_SYMBOLS)
    find_program(OBJCOPY_PATH objcopy REQUIRED)
    find_program(STRIP_PATH strip REQUIRED)

    # Function to add stripping post-build for a target
    function(aduc_strip_target target)
        add_custom_command(TARGET ${target} POST_BUILD
            # Extract debug info to .debug file
            COMMAND ${OBJCOPY_PATH} --only-keep-debug
                    $<TARGET_FILE:${target}>
                    $<TARGET_FILE:${target}>.debug
            # Strip the binary
            COMMAND ${STRIP_PATH} --strip-debug --strip-unneeded
                    $<TARGET_FILE:${target}>
            # Add debug link back to stripped binary
            COMMAND ${OBJCOPY_PATH} --add-gnu-debuglink=$<TARGET_FILE:${target}>.debug
                    $<TARGET_FILE:${target}>
            COMMENT "Stripping symbols from ${target} → ${target}.debug"
        )
    endfunction()

    # Custom target to collect all .debug files into a symbols directory
    set(ADUC_SYMBOLS_DIR "${CMAKE_BINARY_DIR}/symbols")

    add_custom_target(collect_symbols
        COMMENT "Collecting debug symbols to ${ADUC_SYMBOLS_DIR}..."
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ADUC_SYMBOLS_DIR}
        COMMAND find ${CMAKE_BINARY_DIR} -name "*.debug" -exec cp {} ${ADUC_SYMBOLS_DIR}/ \;
        COMMAND echo "Symbols collected in: ${ADUC_SYMBOLS_DIR}"
        COMMAND ls -la ${ADUC_SYMBOLS_DIR}/
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    message(STATUS "Symbol stripping enabled")
    message(STATUS "  .debug files created alongside binaries")
    message(STATUS "  Run 'ninja collect_symbols' to gather all .debug files")
endif()
