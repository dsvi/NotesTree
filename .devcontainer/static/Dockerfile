# NotesTree - release-parity development container: the *shipping* toolchain,
# but set up for interactive work instead of one-shot packaging.
#
# This is the container to use when you need to work on the thing that actually
# ships: Qt 6.12 built from source, static, musl, Wayland-only, software
# rendered, with D-Bus (the KDE platform theme). Configure with
#   -DNOTES_STATIC_BUILD=ON
# and you get the same binary the top level Dockerfile produces.
#
# Every RUN below the FROM is byte-identical to the top level Dockerfile up to
# and including the qtsvg install, so the two images share Docker's layer
# cache: if you have ever run ./docker-build.sh, building this image reuses the
# whole Qt compile and finishes in seconds. Keep them in sync - if you change
# the release Dockerfile's toolchain, change it here too.
#
# What is *not* here, on purpose:
#   * the AppImage stages (type2-runtime, libfuse, squashfuse, mksquashfs) -
#     packaging, not development;
#   * the `COPY . ./` + build stage - the project directory is bind mounted by
#     the dev container runtime, so the image never goes stale;
#   * Qt's testlib: the release Qt is configured with QT_FEATURE_testlib=OFF,
#     so tests/ cannot be configured here. Run the unit tests in the default
#     container (.devcontainer/Dockerfile), which has Alpine's Qt with QtTest.
#
# Also worth knowing before you pick this one over the default container:
#   * linking is slow - Qt is static, so every relink of the app pulls in the
#     whole Qt core (tens of seconds);
#   * the app is Wayland-only and has no OpenGL/EGL, so it needs a Wayland
#     compositor at run time. Qt Creator cannot mount one into the container
#     (it ignores devcontainer.json "mounts"/"runArgs" and only wires up the
#     X11 socket), so running it from the IDE will not show a window; use the
#     `docker run ... -v $XDG_RUNTIME_DIR/wayland-0:...` recipe in README.md,
#     or debug with gdb from a shell you started yourself.

FROM alpine:3.21

# Qt to build; tag or branch name. Same default as the top level Dockerfile so
# the layer cache is shared.
ARG QT_VERSION=v6.12.0

# ---------------------------------------------------------------------------
# Build tools and libraries - identical to the top level Dockerfile.
RUN apk add --no-cache \
        build-base \
        cmake \
        ninja \
        meson \
        git \
        curl \
        ca-certificates \
        gperf \
        perl \
        python3 \
        py3-packaging \
        pkgconf \
        file \
        util-linux \
        squashfs-tools \
        libffi-dev \
        freetype-dev freetype-static \
        fontconfig-dev fontconfig-static \
        harfbuzz-dev harfbuzz-static \
        graphite2-static \
        libxkbcommon-dev libxkbcommon-static \
        expat-dev expat-static \
        zlib-dev zlib-static \
        bzip2-dev bzip2-static \
        libpng-dev libpng-static \
        clang \
        mimalloc-dev \
        zstd-dev zstd-static \
        autoconf automake libtool \
        patch \
        eudev-dev gettext-dev linux-headers

# ---------------------------------------------------------------------------
# brotli 1.1.0, static, from source - identical to the top level Dockerfile.
RUN curl -fsSL https://github.com/google/brotli/archive/refs/tags/v1.1.0.tar.gz \
        | tar xz -C /tmp \
 && cmake -B /tmp/brotli-build -S /tmp/brotli-1.1.0 -GNinja \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
        -DBUILD_SHARED_LIBS=OFF -DBROTLI_DISABLE_TESTS=ON \
 && cmake --build /tmp/brotli-build \
 && cmake --install /tmp/brotli-build

# ---------------------------------------------------------------------------
# wayland 1.23.1 (client + cursor), static, from source - identical.
RUN curl -fsSL https://gitlab.freedesktop.org/wayland/wayland/-/archive/1.23.1/wayland-1.23.1.tar.gz \
        | tar xz -C /tmp \
 && meson setup /tmp/wayland-build /tmp/wayland-1.23.1 \
        --prefix=/usr --libdir=lib \
        --default-library=static -Ddocumentation=false -Dtests=false \
 && meson compile -C /tmp/wayland-build \
 && meson install -C /tmp/wayland-build

# ---------------------------------------------------------------------------
# OpenSSL 3.5.9 (LTS), static, from source - identical.
RUN curl -fsSL https://www.openssl.org/source/openssl-3.5.9.tar.gz \
        | tar xz -C /tmp \
 && cd /tmp/openssl-3.5.9 \
 && ./Configure --prefix=/usr --openssldir=/etc/ssl --libdir=lib no-shared \
 && chrt --idle 0 make -j"$(nproc)" \
 && make install_sw

