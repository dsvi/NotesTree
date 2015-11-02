#include "Note.h"

using namespace std;
using namespace boost::filesystem;

const char Note::delimChar;

Note::Note()
{
}

QString Note::decodeFromFilename(const boost::filesystem::path &filename)
{
	QString ret;
	QString fn(filename.c_str());
	ret.reserve(fn.size());
	for (auto c = fn.begin(); c != fn.end(); ++c){
		if (*c == delimChar){
			QString num;
			for (int i = 2; --i >= 0;){
				if (++c == fn.end()){
					throw Exception(QCoreApplication::translate(
														"Filesystem", "Wrong num after delimiter %1 in file name: %2").arg(delimChar).arg(fn));
				}
				num += *c;
			}
			bool ok;
			int code = num.toInt(&ok, 16);
			if (!ok){
				throw Exception(QCoreApplication::translate(
													"Filesystem", "Wrong num after delimiter %1 in file name: %2").arg(delimChar).arg(fn));
			}
			ret += QChar(code);
		}
		else{
			ret += *c;
		}
	}
	return ret;
}

boost::filesystem::path Note::encodeToFilename(const QString &name)
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
	return path(ret.toUtf8());
}

void Note::addFromSubnotesDir(const boost::filesystem::path &path)
{
	if (name_.isNull())
		name_ = decodeFromFilename(path.filename());
	std::unordered_map<QString, directory_entry> dirs;
	for (auto&& x : directory_iterator(path))
		if (x.status().type() == file_type::directory_file)
			dirs.insert({x.path().filename().c_str(), x});
	for (directory_entry& fi : directory_iterator(path)){
		if (fi.status().type() == file_type::directory_file)
			continue;
		QString name = fi.path().filename().c_str();
		if (!name.endsWith(Note::textExt))
			continue;
		auto note = make_unique<Note>();
		note->parent_ = this;
		note->createFromNoteTextFile(fi.path());
		dirs.erase(encodeToFilename(note->name()).c_str());
		subNotes_.push_back(std::move(note));
	}
	for (auto &dir : dirs){
		const boost::filesystem::path &dirPath = dir.second.path();
		if (QString(dirPath.filename().c_str()).endsWith(attachExt))
			continue;
		auto subNote = make_unique<Note>();
		subNote->parent_ = this;
		subNote->addFromSubnotesDir(dirPath);
		subNotes_.push_back(std::move(subNote));
	}
	sortSubnotes();
}

void Note::createFromNoteTextFile(const path &fi)
{
	path name = fi.filename();
	ASSERT(name.extension() == Note::textExt);
	name = name.stem();
	name_ = decodeFromFilename(name);
	path subDir = subNotesDir();
	if (exists(subDir))
		addFromSubnotesDir(subDir);
}

void Note::createHierarchyFromRoot(const QString &p)
{
	try{
		subNotes_.clear();
		boost::filesystem::path path(p.toUtf8());
		if (!exists(path))
			throw RecoverableException(QCoreApplication::translate("Filesystem", "directory '%1' doesnt exist").arg(p));
		name_ = p;
		addFromSubnotesDir(path);
	}
	catch(...){
		subNotes_.clear();
		throw;
	}
}

