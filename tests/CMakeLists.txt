cmake_minimum_required(VERSION 3.21)

# Standalone on purpose: the tests only need Qt + QtTest, so they can be built
# without configuring the app (tst_NotesTreeModel compiles some of the app's
# own sources, but does not need its CMakeLists):
#   cmake -S tests -B tests/build && cmake --build tests/build
#   ctest --test-dir tests/build --output-on-failure
# They are also added from the top level CMakeLists.txt with -DNOTES_BUILD_TESTS=ON.
project(NotesTreeTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
# tst_NotesTreeModel compiles sources whose widgets come from a .ui
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test Svg Xml Network)

# the html fixtures the tests read; passed to the test code as a string
set(NOTES_TEST_FIXTURES "${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

# the colour stripping itself
qt_add_executable(tst_HtmlColorStripper
    tst_HtmlColorStripper.c++
    ../src/HtmlColorStripper.c++
    ../src/HtmlColorStripper.h
)
target_include_directories(tst_HtmlColorStripper PRIVATE ../src)
target_compile_definitions(tst_HtmlColorStripper PRIVATE NOTES_TEST_FIXTURES="${NOTES_TEST_FIXTURES}")
target_link_libraries(tst_HtmlColorStripper PRIVATE Qt6::Core Qt6::Test)

# the widget that applies it to everything pasted into a note
qt_add_executable(tst_ColorStrippingTextEdit
    tst_ColorStrippingTextEdit.c++
    ../src/ColorStrippingTextEdit.c++
    ../src/ColorStrippingTextEdit.h
    ../src/HtmlColorStripper.c++
    ../src/HtmlColorStripper.h
    ../src/NoteTableStyle.c++
    ../src/NoteTableStyle.h
)
target_include_directories(tst_ColorStrippingTextEdit PRIVATE ../src)
target_compile_definitions(tst_ColorStrippingTextEdit PRIVATE NOTES_TEST_FIXTURES="${NOTES_TEST_FIXTURES}")
target_link_libraries(tst_ColorStrippingTextEdit PRIVATE Qt6::Core Qt6::Widgets Qt6::Test)

# the table look the editor gives to every table of a note
qt_add_executable(tst_NoteTableStyle
    tst_NoteTableStyle.c++
    ../src/NoteTableStyle.c++
    ../src/NoteTableStyle.h
    ../src/HtmlColorStripper.c++
    ../src/HtmlColorStripper.h
)
target_include_directories(tst_NoteTableStyle PRIVATE ../src)
target_compile_definitions(tst_NoteTableStyle PRIVATE NOTES_TEST_FIXTURES="${NOTES_TEST_FIXTURES}")
target_link_libraries(tst_NoteTableStyle PRIVATE Qt6::Core Qt6::Gui Qt6::Test)

# the notes tree model and its drag and drop. The drop has to end up on disk
# to be worth testing, so this one works with real Notes, which drags in the
# rest of the app's sources: Note.c++ uses VisitSrcUrls() from NoteEditor.c++
# (and through it the editor widget with its custom text edit), and both reach
# for app(), so App/Config/Downloader/KdeLookStyle have to link too.
qt_add_executable(tst_NotesTreeModel
    tst_NotesTreeModel.c++
    ../src/NotesTreeModel.c++
    ../src/NotesTreeModel.h
    ../src/ByteArraySerializer.h
    ../src/Note.c++
    ../src/Note.h
    ../src/NoteSearchProxyModel.c++
    ../src/NoteSearchProxyModel.h
    ../src/NoteEditor.c++
    ../src/NoteEditor.h
    ../src/NoteEditor.ui
    ../src/NoteEditorSearchHighlighter.c++
    ../src/NoteEditorSearchHighlighter.h
    ../src/ColorStrippingTextEdit.c++
    ../src/ColorStrippingTextEdit.h
    ../src/HtmlColorStripper.c++
    ../src/HtmlColorStripper.h
    ../src/NoteTableStyle.c++
    ../src/NoteTableStyle.h
    ../src/Exceptions.c++
    ../src/Exceptions.h
    ../src/App.c++
    ../src/App.h
    ../src/Config.c++
    ../src/Config.h
    ../src/Downloader.c++
    ../src/Downloader.h
    ../src/KdeLookStyle.c++
    ../src/KdeLookStyle.h
    ../src/precomp.h
)
target_include_directories(tst_NotesTreeModel PRIVATE ../src)
# the app's sources expect it: Note.h has no includes of its own
target_precompile_headers(tst_NotesTreeModel PRIVATE ../src/precomp.h)
target_link_libraries(tst_NotesTreeModel PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test
    Qt6::Svg Qt6::Xml Qt6::Network
)

enable_testing()
add_test(NAME HtmlColorStripper COMMAND tst_HtmlColorStripper)
add_test(NAME ColorStrippingTextEdit COMMAND tst_ColorStrippingTextEdit)
add_test(NAME NoteTableStyle COMMAND tst_NoteTableStyle)
add_test(NAME NotesTreeModel COMMAND tst_NotesTreeModel)
set_tests_properties(ColorStrippingTextEdit PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
set_tests_properties(NoteTableStyle PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
set_tests_properties(NotesTreeModel PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
