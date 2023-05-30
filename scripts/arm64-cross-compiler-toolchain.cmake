set (CMAKE_SYSTEM_NAME Linux)
set (CMAKE_SYSTEM_PROCESSOR aarch64)

set (TOOLCHAIN_TRIPLET aarch64-linux-gnu)

#execute_process(
#    COMMAND which ${TOOLCHAIN_TRIPLET}-gcc
#    OUTPUT_VARIABLE TOOLCHAIN_DIR
#  OUTPUT_STRIP_TRAILING_WHITESPACE)
#set(TOOLCHAIN_DIR "${TOOLCHAIN_DIR}/../..")
set (TOOLCHAIN_DIR /usr/bin)

# The path on the host to install to as opposed to
# CMAKE_INSTALL_PREFIX, which is the runtime install
# location even when cross-compiling.
set (CMAKE_STAGING_PREFIX $ENV{cross_compilation_staging_dir})

set (CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/${TOOLCHAIN_TRIPLET}-gcc)
set (CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/${TOOLCHAIN_TRIPLET}-g++)

# This is where cross-compilation will find aarch64 system lib, include, etc.
# instead of the normal x64 ones.
# For more info, see gcc docs on --sysroot=dir docs at:
# https://gcc.gnu.org/onlinedocs/gcc/Directory-Options.html
set (CMAKE_SYSROOT ${TOOLCHAIN_DIR}/../${TOOLCHAIN_TRIPLET})

set (CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_DIR})
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# This is to avoid the simple linking test that will fail when
# cross-compiling. CMAKE_TRY_COMPILE_TARGET_TYPE with static lib
# is the proper way instead of the hackier of setting
# CMAKE_<LANG>_COMPILER_WORKS 1
set (CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
#SET (CMAKE_C_COMPILER_WORKS 1)
#SET (CMAKE_CXX_COMPILER_WORKS 1)
