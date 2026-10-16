cmake_minimum_required(VERSION 3.21)

# Standalone on purpose: the tests only need QtCore/QtWidgets + QtTest, so they
# can be built without configuring the whole app:
#   cmake -S tests -B tests/build && cmake --build tests/build
#   ctest --test-dir tests/build --output-on-failure
# They are also added from the top level CMakeLists.txt with -DNOTES_BUILD_TESTS=ON.
project(NotesTreeTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)

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

enable_testing()
add_test(NAME HtmlColorStripper COMMAND tst_HtmlColorStripper)
add_test(NAME ColorStrippingTextEdit COMMAND tst_ColorStrippingTextEdit)
add_test(NAME NoteTableStyle COMMAND tst_NoteTableStyle)
set_tests_properties(ColorStrippingTextEdit PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
set_tests_properties(NoteTableStyle PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
