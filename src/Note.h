#ifndef NOTE_H
#define NOTE_H

class Note
{
public:
	Note();

	/// nullptr means this is root
	Note *parent();
	const Note *parent() const;
	size_t numChildren() const;
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

	QString name() const;
	void name(const QString &name);

	/// create hierarchy of notes and subnotes from the root folder
	/// this note becames root of the hierarchy
	/// exception safe
	void createHierarchyFromRoot(const QString &path);

	/// add the file as attached to the note
	void attach(const QFileInfo &fi);
	/// attache the file.
	bool  hasAttach() const;
	bool  hasSubnotes() const;
	bool  hasText() const;
	boost::filesystem::path attachDir() const;
	boost::filesystem::path subNotesDir() const;
	boost::filesystem::path textPathname() const;

	/// adds the note as child to this, removes it from prev parent.
	/// make sure the note with the same name doesn't exist here already, before adopting another one.
	/// or RecoverableException happens
	/// \sa exist
	void adopt(Note *n);
	/// same as above, but adds a lot at a time
	void adopt(std::vector<Note*> &&list);

	Note *createSubnote(const QString &name);
	void deleteRecursively(Note *child);

signals:
public slots:
private:
	Note     *parent_ = nullptr;
	QString		name_; // root has path to root here, instead of name

	std::vector<std::unique_ptr<Note>> subNotes_;

	boost::filesystem::path pathToNote() const;

	/// populate list of subnotes and the name from the @path dir
	void addFromSubnotesDir(const boost::filesystem::path &path);
	/// pathname should end on textExt
	void createFromNoteTextFile(const boost::filesystem::path &textPathname);
	void move(const boost::filesystem::path &newPath, const boost::filesystem::path &newFileName);
	void sortSubnotes();
	void adopt_(Note *n);
	void cleanUpFileSystem();
	void ensureSubDirExist();
	std::unique_ptr<boost::filesystem::fstream> openText();
	

	static
	QString decodeFromFilename(const boost::filesystem::path& fn);
	static
	boost::filesystem::path encodeToFilename(const QString& name);
	constexpr	static
	const char *textExt = u8".html";
	static constexpr
	const char delimChar = 1;
	constexpr	static
	const char *attachExt = delimChar + u8"attach";
};

inline
Note *Note::parent()
{
	return parent_;
}
inline
const Note *Note::parent() const
{
	return parent_;
}

inline
size_t Note::numChildren() const
{
	return subNotes_.size();
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

inline
QString Note::name() const
{
	return name_;
}



#endif // NOTE_H
