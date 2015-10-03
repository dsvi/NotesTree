#include "Note.h"
#include "FileSystem.h"

const QChar Note::delimChar;

Note::Note()
{

}
QString Note::DecodeFromFilename(const QString &fn)
{
	QString ret;
	ret.reserve(fn.size());
	for (auto c = fn.begin(); c != fn.end(); ++c){
		if (*c == delimChar){
			QString num;
			for (int i = 2; --i >= 0;){
				if (++c == fn.end()){
					throw Exception("Filesystem", "Wrong num after delimiter %1 in file name: %2") % delimChar % fn;
				}
				num += *c;
			}
			bool ok;
			int code = num.toInt(&ok, 16);
			if (!ok){
				throw Exception("Filesystem", "Wrong num after delimiter %1 in file name: %2") % delimChar % fn;
				return ret;
			}
			ret += QChar(code);
		}
		else{
			ret += *c;
		}
	}
	return ret;
}

QString Note::EncodeToFilename(const QString &name)
{
	QString ret;
	ret.reserve(name.size());
	for (auto c:name){
		if (c == delimChar || c == L'/' || c == L'\\'){
			ret += delimChar;
			QString num;
			num.setNum(c.unicode(), 16);
			ASSERT(num.length() <= 2);
			if (num.length() < 2)
				ret += L'0';
			ret += num;
		}
		else{
			ret += c;
		}
	}
	return ret;
}

void Note::addFromSubnotesDir(const QDir &path, Note *parent)
{
	parent_ = parent;
	subNotes_.clear();
	name_ = DecodeFromFilename(path.dirName());
	subDir_ = path;
	path_ = GetParentDir(path) + "/";
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
		std::unique_ptr<Note> note(new Note());
		note->noteTextFile(fi.absoluteFilePath(), this);
		dirs.erase(note->subNotesDir()->dirName());
		subNotes_.push_back(std::move(note));
	}
	for (auto &dir : dirs){
		if (dir.second->fileName().endsWith(attachExt))
			continue;
		auto subDir = QDir{dir.second->absoluteFilePath()};
		std::unique_ptr<Note> subNote(new Note());
		subNote->addFromSubnotesDir(subDir, this);
		subNotes_.push_back(std::move(subNote));
	}
	sortSubnotes();
}

void Note::createHierarchyFromRoot(const QDir &path)
{
	try{
		if (!path.exists())
			throw Exception("Filesystem", "directory '%1' doesnt exist") % path.absolutePath();
		addFromSubnotesDir(path, nullptr);
	}
	catch(...){
		subNotes_.clear();
		throw;
	}
}


void Note::noteTextFile(const QFileInfo &fi, Note *parent)
{
	parent_ = parent;
	textPathname_ = fi;
	path_ = fi.absolutePath() + "/";
	auto name = textPathname_.fileName();
	ASSERT(name.endsWith(Note::fileExt()));
	name.truncate(name.length() - std::strlen(Note::fileExt()));
	auto subDir = QDir{textPathname_.dir().absolutePath() + "/" + name};
	if (subDir.exists())
		addFromSubnotesDir(subDir, parent);
	else
		name_ = DecodeFromFilename(name);
	auto attachDir = QDir{subDir.absolutePath().append(attachExt)};
	if (attachDir.exists())
		attachDir_ = attachDir;
}

void Note::move(const QString &newPath, const QString &newFileName)
{
	ASSERT(newPath.endsWith("/"));
	if (hasText()){
		QFileInfo newPathname(newPath + newFileName + fileExt());
		Rename( textPathname_.absoluteFilePath(), newPathname.absoluteFilePath() );
		textPathname_ = newPathname;
	}
	if (subDir_.path() != "."){
		QDir newSubDir(newPath + newFileName);
		Rename(subDir_.absolutePath(), newSubDir.absolutePath());
		subDir_ = newSubDir;
	}
	if (hasAttach()){
		QDir newAttachDir(newPath + newFileName + attachExt);
		Rename(attachDir_.absolutePath(), newAttachDir.absolutePath());
		attachDir_ = newAttachDir;
	}
}

void Note::sortSubnotes()
{
	std::sort(subNotes_.begin(), subNotes_.end(), [](const decltype(subNotes_[0]) &a, const decltype(subNotes_[0]) &b){
		return a->name() < b->name();
	});
}

void Note::adopt_(Note *n)
{
	Note *prevParent = n->parent_;
	n->parent_ = this;
	auto prevParentNdx = prevParent->findIndexOf(n);
	subNotes_.push_back(std::move(prevParent->subNotes_[prevParentNdx]));
	auto it = prevParent->subNotes_.begin() + prevParentNdx;
	prevParent->subNotes_.erase(it, it+1);
	subDir_ = QDir(path_ + EncodeToFilename(name_));
	QString subDirPath = subDir_.absolutePath() +"/";
	if (!subDir_.mkpath(subDirPath))
		throw Exception("Filesystem", "Can't create directory '%1'") % subDirPath;
	subNotes_.back()->move(subDirPath, EncodeToFilename(n->name_) );
	if (RemoveIfEmpty(prevParent->subDir_))
		prevParent->subDir_ = QDir();
}

bool Note::hasSubnotes() const
{
	return !subNotes_.empty();
}

bool Note::hasText() const
{
	return !textPathname_.filePath().isEmpty();
}

void Note::adopt(Note *n)
{
	adopt_(n);
	sortSubnotes();
}

void Note::adopt(std::vector<Note *> &&list)
{
	std::sort(list.begin(), list.end(), [](const Note *a, const Note *b){
		return a->hierarchyDepth() > b->hierarchyDepth();
	});
	for (auto n : list)
		adopt_(n);
	sortSubnotes();
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

bool Note::exist(const QString &name)
{
	for (auto &sub : subNotes_)
		if (sub->name_ == name)
			return true;
	return false;
}

QString Note::makePathName(const QString separator) const
{
	std::stack<const Note*> stack;
	for (const Note *n = parent(); n != nullptr; n = n->parent())
		stack.push(n);
	stack.pop();
	QString ret;
	while (!stack.empty()){
		ret += stack.top()->name();
		stack.pop();
		if (!stack.empty())
			ret += separator;
	}
	return ret;
}

int Note::hierarchyDepth() const
{
	int depth = 0;
	for (const Note *n = parent(); n != nullptr; n = n->parent())
		depth++;
	return depth;
}


void Note::name(const QString &name)
{
	if (name == name_)
		return;
	QString filename = EncodeToFilename(name);
	move(path_, filename);
	name_ = name;
	parent_->sortSubnotes();
}

