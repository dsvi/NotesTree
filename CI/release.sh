#!/usr/bin/bash

renice -n 19 $$
cd ..
mkdir -p release

set -e
docker run --privileged --rm tonistiigi/binfmt --install arm64,riscv64
for platform in amd64 riscv64 arm64
do
    ./docker-build.sh "--platform linux/$platform"
    export ZSTD_CLEVEL=19
    tar -caf release/notestree-$platform.tar.zst -C build-out NotesTree
    mv build-out/NotesTree.appimage release/notestree-$platform.AppImage
done

rm -rf build-out

docker system prune -a -f
docker builder prune -a -f
