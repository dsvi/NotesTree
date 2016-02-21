#ifndef NOTE_H
#define NOTE_H


class Note : public QObject
{
	Q_OBJECT
public:
	Note();
	~Note();
	QString name() const;
	bool hasAttach();
signals:
	// signaled by root only
	void clear();
	void noteAdded(std::weak_ptr<Note> n);
	void noteRemoved();
	void nameChanged(const QString &name);
	void noteTextRdy(const QString &txt, const QString &basePath);
	void notePlainTextRdy(const QString &txt);
	void attachReady(const QString &attachDirPath);
public slots:

	/// create hierarchy of notes and subnotes from the root folder
	/// this note becames root of the hierarchy
	/// also does filesystem cleanups. (removes empty attach folders etc)
	void createHierarchyFromRoot(const boost::filesystem::path &path);
	void changeName(const QString &name);

	/// adds the notes as child to this, removes it from prev parent.
	/// when note with the same name exists here already, or was added in the process, RecoverableException is thrown
	void adopt(const std::vector<std::weak_ptr<Note>> &list);

	void createSubnote(const QString &name);
	void deleteRecursively(const std::vector<std::weak_ptr<Note> > &list);

	/// only one editor should exist at a time.
	/// the slot eventualy responds with the noteTextRdy() signal
	/// calls stopEditing() so all previous editors become invalid
	void startEditing();
	void save(QString html);
	void stopEditing();

	// emits notePlainTextRdy
	void getNotePlainTxt();

	/// creates attach, if did not exist. emits attachReady, if done
	void attach();
private:
	Note     *parent_ = nullptr; 	/// nullptr means this is root
	QString		name_; // root has path to root here, instead of name
	bool      isZombie(){  // the method is only valid for root
		return name_.isNull();
	}
	std::vector<std::shared_ptr<Note>> subNotes_;
	std::map<QString, QString>         urlsPatch_;
	std::set<QString>                  urlsInDownload_;

	/// populate list of subnotes and the name from the @path dir
	void addFromSubnotesDir(const boost::filesystem::path &path);
	/// pathname should end on textExt
	void createFromNoteTextFile(const boost::filesystem::path &textPathname);

	void move(const boost::filesystem::path &newPath, const boost::filesystem::path &newFileName);
	void adopt_(const std::shared_ptr<Note> &n);
	void cleanUpFileSystem();
	void ensureSubDirExist();
	Note *child(size_t ndx);
	const Note *child(size_t ndx) const;
	size_t findIndexOf(const Note *) const;
	/// true if a subnote with such name already here
	bool exist(const QString &name);
	/// returns the full path in hierarchy to this note, not including its name.
	/// or empty string, if the parent() of this note is nullptr
	/// example "notename1⮕notename2⮕notename3"
	QString makePathName(const QString separator = u8"⮕") const;
	/// returns number of parent nodes to this one. including root
	int hierarchyDepth() const;

	/// add the file as attached to the note
	//void attach(const QFileInfo &fi);

	boost::filesystem::path pathToNote() const;
	boost::filesystem::path attachDir() const;
	boost::filesystem::path embedDir() const;
	boost::filesystem::path subNotesDir() const;
	boost::filesystem::path textPathname() const;

	void warning(QString &&msg);
	void error(QString &&msg);
	void error();
	/// root of the current hierarchy
	Note* root();

	void emitAddNoteRecursively(std::shared_ptr<Note> &note);
	/// name_ is supposed to be set on the note already
	void addNote(std::shared_ptr<Note> note);
	std::shared_ptr<Note> removeFromParent();

	void deleteSelfRecursively();

	/// also creates embed dir, if it did not exist
	boost::filesystem::path generateEmbedFilename(const boost::filesystem::path &hint);
	void downloaded(const QString &originalUrl, const QByteArray &content, const QString &error);

	QString loadTxt();
	void saveTxt(const QString &txt);

	QString applyPatch(const QString &html);

	static
	QString decodeFromFilename(const boost::filesystem::path& fn);
	static
	boost::filesystem::path encodeToFilename(const QString& name);
	constexpr	static
	const char *textExt = u8".html";
	static constexpr
	const char delimChar = 1;
	constexpr	static
	const char *attachExt = u8".attach";
	constexpr	static
	const char *embedExt = u8".embed";  // files embedded in note html (images etc)
	constexpr	static
	const char *newFileExt = u8".new";
};

Q_DECLARE_METATYPE(std::weak_ptr<Note>)
Q_DECLARE_METATYPE(std::vector<std::weak_ptr<Note>>)

inline
QString Note::name() const
{
	return name_;
}

inline
Note *Note::child(size_t ndx)
{
	return subNotes_[ndx].get();
}
inline
const Note *Note::child(size_t ndx) const
{
	return subNotes_[ndx].get();
}




#endif // NOTE_H
