#include <QtTest>

#include <QAbstractItemView>
#include <QApplication>
#include <QDropEvent>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTreeView>

#include <filesystem>
#include <memory>

#include "Note.h"
#include "NoteSearchProxyModel.h"
#include "NotesTreeModel.h"

/**
 * Drag and drop of notes in the tree.
 *
 * A note is a file on disk (plus a dir for its subnotes), so the only thing a
 * drop can honestly do is move it. A copy would have to invent a second name
 * that stays unique among the subnotes, and link/ignore mean nothing for a
 * note. The model says so through supportedDropActions(), and dropMimeData()
 * refuses every other action instead of performing a move while the caller
 * believes it copied.
 *
 * The notes here are real ones, in a real temporary directory, because what a
 * drop is worth is where the note's file ends up, not only what the model
 * claims.
 *
 * Caveat: Note reports its own problems through app(), which in a test is not
 * a real App, so the tests deliberately stay away from the operations that
 * would make a note complain (a name clash, a note into its own subnote, ...).
 */
class NotesTreeModelDnDTest : public QObject
{
	Q_OBJECT

private:
	std::unique_ptr<QTemporaryDir> tmp_;
	std::filesystem::path          rootPath_;
	/// declared before model_ on purpose: the model only borrows the note
	std::unique_ptr<Note>          rootNote_;
	std::unique_ptr<NotesTreeModel> model_;

	/// notes are announced to the model through queued invocations, so they
	/// show up in it only after the event loop had a turn
	static void flush()
	{
		for (int i = 0; i < 5; ++i)
			QCoreApplication::processEvents();
	}

	QModelIndex findIndex(const QModelIndex &parent, const QString &name)
	{
		for (int r = 0; r < model_->rowCount(parent); ++r){
			auto i = model_->index(r, 0, parent);
			if (model_->data(i, Qt::EditRole).toString() == name)
				return i;
		}
		return QModelIndex();
	}

	Note *noteNamed(const QString &name, const QModelIndex &parent = QModelIndex())
	{
		auto i = findIndex(parent, name);
		if (!i.isValid())
			return nullptr;
		return model_->noteAt(i)->note.lock().get();
	}

	/// all notes below parent, parent itself excluded
	int countNotes(const QModelIndex &parent = QModelIndex())
	{
		int ret = 0;
		for (int r = 0; r < model_->rowCount(parent); ++r)
			ret += 1 + countNotes(model_->index(r, 0, parent));
		return ret;
	}

	static bool onDisk(const std::filesystem::path &p)
	{
		return std::filesystem::exists(p);
	}

	/// what mimeData() hands to the drag for the given source indexes
	std::unique_ptr<QMimeData> payloadFor(const QModelIndexList &srcIndexes)
	{
		return std::unique_ptr<QMimeData>(model_->mimeData(srcIndexes));
	}

	/// A tree view the test can hand a drop event to directly: QApplication::notify()
	/// reroutes every Drop event to the drag manager's current target, so a
	/// synthetic one never reaches a widget, and QDrag::exec() would need a real
	/// windowing system.
	///
	/// The mode is DragDrop rather than the app's InternalMove: InternalMove only
	/// accepts a drop coming from a drag started by this very view, which only a
	/// real QDrag can express. DragDrop is the mode where the view hands the
	/// action of the event over to the model, and that is what is under test.
	struct DropableView : QTreeView
	{
		using QTreeView::QTreeView;
		void drop(QDropEvent *event){ dropEvent(event); }
	};

	/// the model/view chain NotesTree::root() builds: the model behind the
	/// search proxy
	void setupView(QTreeView &view, NoteSearchProxyModel &proxy)
	{
		proxy.setSourceModel(model_.get());
		view.setModel(&proxy);
		view.setDragEnabled(true);
		view.setDragDropMode(QAbstractItemView::DragDrop);
		view.setHeaderHidden(true);
		view.resize(300, 400);
		view.show();
		// the rows need a layout pass before their rects mean anything
		for (int i = 0; i < 20 && view.visualRect(proxy.index(0, 0)).isEmpty(); ++i)
			QTest::qWait(25);
	}

private slots:
	/// the tree every test starts from:
	/// 🌳 ├─ a ├─ a1 │  └─ a2 └─ b
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

