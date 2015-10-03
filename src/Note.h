#ifndef NOTE_H
#define NOTE_H

class Note
{
public:
	Note();
	Note(const QString &name);

	/// nullptr means this is root
	Note *parent();
	const Note *parent() const;
	size_t numChildren() const;
	Note *child(size_t ndx);
	const Note *child(size_t ndx) const;
	size_t findIndexOf(const Note *) const;

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
	void createHierarchyFromRoot(const QDir &path);

	/// add the file as attached to the note
	void attach(const QFileInfo &fi);
	/// attache the file.
	bool  hasAttach() const;
	bool  hasSubnotes() const;
	bool  hasText() const;
	const QDir* attachDir() const;
	const QDir* subNotesDir() const;
	const QFileInfo* textPathname() const;

	/// adds the note as child to this, removes it from prev parent.
	/// make sure the note with the same name doesn't exist here already, before adopting another one.
	/// \sa exist
	void adopt(Note *n);
	/// this one takes in account positions in hierarchy. it adops chlidren frst, than parents, to avoid file renaming troubles.
	/// so if you'd like to adopt a bunch of notes at a time, you \b should use this.
	/// the list shall not contain notes with the same names or names of subnotes to this one.
	void adopt(std::vector<Note*> &&list);

signals:
public slots:
private:
	Note     *parent_ = nullptr;
	QString		name_;
	QString   path_;
	QFileInfo textPathname_;
	QDir      subDir_;
	QDir      attachDir_;
	std::vector<std::unique_ptr<Note>> subNotes_;

	/// populate list of subnotes and the name from the @path dir
	void addFromSubnotesDir(const QDir &path, Note *parent);
	/// pathname should end on fileExt()
	void noteTextFile(const QFileInfo &textPathname, Note *parent);
	void move(const QString &newPath, const QString &newFileName);
	void sortSubnotes();
	void adopt_(Note *n);

	static
	QString DecodeFromFilename(const QString &fn);
	static
	QString EncodeToFilename(const QString &name);
	constexpr	static
	const char* fileExt(){
		return ".html";
	}
	static constexpr
	const QChar delimChar = L'|';
	constexpr	static
	const char *attachExt = u8"|attach";
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
inline
const QDir* Note::attachDir() const
{
	return &attachDir_;
}
inline
const QDir *Note::subNotesDir() const
{
	return &subDir_;
}
inline
const QFileInfo* Note::textPathname() const
{
	return &textPathname_;
}


#endif // NOTE_H
