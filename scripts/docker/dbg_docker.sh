#!/bin/sh

# e.g. $ ./dbg_docker.sh duagentaptcurlubuntu18.04stable:1.0 bash

# SYS_PTRACE capability is needed to allow gdb to set breakpoints
docker run --rm -it --cap-add=SYS_PTRACE --security-opt seccomp=unconfined "$@"
ret_val=$?
if [ $ret_val -ne 0 ]; then
    exit 1
fi
