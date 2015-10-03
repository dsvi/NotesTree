import qbs

QtGuiApplication {
	name: "OverNote"
	//type: "application" // To suppress bundle generation on Mac
	//destinationDirectory: "./out"
	files: [
        "res/qml.qrc",
        "src/App.c++",
        "src/App.h",
        "src/FilenameEncoder.c++",
        "src/FilenameEncoder.h",
        "src/MainWindow.c++",
        "src/MainWindow.h",
        "src/MainWindow.ui",
        "src/Note.c++",
        "src/Note.h",
        "src/NotesTreeModel.c++",
        "src/NotesTreeModel.h",
        "src/main.c++",
        "src/precomp.h",
    ]

//	Group {     // Properties for the produced executable
//		fileTagsFilter: product.type
//		qbs.install: true
//	}
	//qbs.toolchain: "clang"
	Properties {
			condition: qbs.buildVariant == "debug"
			//cpp.defines: base.concat("DEBUG")
			cpp.defines: ["DEBUG"]
			Qt.quick.qmlDebugging: true
	}
	Properties {
		condition: qbs.buildVariant == "release"
		cpp.optimization: "fast"
	}
	cpp.cxxLanguageVersion : "c++14"
	cpp.cxxPrecompiledHeader: "src/precomp.h"
	Depends { name: "Qt"; submodules: ["widgets", "svg"] }
	//Depends { name: "Qt"; submodules: ["core", "gui", "quick", "svg"] }
}

