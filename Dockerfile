# FULLY static build of NotesTree - musl, zero shared libraries.
#
# Qt 6.12 (qtbase + qtsvg) built from source: Wayland only,
# software rendering, no OpenGL/EGL - but WITH D-Bus: that is what lets
# the app pick up the current KDE/Plasma theme (palette, fonts and icon
# theme from kdeglobals, live updates over the KDE D-Bus interfaces).
#
# The difference to a normal static Qt build: EVERYTHING is linked
# statically - including libc and libdbus - so the resulting binary has
# no PT_INTERP and no NEEDED entries at all. It runs on any x86-64 Linux
# (glibc or musl, any version) with a Wayland compositor, with no runtime
# library requirements whatsoever.
#
# That rules out dlopen() too: a static musl binary cannot load the host's
# glibc-built .so files. Hence OpenSSL is linked in (Qt's -openssl-linked
# mode, INPUT_openssl=linked below) instead of being dlopen'ed by the TLS
# backend at runtime, and the imageformats/TLS/Wayland plugins are linked
# into the binary automatically by Qt's CMake package (the default static
# plugins; see the NOTES_STATIC_BUILD block in CMakeLists.txt).
#
# Alpine is the base because musl is the only libc that is sane to link
# statically (glibc's static libc drags in NSS/dlopen stubs). The catch on
# Alpine: -dev packages ship no .a at all - the archives live in separate
# -static subpackages - and the wayland and brotli archives are built with
# LTO (GCC bytecode, only linkable by a matching GCC). Everything else is
# a plain archive and is taken from the -static packages; wayland and
# brotli are rebuilt from source without LTO, and OpenSSL and D-Bus (no
# -static package exists for either) are built from source static-only.
#
# Usage:  ./docker-build.sh [extra docker build args, e.g. --platform linux/arm64]

FROM alpine:3.21

# Qt to build; tag or branch name (see the comment at the Qt section).
ARG QT_VERSION=v6.12.0

# ---------------------------------------------------------------------------
# Build tools and libraries.
#
# The -static subpackages provide the plain .a archives (freetype,
# fontconfig, harfbuzz, xkbcommon, expat, zlib, bzip2, libpng); the -dev
# packages provide the headers and pkg-config files. libjpeg is NOT
# installed: Qt falls back to its bundled copy, one dependency less.
# No mesa/libdrm anywhere: EGL and OpenGL are compiled out of Qt.
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
        libffi-dev \
        freetype-dev freetype-static \
        fontconfig-dev fontconfig-static \
        harfbuzz-dev harfbuzz-static \
        graphite2-static \
        libxkbcommon-dev libxkbcommon-static \
        expat-dev expat-static \
        zlib-dev zlib-static \
        bzip2-dev bzip2-static \
        libpng-dev libpng-static

# ---------------------------------------------------------------------------
# brotli 1.1.0, static, from source.
#
# Alpine's libbrotli*.a are LTO archives ("plugin needed to handle lto
# object"); rebuilding without LTO keeps them linkable regardless of the
# image's GCC version. They are needed because the system FreeType archive
# references BrotliDecoderDecompress (WOFF2 support), and Qt uses brotli
# as well.
RUN curl -fsSL https://github.com/google/brotli/archive/refs/tags/v1.1.0.tar.gz \
        | tar xz -C /tmp \
 && cmake -B /tmp/brotli-build -S /tmp/brotli-1.1.0 -GNinja \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
        -DBUILD_SHARED_LIBS=OFF -DBROTLI_DISABLE_TESTS=ON \
 && cmake --build /tmp/brotli-build \
 && cmake --install /tmp/brotli-build

# ---------------------------------------------------------------------------
# wayland 1.23.1 (client + cursor), static, from source.
#
# Same LTO problem as brotli: Alpine's libwayland-client.a /
# libwayland-cursor.a are LTO archives. The from-source build installs
# plain archives plus the headers and .pc files over the system's, so
# Qt's pkg-config lookup finds the static ones. All libraries are built
# (in 1.23 "libraries" is a single boolean); the server needs libffi
# (installed) and nothing links the server/egl archives anyway - only
# -client and -cursor end up on the link line.
RUN curl -fsSL https://gitlab.freedesktop.org/wayland/wayland/-/archive/1.23.1/wayland-1.23.1.tar.gz \
        | tar xz -C /tmp \
 && meson setup /tmp/wayland-build /tmp/wayland-1.23.1 \
        --prefix=/usr --libdir=lib \
        --default-library=static -Ddocumentation=false -Dtests=false \
 && meson compile -C /tmp/wayland-build \
 && meson install -C /tmp/wayland-build

