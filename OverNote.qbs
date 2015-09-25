import qbs

QtGuiApplication {
	name: "OverNote"
	//type: "application" // To suppress bundle generation on Mac
	//destinationDirectory: "./out"
	files: [
        "res/qml.qrc",
        "src/Directory.cpp",
        "src/Directory.h",
        "src/Note.cpp",
        "src/Note.h",
        "src/NotesTreeModel.cpp",
        "src/NotesTreeModel.h",
        "src/main.cpp",
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
	//cpp.cxxLanguageVersion : "c++11"
	cpp.cppFlags: "-std=c++14"
	cpp.cxxPrecompiledHeader: "src/precomp.h"
	Depends { name: "Qt"; submodules: ["quick", "svg"] }
	//Depends { name: "Qt"; submodules: ["core", "gui", "quick", "svg"] }
}