void Note::move(const boost::filesystem::path &newPath, const boost::filesystem::path &newFileName)
{
	ASSERT(is_directory(newPath));
	auto textFile = textPathname();
	if (exists(textFile)){
		path newTexFile = newPath / newFileName;
		newTexFile += textExt;
		rename(textFile, newTexFile);
	}
	auto subDir = subNotesDir();
	if (exists(subDir)){
		path newSubDir = newPath / newFileName;
		rename(subDir, newSubDir);
	}
	auto attach = attachDir();
	if (exists(attach)){
		path newAttachDir = newPath / newFileName;
		newAttachDir += attachExt;
		rename(attach, newAttachDir);
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
	if (n == this)
		throw Exception(QCoreApplication::translate("Note moving", "Can't add as subnote to self. (%1)").arg(name_));
	ensureSubDirExist();
	n->move(subNotesDir(), encodeToFilename(n->name_));
	Note *prevParent = n->parent_;
	auto prevParentNdx = prevParent->findIndexOf(n);
	subNotes_.push_back(std::move(prevParent->subNotes_[prevParentNdx]));
	n->parent_ = this;
	auto it = prevParent->subNotes_.begin() + prevParentNdx;
	prevParent->subNotes_.erase(it, it+1);
	prevParent->cleanUpFileSystem();
}

void Note::cleanUpFileSystem()
{
	if ( parent_ == nullptr ) //nothing to clean up at root
		return;
	auto subDir = subNotesDir();
	auto textFile = textPathname();
	auto attach = attachDir();
	if (exists(subDir) && boost::filesystem::is_empty(subDir) && exists(textFile))
		remove(subDir);
	if (exists(textFile) && boost::filesystem::is_empty(textFile) && exists(subDir))
		remove(textFile);
	if (exists(attach) && boost::filesystem::is_empty(attach))
		remove(attach);
}

void Note::ensureSubDirExist()
{
	auto subDir = subNotesDir();
	if (exists(subDir))
		return;
	create_directory(subDir);
}

//std::unique_ptr<boost::filesystem::fstream>  Note::openText()
//{
//	return std::make_unique<boost::filesystem::fstream>(
//		textPathname(), ios_base::in | ios_base::trunc | ios_base::binary);
//}

bool Note::hasSubnotes() const
{
	return !subNotes_.empty();
}

bool Note::hasText() const
{
	return exists(textPathname());
}

bool Note::hasAttach() const
{
	return exists(attachDir());
}

void Note::adopt(Note *n)
{
	QString name = n->name();
	if (exist(name)){
		throw RecoverableException(
			QCoreApplication::translate(
			"notes moving",
			"'%1' already exist there").arg(name));
	}
	adopt_(n);
	sortSubnotes();
}

void Note::adopt(std::vector<Note *> &&list)
{
	std::set<QString> uniquenessCheck;
	for (const Note *n : list){
		QString name = n->name();
		if (exist(name) || uniquenessCheck.find(name) != uniquenessCheck.end()){
			auto e = RecoverableException(
				QCoreApplication::translate(
				"notes moving",
				"Note names have to be unique in the subnotes list. '%1' is not.\n").arg(name));
			if (n->hierarchyDepth() > 1)
				e.append(
					QCoreApplication::translate(
					"notes moving",
					"It came from '%1'.\n").arg(n->makePathName()));
			throw e;
		}
		uniquenessCheck.insert(name);
	}
	for (auto n : list)
		adopt_(n);
	sortSubnotes();
}

Note* Note::createSubnote(const QString &name)
{
	ensureSubDirExist();
	auto subNote = make_unique<Note>();
	subNote->parent_ = this;
	subNote->name_ = name;
	boost::filesystem::fstream(
		subNote->textPathname(), ios_base::out | ios_base::binary);
	Note *n = subNote.get();
	subNotes_.push_back(std::move(subNote));
	sortSubnotes();
	return n;
}

void Note::deleteRecursively(Note *child)
{
	auto name = child->name();
	try{
		auto subDir = child->subNotesDir();
		auto textFile = child->textPathname();
		auto attach = child->attachDir();
		remove_all(subDir);
		remove(textFile);
		remove_all(attach);
		auto ndx = findIndexOf(child);
		subNotes_.erase(subNotes_.begin() + ndx);
	}
	catch(...){
		throw_with_nested(Exception(QCoreApplication::translate(
			"note deletion error", "Can't delete note '%1':").arg(name)));
	}
}

path Note::pathToNote() const
{
	if (parent_){
		if (parent_->parent_)
			return parent_->pathToNote() / encodeToFilename(parent_->name_);
		else
			return parent_->pathToNote(); // since name_ field in root has different meaning
	}
	else
		return path(name_.toUtf8());
}

boost::filesystem::path Note::attachDir() const
{
	return pathToNote() / encodeToFilename(name_+attachExt);
}

boost::filesystem::path Note::subNotesDir() const
{
	return pathToNote() / encodeToFilename(name_);
}

boost::filesystem::path Note::textPathname() const
{
	return pathToNote() / encodeToFilename(name_+textExt);
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
	if (parent_->exist(name)){
		throw RecoverableException(
			QCoreApplication::translate(
			"notes renaming",
			"'%1' already exist there.\n").arg(name));
	}
	move(pathToNote(), encodeToFilename(name));
	name_ = name;
	parent_->sortSubnotes();
}