# ---------------------------------------------------------------------------
# OpenSSL 3.5.9 (LTS), static, from source - there is no -static package.
#
# no-shared: only .a archives are produced. install_sw installs the
# headers, archives and .pc files over the system's, so Qt's
# INPUT_openssl=linked below links libssl.a/libcrypto.a into the TLS
# plugin instead of dlopen'ing the host's glibc-built libssl at runtime
# (impossible for a static musl binary anyway).
RUN curl -fsSL https://www.openssl.org/source/openssl-3.5.9.tar.gz \
        | tar xz -C /tmp \
 && cd /tmp/openssl-3.5.9 \
 && ./Configure --prefix=/usr --openssldir=/etc/ssl --libdir=lib no-shared \
 && chrt --idle 0 make -j"$(nproc)" \
 && make install_sw

# ---------------------------------------------------------------------------
# D-Bus 1.14.10, static, from source - there is no -static package
# (Alpine's dbus subpackages are dev/libs/doc/x11/openrc only).
#
# Why D-Bus at all: it is what gives the app the KDE platform theme.
# With the dbus feature on, qtbase compiles qkdetheme.cpp into QtGui
# (guarded by "UNIX AND settings AND dbus AND (xcb OR wayland)"): it
# reads ~/.config/kdeglobals for the palette, fonts and icon theme and
# listens to the KDE D-Bus interfaces for live theme changes. With dbus
# off, Qt falls back to the generic unix theme: default palette and
# fonts, no KDE icon theme.
#
# --disable-shared: only libdbus-1.a is produced (plus the dbus tools,
# which nothing links), so Qt's pkg-config lookup of dbus-1 resolves to
# the archive and the final binary stays fully static. expat (already a
# static archive here) is dbus' only external dependency; systemd, X,
# selinux and apparmor support are off.
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

# Drop the .so linker symlinks of every library that must resolve to its
# archive: pkg-config hands Qt plain -lfoo flags, and without the symlink
# the linker can only find libfoo.a. (The real .so.N runtime files stay
# but are never referenced.) Exception: harfbuzz - Qt resolves it through
# pkg-config to the *versioned* file (/usr/lib/libharfbuzz.so.0.60900.0)
# and records that absolute path in its CMake package files, so the
# versioned runtime files must go too; nothing in the image links
# harfbuzz dynamically (only cairo would, and nothing runs cairo).
# Also drop Alpine's harfbuzz CMake *config package*: it defines
# harfbuzz::harfbuzz with IMPORTED_LOCATION pointing at the versioned
# .so file, and Qt's FindWrapSystemHarfbuzz prefers config packages over
# pkg-config - it would record the (now missing) .so path. Without the
# config, the pkg-config/find_library fallback finds libharfbuzz.a.
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
# Qt ${QT_VERSION} (6.12 LTS), built static, Wayland only,
# software rendering only, with D-Bus on (see the D-Bus section above -
# it is what enables the KDE platform theme inside QtGui).
#
# The wayland client (the WaylandClient module and the
# QWaylandIntegrationPlugin platform plugin) is part of qtbase; the
# qtwayland repo only provides the compositor side, which nothing here
# uses, so it is not built. QT_QPA_PLATFORMS=wayland marks the wayland
# plugin as the default platform plugin, which Qt's CMake package then
# links into any app that links Qt6::Gui.
#
#   INPUT_openssl=linked   the CMake spelling of -openssl-linked: the TLS
#                          backend links libssl/libcrypto directly instead
#                          of dlopen'ing them at runtime.
#
# CMAKE_FIND_LIBRARY_SUFFIXES=".a;.so" makes find_library() prefer the
# archives; the .so symlinks removed above make even the pkg-config-driven
# -l flags resolve to archives.
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
# NotesTree itself. NOTES_STATIC_BUILD adds -static to the final link
# (see CMakeLists.txt): no interpreter, no dynamic section at all.
#
# The ldd check asserts full staticness: a binary with no dynamic section
# and no PT_INTERP is not a dynamic program at all, so ldd must fail
# ("Not a valid dynamic program").
#
# The last two checks assert the KDE theme support really got linked in:
# QKdeTheme's kdeglobals path and the org.kde.* D-Bus service names must
# be in the binary.
WORKDIR /src
COPY . ./
RUN cmake -B build -S . -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH=/opt/qt \
        -DNOTES_STATIC_BUILD=ON \
 && cmake --build build --target NotesTree \
 && strip build/NotesTree \
 && file build/NotesTree \
 && ! ldd build/NotesTree \
 && (strings build/NotesTree | grep -E "libEGL|libGLESv2|libgbm|libdrm\.|libwayland-egl|libxkbcommon\.so|libssl\.so|libcrypto\.so|libfontconfig\.so|libdbus-1\.so" || echo "no dynamic-library strings") \
 && (strings build/NotesTree | grep -oE "org\.kde\.[A-Za-z0-9]+" | sort -u || true) \
 && (strings build/NotesTree | grep -q kdeglobals && echo "KDE theme support linked in")
