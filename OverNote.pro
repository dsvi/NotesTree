TEMPLATE = app
LANGUAGE  = C++
OBJECTS_DIR = ./tmp
#TARGET =
QT += qml quick
CONFIG += precompile_header
CONFIG += c++14

#QMAKE_CXXFLAGS += -std=c++14

CONFIG(debug, debug|release) {
  DEFINES += DEBUG
}

PRECOMPILED_HEADER = src/precomp.h

SOURCES += \
    src/main.cpp \
    src/Note.cpp \
    src/NotesTreeModel.cpp \
    src/FilenameEncoder.cpp \
    src/QMLApp.cpp

RESOURCES += res/qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.

HEADERS +=  src/precomp.h \
    src/Note.h \
    src/NotesTreeModel.h \
    src/FilenameEncoder.h \
    src/QMLApp.h