# ---------------------------------------------------------------------------
# D-Bus 1.14.10, static, from source - identical.
RUN curl -fsSL https://dbus.freedesktop.org/releases/dbus/dbus-1.14.10.tar.xz \
        | tar xJ -C /tmp \
 && cd /tmp/dbus-1.14.10 \
 && ./configure --prefix=/usr --libdir=/usr/lib \
        --sysconfdir=/etc --localstatedir=/var \
        --enable-static --disable-shared \
        --disable-embedded-tests --disable-stats-syslog \
        --disable-selinux --disable-apparmor \
        --disable-doxygen-docs --disable-xml-docs \
        --without-x \
 && chrt --idle 0 make -j"$(nproc)" \
 && make install

# ---------------------------------------------------------------------------
# Drop the .so linker symlinks - identical to the top level Dockerfile.
RUN rm -f /usr/lib/libfreetype.so \
          /usr/lib/libfontconfig.so \
          /usr/lib/libharfbuzz.so* \
          /usr/lib/libxkbcommon.so \
          /usr/lib/libexpat.so \
          /usr/lib/libz.so \
          /usr/lib/libbz2.so \
          /usr/lib/libpng16.so \
          /usr/lib/libpng.so \
 && rm -rf /usr/lib/cmake/harfbuzz

# ---------------------------------------------------------------------------
# Qt ${QT_VERSION}, static, Wayland only, software rendering, D-Bus on -
# identical to the top level Dockerfile.
RUN git clone --depth 1 --branch ${QT_VERSION} https://github.com/qt/qtbase \
 && git clone --depth 1 --branch ${QT_VERSION} https://github.com/qt/qtsvg
RUN cmake -B qtbase-build -S qtbase -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/qt \
        -DCMAKE_CXX_STANDARD=17 \
        -DCMAKE_FIND_LIBRARY_SUFFIXES=".a;.so" \
        -DBUILD_SHARED_LIBS=OFF \
        -DQT_QPA_PLATFORMS="wayland" \
        -DQT_FEATURE_xcb=OFF \
        -DQT_FEATURE_eglfs=OFF \
        -DQT_FEATURE_sql=OFF \
        -DQT_FEATURE_dbus=ON \
        -DQT_FEATURE_printsupport=OFF \
        -DQT_FEATURE_testlib=OFF \
        -DINPUT_opengl=no \
        -DQT_FEATURE_egl=OFF \
        -DQT_FEATURE_glib=OFF \
        -DQT_FEATURE_system_pcre2=OFF \
        -DINPUT_freetype=system \
        -DINPUT_harfbuzz=system \
        -DINPUT_openssl=linked \
        -DQT_FEATURE_icu=OFF \
        -DQT_BUILD_EXAMPLES=OFF \
        -DQT_BUILD_TESTS=OFF \
 && chrt --idle 0 cmake --build qtbase-build
RUN cmake --install qtbase-build
RUN cmake -B qtsvg-build -S qtsvg -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/qt \
        -DCMAKE_PREFIX_PATH=/opt/qt \
        -DCMAKE_CXX_STANDARD=17 \
        -DCMAKE_FIND_LIBRARY_SUFFIXES=".a;.so" \
        -DBUILD_SHARED_LIBS=OFF \
 && chrt --idle 0 cmake --build qtsvg-build
RUN cmake --install qtsvg-build

# ---------------------------------------------------------------------------
# Everything above is shared with the release image; the rest is dev-only.
#
# Debuggers: gdb for the GCC link, lldb for the clang one (CMakeLists.txt
# picks lld as the linker when the compiler is clang). musl-dbg is what lets
# gdb enumerate threads of a musl program. strace for the usual "why does it
# not pick up my kdeglobals" questions. No Qt from the repos here on purpose:
# /opt/qt is the only Qt in this image, so nothing can shadow it.
RUN apk add --no-cache \
        bash bash-completion which tar xz zip unzip findutils grep sed gawk \
        diffutils procps less nano \
        gdb lldb musl-dbg strace ccache

# Qt's own install rules put both qmake and qmake6 in <prefix>/bin (that is
# what the Alpine qt6-qtbase-dev package ships too), so this is normally a
# no-op; it is here only so the qmake6 path the kit entries use is guaranteed
# to exist whatever the Qt version does.
RUN test -x /opt/qt/bin/qmake6 || ln -sf qmake /opt/qt/bin/qmake6

# The dev user, same uid/gid story as the default container: Qt Creator runs
# every command as the image's USER, and the project directory is bind mounted,
# so a mismatch means root-owned build directories on the host.
ARG USER_UID=1000
ARG USER_GID=1000
ARG USER_NAME=dev
RUN addgroup -g $USER_GID $USER_NAME \
 && adduser -D -u $USER_UID -G $USER_NAME -s /bin/bash $USER_NAME \
 && mkdir -p /home/$USER_NAME/.ccache \
 && chown -R $USER_UID:$USER_GID /home/$USER_NAME

ENV HOME=/home/$USER_NAME
ENV PATH=/opt/qt/bin:$PATH
ENV CMAKE_PREFIX_PATH=/opt/qt

CMD ["sleep", "infinity"]

USER $USER_NAME
WORKDIR $HOME
