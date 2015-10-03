TEMPLATE = app
QMAKE_EXT_CPP += c++
OBJECTS_DIR = ./tmp
#TARGET =
QT += widgets
CONFIG += precompile_header
CONFIG += c++14

#QMAKE_CXXFLAGS += -std=c++14

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
    src/FileSystem.c++ \
    src/Exceptions.c++

RESOURCES += res/qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.

HEADERS +=  src/precomp.h \
    src/Note.h \
    src/NotesTreeModel.h \
    src/MainWindow.h \
    src/App.h \
    src/FileSystem.h \
    src/Exceptions.h \
    src/ByteArraySerializer.h

FORMS    += src/MainWindow.ui


