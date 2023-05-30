# Cross compile for ARM64 on x64 host system

## Install cross-compiler toolchain

```sh
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu binutils-aarch64-linux-gnu
$ ls -latr /usr/bin/aarch64-linux-gnu-gcc | awk '{ print $9 " " $10 " " $11 }'
lrwxrwxrwx 1 root root 23 Mar 20  2020 /usr/bin/aarch64-linux-gnu-gcc -> aarch64-linux-gnu-gcc-9
```

## Build using build script

```sh
./scripts/build.sh -c --target-arch ARM64 --type MinSizeRel
```

On build completion, output will exist in `out/ARM64-MinSizeRel` directory.

## CMake toolchain file

The CMake cross-compiler toolchain file is `scripts/arm64-cross-compiler-toolchain.cmake`


