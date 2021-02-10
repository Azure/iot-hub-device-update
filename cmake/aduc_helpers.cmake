cmake_minimum_required (VERSION 3.5)

# Sets a cache variable with the following precedence
# 1. Value already present in cache. This macro will not override an existing cache value.
# 2. An environment variable with the same name. Only reads from the environment and does not set environment variables.
# 3. The specified default value.
macro (
    set_cache_with_env_or_default
    variable
    default
    type
    description)
    if (DEFINED ENV{${variable}})
        set (
            ${variable}
            $ENV{${variable}}
            CACHE ${type} ${description})
    else ()
        set (
            ${variable}
            ${default}
            CACHE ${type} ${description})
    endif ()
endmacro ()
