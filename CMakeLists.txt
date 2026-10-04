cmake_minimum_required(VERSION 3.21)

# Set the default CMAKE_BUILD_TYPE to Release.
# This should be done before the project command since the latter can set
# CMAKE_BUILD_TYPE itself (it does so for nmake).
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

project(NotesTree VERSION 1.2.3 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    string(APPEND CMAKE_CXX_FLAGS " -Wno-unqualified-std-cast-call")
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -fuse-ld=lld")
    #string(APPEND CMAKE_CXX_FLAGS_DEBUG " -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer")
endif()

option(NOTES_STATIC_BUILD "Build a fully static version of the program: no dependency on any shared library. Requires the static libc parts (libc.a, crt objects) to be installed." OFF)

if (NOTES_STATIC_BUILD)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # clang + libc++ (e.g. an Alpine/musl build): static C++ runtime and
        # libc. -Wl,-Bdynamic at the end lets the few unavoidable shared
        # libraries resolve to .so. The docker build uses gcc instead (the
        # else branch).
        string(APPEND CMAKE_CXX_FLAGS " -stdlib=libc++")
        string(APPEND CMAKE_EXE_LINKER_FLAGS " -Wl,-Bstatic -lc++ -lc++abi -lc -Wl,-Bdynamic")
    else ()
        # -static: link everything statically, including libc, libstdc++
        # and the dynamic loader, so the binary has no shared-library
        # dependencies at all. The Qt build in the docker image is made for
        # this: it is built with BUILD_SHARED_LIBS=OFF and all third-party
        # deps (zlib, freetype, fontconfig, dbus, ...) are static archives.
        string(APPEND CMAKE_EXE_LINKER_FLAGS " -static")
    endif()
endif()

# otherwise cmake --install tries to install all the dependeencies
install(CODE "set(CMAKE_INSTALL_LOCAL_ONLY true)")

# The wayland client (the WaylandClient module and the
# QWaylandIntegrationPlugin platform plugin) is part of qtbase, and Qt's
# CMake package auto-links the default static plugins (see the
# NOTES_STATIC_BUILD block below).
find_package(Qt6 REQUIRED COMPONENTS Widgets Xml Svg Network)

set(PROJECT_SOURCES
    src/precomp.h

    src/AddNewNoteDialog.c++
    src/AddNewNoteDialog.h
    src/App.c++
    src/App.h
    src/ByteArraySerializer.h
    src/ColorStrippingTextEdit.c++
    src/ColorStrippingTextEdit.h
    src/Config.c++
    src/Config.h
    src/Downloader.c++
    src/Downloader.h
    src/Exceptions.c++
    src/Exceptions.h
    src/HtmlColorStripper.c++
    src/HtmlColorStripper.h
    src/KdeLookStyle.c++
    src/KdeLookStyle.h
    src/MainWindow.c++
    src/MainWindow.h
    src/Note.c++
    src/Note.h
    src/NoteEditor.c++
    src/NoteEditor.h
    src/NoteEditorSearchHighlighter.c++
    src/NoteEditorSearchHighlighter.h
    src/NotesTree.c++
    src/NotesTree.h
    src/NotesTreeModel.c++
    src/NotesTreeModel.h
    src/main.c++

    res/qml.qrc

    src/AddNewNoteDialog.ui
    src/MainWindow.ui
    src/NoteEditor.ui
    src/NotesTree.ui
)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
qt_add_executable(NotesTree ${PROJECT_SOURCES})
target_include_directories(NotesTree PRIVATE src)
target_precompile_headers(NotesTree PRIVATE src/precomp.h)
target_link_libraries(NotesTree PRIVATE
    Qt6::Widgets
    Qt6::Xml
    Qt6::Svg
    Qt6::Network
)

if (NOTES_STATIC_BUILD)
    # With a static Qt the plugins that would normally be dlopen()ed are
    # linked in automatically by Qt's own CMake package: find_package(Qt6)
    # pulls in the plugin targets belonging to every module that is linked,
    # and linking those modules links the "default" plugins - the platform
    # plugin selected by QT_QPA_PLATFORMS when Qt was built (wayland,
    # software rendered: the docker build compiles OpenGL/EGL out, so the
    # wayland_egl path does not exist), the OpenSSL TLS backend, the
    # imageformats (jpeg, gif, ...) and the svg plugins - together with
    # their Q_IMPORT_PLUGIN initialization stubs. qt_import_plugins() is
    # only needed to override that default set (INCLUDE_BY_TYPE, EXCLUDE,
    # ...), which we don't need.
    #
    # Qt's CMake package already references the static archives of every
    # system library it found (freetype, harfbuzz, fontconfig, xkbcommon,
    # wayland, dbus-1, zlib, ...), but two things are missing from that:
    #
    # 1. The *private* dependencies of those libraries: Qt's own
    #    pkg-config lookups run without --static, so Requires.private
    #    entries never reach the link line. The archives missing because
    #    of that are the leaves of the dependency chains:
    #      graphite2         Alpine's harfbuzz is built with the graphite2
    #                        shaper, so libharfbuzz.a references gr_*
    #      brotlidec/common  FreeType's WOFF2 support
    #      libffi            the wayland client's closure marshalling
    #      expat             D-Bus's XML parser
    # 2. Link-order insurance: the linker scans archives in a single
    #    left-to-right pass, so providers must come after their
    #    consumers; the order inside Qt's interface libraries is not
    #    guaranteed (harfbuzz needs freetype, freetype needs
    #    zlib/bzip2/libpng/brotli).
    #
    # Both are covered by appending the whole provider set - in
    # topological order, consumers first - via pkg-config: the .pc files
    # provide the names and paths, and the docker image deletes the .so
    # symlinks, so every -l flag resolves to the archive. Archives whose
    # members were already pulled earlier on the line contribute
    # nothing, so the duplicates are harmless. Letting pkg-config resolve
    # the chains itself (pkg_check_modules STATIC) is deliberately not
    # done: harfbuzz's private deps would then drag in glib/intl/pcre2,
    # which nothing here links.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NOTES_DEPS REQUIRED IMPORTED_TARGET
        fontconfig harfbuzz graphite2 freetype2 libbrotlidec
        libbrotlicommon libpng bzip2 zlib expat libffi)
    target_link_libraries(NotesTree PRIVATE PkgConfig::NOTES_DEPS)
endif()

include(GNUInstallDirs)
install(TARGETS NotesTree
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    NotesTree.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)
install(FILES
    NotesTree.svg     DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
)

# The tests only need QtCore and QtTest; see tests/CMakeLists.txt, which can
# also be configured on its own without the app.
option(NOTES_BUILD_TESTS "Build the unit tests." OFF)
if (NOTES_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
