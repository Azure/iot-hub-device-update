# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

#
# Embeds the test root keys into the target binary if applicable.
#
# Inputs: target - The cmake target that may be affected.
#
# Dependencies: CMAKE_BUILD_TYPE - The build type factors into applicability decision.
# Side-effects: When applicable, the target's compile definitions will be affected.
#
function (embed_test_root_keys_if_applicable target)
    message (STATUS "  CMAKE_BUILD_TYPE is: " ${CMAKE_BUILD_TYPE})
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        message (STATUS "Adding Test Root Keys for Debug Build.")

        set (test_root_keys_flag -DBUILD_WITH_TEST_KEYS=1)
        target_compile_definitions (${target} PRIVATE ${test_root_keys_flag})
    endif ()
endfunction ()
