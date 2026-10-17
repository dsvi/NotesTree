# NotesTree - day to day development container: Alpine (musl) + Qt 6 from the
# Alpine repositories.
#
# This is the "iterate fast" image: Qt is a *shared* build (qt6-qtbase-dev,
# qt6-qtsvg-dev), so linking the app takes a second instead of the minute a
# static Qt link takes. It carries both toolchains the project supports (GCC
# and clang/lld - CMakeLists.txt has a clang branch), both debuggers (gdb for
# GCC, lldb for clang), musl-dbg for thread debugging, and QtTest for the unit
# tests in tests/.
#
# The shipping configuration - fully static musl, Qt 6.12 built from source -
# is the sibling container in .devcontainer/static/; that one mirrors the top
# level Dockerfile.
#
# Why this image works with Qt Creator's Development Container support:
#   * Qt Creator starts the container with `--entrypoint /bin/sh`, so /bin/sh
#     has to exist - busybox ash is fine.
#   * It mounts its own libexec dir into the container and runs the Command
#     Bridge helper from there. That helper (cmdbridge-linux-<arch>) is a
#     *statically linked Go* binary, so it runs on musl exactly as it does on
#     glibc - no gcompat needed. Hence mount-libexec can stay enabled.
#   * It adds --cap-add=SYS_PTRACE and --security-opt seccomp=unconfined, so
#     gdb/lldb can attach and the app can fork.
#   * It mounts the host's X11 socket and Xauthority, which is how the app
#     started from the IDE shows up on the host desktop: the Qt in this image
#     has the xcb platform plugin (see the libxcb/xcb-util* packages below),
#     so it displays through the host's XWayland.
#
# Nothing here is copied from the build context: the project directory is
# bind mounted at run time, so the image is build-cache friendly and the IDE
# sees live sources.

# Alpine 3.23, not the 3.21 the release image uses: 3.21 ships Qt 6.8.2, which
# is too old for this code - QString::arg(char, ...) as used in src/Note.c++
# (decodeFromFilename) only exists from Qt 6.10 on, and the app does not
# compile against 6.8. 3.23 has Qt 6.10.3, close to the 6.12 the release build
# compiles against (edge has 6.11.1); both build the project as it is.
FROM alpine:3.23

# For reference only - apk decides the actual version.
ARG QT_VERSION=6.10.3

# The user Qt Creator runs every command as (docker exec uses the image's
# USER). Keep uid/gid equal to the host user's - 1000:1000 on most desktops -
# so files the build writes into the bind mounted project directory are owned
# by you on the host and not by root. Override with
#   docker build --build-arg USER_UID=$(id -u) --build-arg USER_GID=$(id -g)
# (Qt Creator does not pass build args from devcontainer.json, so change the
# defaults here if your uid is not 1000.)
ARG USER_UID=1000
ARG USER_GID=1000
ARG USER_NAME=dev

RUN apk add --no-cache \
        # shell + coreutils: the interactive shell, and the basics Qt Creator's
        # setup steps and a developer's fingers expect
        bash bash-completion ca-certificates curl wget git which tar xz zip unzip \
        findutils grep sed gawk diffutils file procps less nano \
        # build system + the two toolchains + debuggers
        build-base cmake ninja ccache pkgconf python3 \
        clang lld lldb gdb musl-dbg \
        # Qt 6: Widgets/Xml/Svg/Network are what the app links, Test is what
        # tests/ links. moc/uic/rcc come with qt6-qtbase-dev (they live in
        # /usr/lib/qt6/libexec, where AUTOMOC/AUTOUIC/AUTORCC find them).
        # qt6-qttools-dev is deliberately NOT installed: it is linguist and
        # designer, and it pulls in qtdeclarative + qtshadertools - all the QML
        # tooling this project has no use for, and about half a GB of image.
        qt6-qtbase-dev qt6-qtsvg-dev \
        # xcb platform plugin runtime deps: Qt Creator mounts /tmp/.X11-unix
        # and XAUTHORITY into the container, so the app can display over X11
        # with no configuration at all. The wayland plugin ships with
        # qt6-qtbase as well; the wayland and D-Bus session sockets reach the
        # container through the "mounts" entry in devcontainer.json.
        libxcb xcb-util-cursor xcb-util-wm xcb-util-image xcb-util-keysyms

# ccache lives in /usr/lib/ccache for the compiler-wrapper style setup; the
# recommended way to use it from the IDE is
#   -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
# in the CMake configuration of the kit (see README.md).
RUN mkdir -p /home/$USER_NAME/.ccache \
 && addgroup -g $USER_GID $USER_NAME \
 && adduser -D -u $USER_UID -G $USER_NAME -s /bin/bash $USER_NAME \
 && chown -R $USER_UID:$USER_GID /home/$USER_NAME

ENV HOME=/home/$USER_NAME

# Keep the container up when it is started without an explicit command
# (`docker run -d` of a plain alpine image would otherwise exit at once). Qt
# Creator overrides the entrypoint anyway; VS Code / the devcontainers CLI
# override the command as well (overrideCommand).
CMD ["sleep", "infinity"]

USER $USER_NAME
WORKDIR $HOME
