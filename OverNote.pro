TEMPLATE = app
QMAKE_EXT_CPP += c++
OBJECTS_DIR = ./tmp
#TARGET =
QT += widgets
CONFIG += precompile_header
#CONFIG += c++14

QMAKE_CXXFLAGS += -stdlib=libc++ -std=c++14
QMAKE_LFLAGS   += -stdlib=libc++ -std=c++14
LIBS += -lboost_filesystem -lboost_system

CONFIG(debug, debug|release) {
  DEFINES += DEBUG
}

PRECOMPILED_HEADER = src/precomp.h

SOURCES += \
    src/main.c++ \
    src/Note.c++ \
    src/NotesTreeModel.c++ \
    src/MainWindow.c++ \
    src/App.c++ \
    src/Exceptions.c++ \
    src/NotesTreeActions.c++ \
    src/AddNewNoteDialog.c++

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
    src/NotesTreeActions.h \
    src/AddNewNoteDialog.h

FORMS    += src/MainWindow.ui \
    src/AddNewNoteDialog.ui


