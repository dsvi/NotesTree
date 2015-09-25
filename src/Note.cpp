#include "Note.h"
#include "FilenameEncoder.h"

Note::Note()
{

}

void Note::addFromSubnotesDir(const QDir &path, Note *parent)
{
	parent_ = parent;
	subNotes_.clear();
	name_ = DecodeFromFilename(path.dirName());
	subDir_ = path;
	auto filesList = path.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot);
	std::unordered_map<QString, QFileInfo*> dirs;
	for (auto &fi : filesList){
		if (fi.isDir())
			dirs.insert({fi.fileName(), &fi});
	}
	for (auto &fi : filesList){
		if (fi.isDir())
			continue;
		auto name = fi.fileName();
		if (!name.endsWith(Note::fileExt()) )
			continue;
		auto note = std::make_unique<Note>();
		note->noteTextFile(fi.absoluteFilePath(), this);
		if (note->hasAttach())
			dirs.erase(note->attachDir()->dirName());
		dirs.erase(note->subNotesDir()->dirName());
		subNotes_.push_back(std::move(note));
	}
	for (auto &dir : dirs){
		auto subNote = std::make_unique<Note>();
		subNote->addFromSubnotesDir(QDir{dir.second->absoluteFilePath()}, this);
		subNotes_.push_back(std::move(subNote));
	}
}

void Note::noteTextFile(const QFileInfo &fi, Note *parent)
{
	parent_ = parent;
	textPathname_ = fi;
	auto name = textPathname_.fileName();
	ASSERT(name.endsWith(Note::fileExt()));
	name.truncate(name.length() - std::strlen(Note::fileExt()));
	auto subDir = QDir{textPathname_.dir().absolutePath() + "/" + name};
	if (subDir.exists())
		addFromSubnotesDir(subDir, parent);
	else
		name_ = DecodeFromFilename(name);
	auto attachDir = QDir{subDir.absolutePath().append(delimChar) + "attach"};
	if (attachDir.exists())
		attachDir_ = attachDir;
}

bool Note::hasSubnotes() const
{
	return !subNotes_.empty();
}

bool Note::hasText() const
{
	return !textPathname_.filePath().isEmpty();
}

bool Note::hasAttach() const
{
	auto p = attachDir_.path();
	return  p != ".";
}

size_t Note::findIndexOf(const Note *n) const
{
	for (size_t i = 0; i < subNotes_.size(); ++i){
		if (subNotes_[i].get() == n)
			return i;
	}
	ASSERT(false);
	return subNotes_.size(); // make caller crash
}