		rootNote_->createSubnote("a");
		rootNote_->createSubnote("b");
		flush();
		auto a = noteNamed("a");
		QVERIFY(a);
		a->createSubnote("a1");
		a->createSubnote("a2");
		flush();
		QCOMPARE(countNotes(), 4);
	}

	void cleanup()
	{
		// the model's NoteInTree proxies are the ones talking to the notes,
		// so they have to go first
		model_.reset();
		rootNote_.reset();
		tmp_.reset();
	}
	// ---- what the model offers to the view ------------------------------

	void onlyMovingIsOffered()
	{
		QCOMPARE(model_->supportedDropActions(), Qt::MoveAction);
		// the view intersects both of these to get the actions it may propose
		QCOMPARE(model_->supportedDragActions(), Qt::MoveAction);
		QCOMPARE(model_->mimeTypes(), QStringList{QStringLiteral("application/x.notes-tree")});

		auto fl = model_->flags(findIndex(QModelIndex(), "a"));
		QVERIFY(fl.testFlag(Qt::ItemIsEnabled));
		QVERIFY(fl.testFlag(Qt::ItemIsDragEnabled));
		QVERIFY(fl.testFlag(Qt::ItemIsDropEnabled));
	}

	void dragPayloadCarriesTheNotes()
	{
		auto a1 = findIndex(findIndex(QModelIndex(), "a"), "a1");
		auto mime = payloadFor({a1});
		QVERIFY(mime->hasFormat(model_->mimeTypes().first()));
		QVERIFY(!mime->data(model_->mimeTypes().first()).isEmpty());
	}

	// ---- a move, which is what a drop is supposed to do ----------------

	void moveNoteIntoAnotherNote()
	{
		auto a = findIndex(QModelIndex(), "a");
		auto b = findIndex(QModelIndex(), "b");
		auto a1 = findIndex(a, "a1");
		QVERIFY(a1.isValid());

		QSignalSpy aboutToRemove(model_.get(), &QAbstractItemModel::rowsAboutToBeRemoved);
		QSignalSpy inserted(model_.get(), &QAbstractItemModel::rowsInserted);

		QVERIFY(model_->dropMimeData(payloadFor({a1}).get(), Qt::MoveAction, -1, 0, b));
		flush();

		QCOMPARE(aboutToRemove.count(), 1);
		QCOMPARE(inserted.count(), 1);

		// in the model: a1 left a and is b's only child now
		QCOMPARE(model_->rowCount(findIndex(QModelIndex(), "a")), 1);
		auto newB = findIndex(QModelIndex(), "b");
		QCOMPARE(model_->rowCount(newB), 1);
		QCOMPARE(model_->data(model_->index(0, 0, newB), Qt::EditRole).toString(), QStringLiteral("a1"));
		QCOMPARE(countNotes(), 4);

		// and on disk
		QVERIFY(onDisk(rootPath_ / "b" / "a1.html"));
		QVERIFY(!onDisk(rootPath_ / "a" / "a1.html"));
	}

	void moveNoteUpToTheRoot()
	{
		auto a = findIndex(QModelIndex(), "a");
		auto a1 = findIndex(a, "a1");

		QVERIFY(model_->dropMimeData(payloadFor({a1}).get(), Qt::MoveAction, -1, 0, QModelIndex()));
		flush();

		QCOMPARE(model_->rowCount(QModelIndex()), 3);
		QVERIFY(findIndex(QModelIndex(), "a1").isValid());
		QVERIFY(onDisk(rootPath_ / "a1.html"));
	}

	void moveSeveralNotesInOneDrop()
	{
		auto a = findIndex(QModelIndex(), "a");
		auto b = findIndex(QModelIndex(), "b");
		auto mime = payloadFor({findIndex(a, "a1"), findIndex(a, "a2")});

		QVERIFY(model_->dropMimeData(mime.get(), Qt::MoveAction, -1, 0, b));
		flush();

		QCOMPARE(model_->rowCount(findIndex(QModelIndex(), "a")), 0);
		auto newB = findIndex(QModelIndex(), "b");
		QCOMPARE(model_->rowCount(newB), 2);
		QVERIFY(onDisk(rootPath_ / "b" / "a1.html"));
		QVERIFY(onDisk(rootPath_ / "b" / "a2.html"));
	}

	/// a note is dropped *into* another note, never next to it: the row the
	/// view passes for a drop between two rows is ignored
	void dropRowLandsInsideTheNoteNotBesideIt()
	{
		auto a = findIndex(QModelIndex(), "a");
		auto b = findIndex(QModelIndex(), "b");
		auto bNote = noteNamed("b");
		QVERIFY(bNote);
		bNote->createSubnote("b1");
		flush();

		QVERIFY(model_->dropMimeData(payloadFor({findIndex(a, "a1")}).get(),
		                                  Qt::MoveAction, /*row=*/0, /*column=*/0, b));
		flush();

		auto newB = findIndex(QModelIndex(), "b");
		QCOMPARE(model_->rowCount(newB), 2);
		// still only a and b at the top, i.e. a1 did not become a sibling of b
		QCOMPARE(model_->rowCount(QModelIndex()), 2);
		QVERIFY(onDisk(rootPath_ / "b" / "a1.html"));
	}

	/// dropping a note onto the note it is already a child of changes nothing
	void dropOnItsOwnParentIsANoOp()
	{
		auto a = findIndex(QModelIndex(), "a");
		auto a1 = findIndex(a, "a1");

		QVERIFY(model_->dropMimeData(payloadFor({a1}).get(), Qt::MoveAction, -1, 0, a));
		flush();

		QCOMPARE(countNotes(), 4);
		QCOMPARE(model_->rowCount(findIndex(QModelIndex(), "a")), 2);
		QVERIFY(onDisk(rootPath_ / "a" / "a1.html"));
	}

	// ---- everything else is refused ------------------------------------

	void nonMoveActionIsRejected_data()
	{
		QTest::addColumn<int>("action");
		QTest::newRow("copy")       << int(Qt::CopyAction);
		QTest::newRow("link")       << int(Qt::LinkAction);
		QTest::newRow("targetMove") << int(Qt::TargetMoveAction);
		QTest::newRow("ignore")     << int(Qt::IgnoreAction);
		QTest::newRow("no action")  << int(Qt::DropAction(0));
	}

	void nonMoveActionIsRejected()
	{
		QFETCH(int, action);
		const auto act = Qt::DropAction(action);

		auto a = findIndex(QModelIndex(), "a");
		auto b = findIndex(QModelIndex(), "b");
		auto a1 = findIndex(a, "a1");
		auto mime = payloadFor({a1});

		QSignalSpy aboutToRemove(model_.get(), &QAbstractItemModel::rowsAboutToBeRemoved);
		QSignalSpy inserted(model_.get(), &QAbstractItemModel::rowsInserted);

		QVERIFY(!model_->dropMimeData(mime.get(), act, -1, 0, b));
		flush();

		// the model is not even told about a change
		QCOMPARE(aboutToRemove.count(), 0);
		QCOMPARE(inserted.count(), 0);

		// the note is exactly where it was, in the model and on disk
		QCOMPARE(model_->rowCount(findIndex(QModelIndex(), "a")), 2);
		QCOMPARE(model_->rowCount(findIndex(QModelIndex(), "b")), 0);
		QVERIFY(onDisk(rootPath_ / "a" / "a1.html"));
		QVERIFY(!onDisk(rootPath_ / "b" / "a1.html"));
		QCOMPARE(countNotes(), 4);

		// and refusing did not break the model: a real move still works
		QVERIFY(model_->dropMimeData(mime.get(), Qt::MoveAction, -1, 0, b));
		flush();
		QVERIFY(onDisk(rootPath_ / "b" / "a1.html"));
	}

	// ---- the tree view, which is dropMimeData()'s only caller in the app --

	/// the view asks the model to move and the model moves it: the mime type,
	/// the item flags and the search proxy in between all have to line up
	void viewMoveDropMovesTheNote()
	{
		NoteSearchProxyModel proxy;
		DropableView view;
		setupView(view, proxy);

		auto aSrc = findIndex(QModelIndex(), "a");
		auto a1Src = findIndex(aSrc, "a1");
		auto bView = proxy.mapFromSource(findIndex(QModelIndex(), "b"));
		QVERIFY(bView.isValid());
		auto target = view.visualRect(bView);
		QVERIFY(!target.isEmpty());

		auto mime = payloadFor({a1Src});
		QDropEvent event(target.center(), Qt::MoveAction, mime.get(), Qt::LeftButton, Qt::NoModifier);
		view.drop(&event);

		QVERIFY(event.isAccepted());
		QCOMPARE(event.dropAction(), Qt::MoveAction);
		flush();
		QVERIFY(onDisk(rootPath_ / "b" / "a1.html"));
		QVERIFY(!onDisk(rootPath_ / "a" / "a1.html"));
	}

	/// a drop that asks for a copy (Ctrl held): the view proposes CopyAction, the
	/// model refuses it, so the drop is not accepted and the note stays exactly
	/// where it was
	void viewCopyDropIsRefused()
	{
		NoteSearchProxyModel proxy;
		DropableView view;
		setupView(view, proxy);

		auto aSrc = findIndex(QModelIndex(), "a");
		auto a1Src = findIndex(aSrc, "a1");
		auto target = view.visualRect(proxy.mapFromSource(findIndex(QModelIndex(), "b")));
		QVERIFY(!target.isEmpty());

		auto mime = payloadFor({a1Src});
		QDropEvent event(target.center(), Qt::CopyAction | Qt::MoveAction, mime.get(),
		                 Qt::LeftButton, Qt::ControlModifier);
		QCOMPARE(event.dropAction(), Qt::CopyAction); // what the drag is asking for
		view.drop(&event);

		QVERIFY(!event.isAccepted());
		flush();
		QCOMPARE(countNotes(), 4);
		QVERIFY(findIndex(findIndex(QModelIndex(), "a"), "a1").isValid());
		QVERIFY(onDisk(rootPath_ / "a" / "a1.html"));
		QVERIFY(!onDisk(rootPath_ / "b" / "a1.html"));
	}
};

QTEST_MAIN(NotesTreeModelDnDTest)
#include "tst_NotesTreeModel.moc"
