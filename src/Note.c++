#include "Note.h"

using namespace std;
using namespace boost::filesystem;

const char Note::delimChar;

Note::Note()
{
}

Note::~Note()
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
		auto note = make_shared<Note>();
		note->createFromNoteTextFile(fi.path());
		dirs.erase(encodeToFilename(note->name_).c_str());
		dirs.erase(toQS(encodeToFilename(note->name_ + attachExt)));
		dirs.erase(toQS(encodeToFilename(note->name_ + embedExt)));
		addNote(note);
	}
	for (auto dir = dirs.begin(); dir != dirs.end(); ){
		const boost::filesystem::path &dirPath = dir->second.path();
		if (
			toQS(dirPath.filename()).endsWith(attachExt) ||
			toQS(dirPath.filename()).endsWith(embedExt)
		){
			++dir;
			continue;
		}
		auto subNote = make_shared<Note>();
		subNote->addFromSubnotesDir(dirPath);
		dirs.erase(toQS(encodeToFilename(subNote->name_ + attachExt)));
		dirs.erase(toQS(encodeToFilename(subNote->name_ + embedExt)));
		dir = dirs.erase(dir);
		addNote(subNote);
	}
	for (auto &dir : dirs){ // just in case we happened to have a note name ending on attachExt etc
		const boost::filesystem::path &dirPath = dir.second.path();
		auto subNote = make_shared<Note>();
		subNote->addFromSubnotesDir(dirPath);
		addNote(subNote);
	}
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
		ASSERT(parent_ == nullptr);
		emit clear();
		subNotes_.clear();
		name_.clear();
		boost::filesystem::path path(p.toUtf8());
		if (!exists(path))
			throw RecoverableException(QCoreApplication::translate("Filesystem", "directory '%1' doesnt exist").arg(p));
		name_ = p;
		addFromSubnotesDir(path);
	}
	catch(...){
		subNotes_.clear();
		name_.clear();
		app->reportError(std::current_exception());
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

void Note::adopt_(const std::shared_ptr<Note> &n)
{
	if (n.get() == this)
		throw Exception(tr("Can't add as subnote to self. (%1)").arg(name_));
	ensureSubDirExist();
	n->move(subNotesDir(), encodeToFilename(n->name_));
	addNote(n->removeFromParent());
}

void Note::cleanUpFileSystem()
{
	if ( parent_ == nullptr ) //nothing to clean up at root
		return;
	auto subDir = subNotesDir();
	auto textFile = textPathname();
	auto attach = attachDir();
	auto embed = embedDir();
	if (exists(subDir) && boost::filesystem::is_empty(subDir) && exists(textFile))
		remove(subDir);
	if (exists(textFile) && boost::filesystem::is_empty(textFile) && exists(subDir))
		remove(textFile);
	if (exists(attach) && boost::filesystem::is_empty(attach))
		remove(attach);
	if (exists(embed) && boost::filesystem::is_empty(embed))
		remove(embed);
}

void Note::ensureSubDirExist()
{
	auto subDir = subNotesDir();
	if (exists(subDir))
		return;
	create_directory(subDir);
}

void Note::error(QString &&msg)
{
	try{
		throw_with_nested(Exception(std::move(msg)));
	}
	catch(...){
		error();
	}
}

void Note::error()
{
	auto e = std::current_exception();
	if (!isRecoverable(e)){
		Note *r = root();
		r->subNotes_.clear(); // zombify!
		r->name_.clear();
	}
	app->error(e);
}

Note* Note::root()
{
	Note *r = this;
	for (Note *n = parent_; n != nullptr; n = n->parent_)
		r = n;
	return r;
}

void Note::emitAddNoteRecursively(std::shared_ptr<Note> &note)
{
	emit noteAdded(note);
	for (auto &cn : note->subNotes_)
		note->emitAddNoteRecursively(cn);
}

void Note::addNote(std::shared_ptr<Note> note)
{
	note->parent_ = this;
	subNotes_.push_back(note);
	emitAddNoteRecursively(note);
}

std::shared_ptr<Note> Note::removeFromParent()
{
	auto myNdx = parent_->findIndexOf(this);
	auto ret = parent_->subNotes_[myNdx];
	auto it = parent_->subNotes_.begin() + myNdx;
	parent_->subNotes_.erase(it, it+1);
	parent_->cleanUpFileSystem();
	emit noteRemoved();
	return ret;
}

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

void Note::adopt(const std::vector<std::weak_ptr<Note> > &list)
{
	try{
		std::set<QString> uniquenessCheck;
		for (auto  np : list){
			auto n = np.lock();
			if (!n)
				continue;
			QString name = n->name_;
			if (exist(name) || uniquenessCheck.find(name) != uniquenessCheck.end()){
				auto e = RecoverableException(tr(
					"Note names have to be unique in the subnotes list. '%1' is not.\n").arg(name));
				if (n->hierarchyDepth() > 1)
					e.append(tr("It came from '%1'.\n").arg(n->makePathName()));
				throw e;
			}
			uniquenessCheck.insert(name);
		}
		for (auto n : list)
			if (!n.expired())
				adopt_(n.lock());
	}
	catch(...){
		error();
	}
}

void Note::createSubnote(const QString &name)
{
	if (root()->isZombie())
		return;
	try{
		if (exist(name))
			throw RecoverableException(
				tr("Note '%1' already exist here.\n").arg(name));
		ensureSubDirExist();
		auto subNote = make_shared<Note>();
		subNote->parent_ = this;
		subNote->name_ = name;
		auto txtPath = subNote->textPathname();
		if (exists(txtPath))
			throw RecoverableException(
				tr("Filename '%1' already exist.\n").arg(QString::fromStdWString(txtPath.wstring())));
		boost::filesystem::fstream out(
			subNote->textPathname(), ios_base::out | ios_base::binary);
		addNote(subNote);
	}
	catch(...){
		error();
	}
}

void Note::deleteRecursively(const std::vector<std::weak_ptr<Note> > &list)
{
	try{
		vector<Note*> notes;
		for (auto &n : list ){
			if (!n.expired())
				notes.push_back(n.lock().get());
		}
		std::sort(notes.begin(), notes.end(),[](const auto a, const auto b){
			return a->hierarchyDepth() > b->hierarchyDepth();
		});
		for (auto n : notes)
			n->deleteSelfRecursively();
	}
	catch(...){
		error();
	}
}

void Note::deleteSelfRecursively()
{
	ASSERT(parent_ != nullptr); //not root
	try{
		remove_all(subNotesDir());
		remove_all(attachDir());
		remove(textPathname());
		removeFromParent();
	}
	catch(...){
		throw Exception(tr("Can't delete note '%1':").arg(name_));
	}
}

void Note::save(const QString &txt)
{
	boost::filesystem::path outFilename;
	try{
		outFilename = textPathname();
		outFilename += newFileExt;
		using io = std::ios_base;
		boost::filesystem::fstream out(outFilename, io::out | io::trunc | io::binary);
		out.exceptions(io::failbit | io::badbit);
		auto utf8txt = txt.toUtf8();
		out.write(utf8txt.data(), utf8txt.size());
		out.close();
		rename(outFilename, textPathname());
	}
	catch(...){
		error(tr("Can't save note '%1' to file '%2'\n").arg(name_).arg(toQS(outFilename)));
	}
}

void Note::load()
{
	try{
		auto inFilename = textPathname();
		if (!exists(inFilename)){
			emit noteTextRdy(QString());
			return;
		}
		using io = std::ios_base;
		boost::filesystem::fstream in(inFilename, io::in | io::binary);
		in.exceptions(io::failbit | io::badbit);
		auto inSize = file_size(inFilename);
		if (inSize == static_cast<uintmax_t>(-1))
			throw RecoverableException(
				tr("Can't determine file size. Is it really a file?").arg(toQS(inFilename)));
		QByteArray ba;
		ba.resize(inSize);
		in.read(ba.data(), ba.size());
		in.close();
		emit noteTextRdy(QString(ba));
	}
	catch(...){
		error(tr("Can't load file '%1'\n").arg(toQS(textPathname())));
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
	ASSERT(parent_ != nullptr); // root has no attach
	return pathToNote() / encodeToFilename(name_+attachExt);
}

path Note::embedDir() const
{
	ASSERT(parent_ != nullptr); // root has no embed
	return pathToNote() / encodeToFilename(name_+embedExt);
}

boost::filesystem::path Note::subNotesDir() const
{
	if (parent_ == nullptr)
		return pathToNote();
	return pathToNote() / encodeToFilename(name_);
}

boost::filesystem::path Note::textPathname() const
{
	ASSERT(parent_ != nullptr); // root has no text
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
	for (const Note *n = parent_; n != nullptr; n = n->parent_)
		stack.push(n);
	stack.pop();
	QString ret;
	while (!stack.empty()){
		ret += stack.top()->name_;
		stack.pop();
		if (!stack.empty())
			ret += separator;
	}
	return ret;
}

int Note::hierarchyDepth() const
{
	int depth = 0;
	for (const Note *n = parent_; n != nullptr; n = n->parent_)
		depth++;
	return depth;
}


void Note::changeName(const QString &name)
{
	ASSERT(parent_ != nullptr);
	try{
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
		emit nameChanged(name);
	}
	catch(...){
		error();
	}
}

