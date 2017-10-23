TEMPLATE = app
QMAKE_EXT_CPP += c++

TMP = ./tmp
OBJECTS_DIR = $$TMP
MOC_DIR = $$TMP
UI_DIR = $$TMP
RCC_DIR = $$TMP

target.path = /usr/bin
INSTALLS += target
desktop.path = /usr/share/applications
desktop.files = NotesTree.desktop
INSTALLS += desktop
icons.path = /usr/share/icons/hicolor/scalable/apps
icons.files = NotesTree.svg
INSTALLS += icons

QT += widgets webkitwidgets network svg
CONFIG += precompile_header
CONFIG += c++14

LIBS += -lboost_filesystem -lboost_system

CONFIG(debug, debug|release) {
  DEFINES += DEBUG
}

CONFIG(release, debug|release) {
  DEFINES += NDEBUG
}

# remove possible other optimization flags
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
# add the desired -O3 if not present
QMAKE_CXXFLAGS_RELEASE += -O3


PRECOMPILED_HEADER = src/precomp.h
INCLUDEPATH += src

SOURCES += \
    src/main.c++ \
    src/Note.c++ \
    src/NotesTreeModel.c++ \
    src/MainWindow.c++ \
    src/App.c++ \
    src/Exceptions.c++ \
    src/AddNewNoteDialog.c++ \
    src/NoteEditor.c++ \
    src/NotesTree.c++ \
    src/Downloader.c++ \
    src/Config.c++

RESOURCES += res/qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

HEADERS +=  src/precomp.h \
    src/Note.h \
    src/NotesTreeModel.h \
    src/MainWindow.h \
    src/App.h \
    src/Exceptions.h \
    src/ByteArraySerializer.h \
    src/AddNewNoteDialog.h \
    src/NoteEditor.h \
    src/NotesTree.h \
    src/Downloader.h \
    src/Config.h

FORMS    += src/MainWindow.ui \
    src/AddNewNoteDialog.ui \
    src/NoteEditor.ui \
    src/NotesTree.ui



