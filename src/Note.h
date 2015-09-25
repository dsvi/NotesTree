#ifndef NOTE_H
#define NOTE_H


class Note
{
public:
  Note();
	Note(const QString &name);

	Note *parent();
	const Note *parent() const;
	size_t numChildren() const;
	Note *child(size_t ndx);
	const Note *child(size_t ndx) const;
	size_t findIndexOf(const Note *) const;

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

signals:
public slots:
private:
	Note     *parent_;
	QString		name_;
	QFileInfo textPathname_;
	QDir      subDir_;
	QDir      attachDir_;
	std::vector<std::unique_ptr<Note>> subNotes_;

	/// populate list of subnotes and the name from the @path dir
	void addFromSubnotesDir(const QDir &path, Note *parent);
	/// pathname should end on fileExt()
	void noteTextFile(const QFileInfo &textPathname, Note *parent);

	constexpr
	static
	const char* fileExt(){
		return ".html";
	}

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
void Note::name(const QString &name)
{
	name_ = name;
}
inline
void Note::createHierarchyFromRoot(const QDir &path)
{
	addFromSubnotesDir(path, nullptr);
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
