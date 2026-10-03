#!/usr/bin/bash

set -e

rm -rf docker-build-out
docker build --tag notestree-build ${1:-} .
mkdir docker-build-out
id=$(docker create notestree-build)
docker cp $id:/src/build/NotesTree docker-build-out/
docker rm $id
docker image rm notestree-build

HI='\033[0;32m'
NC='\033[0m' # No Color

echo
echo  =^..^=   =^..^=   =^..^=    =^..^=    =^..^=    =^..^=    =^..^=    =^..^=    =^..^=
echo
echo -e Output is at ${HI}./docker-build-out${NC} folder.
echo -e "The build is FULLY static (musl): no shared libraries, no libc, no"
echo -e "interpreter - it runs on any x86-64 Linux, glibc or musl."
echo -e "It is static, Wayland only and software rendered - no OpenGL/EGL/mesa."
echo -e "D-Bus is linked in, so it picks up the KDE/Plasma theme."
echo -e "At runtime it needs a wayland compositor, nothing else."
