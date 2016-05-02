TEMPLATE = app
QMAKE_EXT_CPP += c++
OBJECTS_DIR = ./tmp
MOC_DIR = ./tmp
UI_DIR = ./tmp
RCC_DIR = ./tmp
#TARGET =
QT += widgets webkitwidgets network
CONFIG += precompile_header
CONFIG += c++14

#QMAKE_CXXFLAGS += -std=c++14
#QMAKE_LFLAGS   += -std=c++14

№QMAKE_CXXFLAGS += -stdlib=libc++ -std=c++14
№QMAKE_LFLAGS   += -stdlib=libc++ -std=c++14

INCLUDEPATH += /home/ds/boost
LIBS += -L/home/ds/boost/stage/lib -lboost_filesystem -lboost_system

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

# Default rules for deployment.

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


