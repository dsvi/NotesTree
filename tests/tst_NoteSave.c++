#include <QtTest>

#include <QTemporaryDir>
#include <QTextEdit>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>

#include "App.h"
#include "Note.h"
#include "NoteEditor.h"
#include "NotesTreeModel.h"

/**
 * What a save is allowed to throw away.
 *
 * Saving a note makes two destructive decisions: an embed the html no longer
 * references is deleted, and a note with nothing in it is written as an empty
 * file. Both are easy to get wrong in a way that costs content instead of
 * tidying up, so they are checked against real notes in a real directory.
 *
 * The editor is the real NoteEditor, because "is this note empty" is decided
 * there, on the document, and not on the html string that reaches Note.
 */
class NoteSaveTest : public QObject
{
	Q_OBJECT

private:
	std::unique_ptr<QTemporaryDir>  tmp_;
	std::filesystem::path           rootPath_;
	/// declared before model_ on purpose: the model only borrows the note
	std::unique_ptr<Note>           rootNote_;
	std::unique_ptr<NotesTreeModel> model_;
	std::unique_ptr<NoteEditor>     editor_;
	/// NoteEditor keeps its ui private; owned by editor_
	QTextEdit *                     edit_ = nullptr;

	static void flush()
	{
		for (int i = 0; i < 5; ++i)
			QCoreApplication::processEvents();
	}

	std::weak_ptr<Note> noteWeak(const QString &name)
	{
		for (int r = 0; r < model_->rowCount(QModelIndex()); ++r){
			auto i = model_->index(r, 0, QModelIndex());
			if (model_->data(i, Qt::EditRole).toString() != name)
				continue;
			return model_->noteAt(i)->note;
		}
		return std::weak_ptr<Note>();
	}

	/// a note's own files all sit next to each other, named after it:
	/// <name>.html, <name>.embed/, <name>/ for its subnotes
	std::filesystem::path noteFile(const QString &name)
	{
		return rootPath_ / (name.toStdString() + ".html");
	}

	std::filesystem::path embedFile(const QString &note, const QString &file)
	{
		return rootPath_ / (note.toStdString() + ".embed") / file.toStdString();
	}

	/// an embed file, as pasteImage() or a download would have left it
	void putEmbed(const QString &note, const QString &file)
	{
		std::filesystem::create_directories(embedFile(note, file).parent_path());
		std::ofstream out(embedFile(note, file));
		out << "not really a png";
	}

	static QString read(const std::filesystem::path &p)
	{
		std::ifstream in(p);
		std::ostringstream ss;
		ss << in.rdbuf();
		return QString::fromStdString(ss.str());
	}

private slots:
	/// one note "n", empty, in an otherwise empty hierarchy
	void init()
	{
		tmp_ = std::make_unique<QTemporaryDir>();
		QVERIFY(tmp_->isValid());
		rootPath_ = toPath(tmp_->path()) / "notes";
		std::error_code ec;
		std::filesystem::create_directories(rootPath_, ec);
		QVERIFY(!ec);

		rootNote_ = std::make_unique<Note>();
		rootNote_->createHierarchyFromRoot(rootPath_);

		model_ = std::make_unique<NotesTreeModel>();
		model_->root(rootNote_.get());

		rootNote_->createSubnote("n");
		flush();
		QVERIFY(!noteWeak("n").expired());

		editor_ = std::make_unique<NoteEditor>();
		edit_ = editor_->findChild<QTextEdit*>();
		QVERIFY(edit_);
	}

	void cleanup()
	{
		// the model's NoteInTree proxies are the ones talking to the notes,
		// so they have to go first
		edit_ = nullptr;
		editor_.reset();
		model_.reset();
		rootNote_.reset();
		tmp_.reset();
	}

	// ---- a note keeps what it holds -----------------------------------

	/// a table with empty cells has no plain text, but it is not an empty note
	void emptyTableSurvivesASave()
	{
		editor_->editTextFor(noteWeak("n"));
		edit_->insertHtml("<table><tr><td></td><td></td></tr>"
		                  "<tr><td></td><td></td></tr></table>");
		editor_->save();
		QVERIFY(std::filesystem::exists(noteFile("n")));
		auto html = read(noteFile("n"));
		QVERIFY2(html.contains("<table"), qPrintable(html));
	}

	/// an image is content, and the file it points at has to stay
	void imageOnlyNoteKeepsItsEmbed()
	{
		putEmbed("n", "pic.png");
		editor_->editTextFor(noteWeak("n"));
		edit_->insertHtml("<p><img src=\"./n.embed/pic.png\"></p>");
		editor_->save();
		QVERIFY(std::filesystem::exists(embedFile("n", "pic.png")));
		auto html = read(noteFile("n"));
		QVERIFY2(html.contains("pic.png"), qPrintable(html));
	}

	// ---- what the embed pruning is allowed to remove ------------------

	/// an embed nothing references any more is removed
	void unreferencedEmbedIsRemoved()
	{
		auto note = noteWeak("n").lock();
		QVERIFY(note);
		putEmbed("n", "pic.png");
		note->save("<html><body><p>text</p></body></html>");
		QVERIFY(!std::filesystem::exists(embedFile("n", "pic.png")));
	}

	/// html that could not be parsed says nothing about the embeds, so they
	/// must not be judged by it
	void unparseableHtmlKeepsTheEmbeds()
	{
		auto note = noteWeak("n").lock();
		QVERIFY(note);
		putEmbed("n", "pic.png");
		note->save("<html><body><p>text</p>");
		QVERIFY(std::filesystem::exists(embedFile("n", "pic.png")));
	}

	// ---- a note the user really did empty -----------------------------

	/// clearing the editor empties the note, and its embeds go with it
	void clearedNoteIsEmptied()
	{
		putEmbed("n", "pic.png");
		editor_->editTextFor(noteWeak("n"));
		edit_->setHtml("<p>text</p><p><img src=\"./n.embed/pic.png\"></p>");
		edit_->clear();
		editor_->save();
		auto f = noteFile("n");
		QVERIFY2(!std::filesystem::exists(f) || std::filesystem::file_size(f) == 0,
		         qPrintable(read(f)));
		QVERIFY(!std::filesystem::exists(embedFile("n", "pic.png")));
	}
};

/**
 * App is the application object itself (see App.h), so the test has to build it
 * rather than a plain QApplication: NoteEditor's toolbar and Note's error
 * reporting both go through app().
 */
int main(int argc, char **argv)
{
	App a(argc, argv);
	NoteSaveTest t;
	return QTest::qExec(&t, argc, argv);
}

#include "tst_NoteSave.moc"
