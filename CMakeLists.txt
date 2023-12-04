cmake_minimum_required(VERSION 3.16)

# Set the default CMAKE_BUILD_TYPE to Release.
# This should be done before the project command since the latter can set
# CMAKE_BUILD_TYPE itself (it does so for nmake).
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

project(NotesTree VERSION 1.2 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#    message("Using libc++")
#    string(APPEND CMAKE_CXX_FLAGS " -stdlib=libc++")
    string(APPEND CMAKE_CXX_FLAGS " -Wno-unqualified-std-cast-call")
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -fuse-ld=lld")
    #string(APPEND CMAKE_CXX_FLAGS_DEBUG " -fsanitize=address -fsanitize-address-use-after-scope -fno-omit-frame-pointer")
endif()

option(NOTES_STATIC_BUILD "Try to build static version of the program, without dependency on any shared library." ON)

if (NOTES_STATIC_BUILD)
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -static-libstdc++ -static-libgcc")
endif()

# otherwise cmake --install tries to install all the dependeencies
install(CODE "set(CMAKE_INSTALL_LOCAL_ONLY true)")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
#set(QT_VERSION_MAJOR 6)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Xml Svg Network)

set(PROJECT_SOURCES
    src/precomp.h

    src/AddNewNoteDialog.c++
    src/AddNewNoteDialog.h
    src/App.c++
    src/App.h
    src/ByteArraySerializer.h
    src/Config.c++
    src/Config.h
    src/Downloader.c++
    src/Downloader.h
    src/Exceptions.c++
    src/Exceptions.h
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

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(NotesTree
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET untitled APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    # if(ANDROID)
    #     add_library(untitled SHARED
    #         ${PROJECT_SOURCES}
    #     )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    # else()
        add_executable(NotesTree
            ${PROJECT_SOURCES}
        )
    # endif()
endif()
target_include_directories(NotesTree PRIVATE src)
target_precompile_headers(NotesTree PRIVATE src/precomp.h)
target_link_libraries(NotesTree PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets 
    Qt${QT_VERSION_MAJOR}::Xml 
    Qt${QT_VERSION_MAJOR}::Svg 
    Qt${QT_VERSION_MAJOR}::Network
)

if (NOTES_STATIC_BUILD)
    target_link_libraries(NotesTree PRIVATE 
        Qt${QT_VERSION_MAJOR}::WaylandClientPrivate
    )
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER org.ds.NotesTree)
endif()
set_target_properties(NotesTree PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS NotesTree
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(NotesTree)
endif()
