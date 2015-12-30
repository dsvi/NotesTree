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
		auto subNote = make_shared<Note>();
		subNote->createFromNoteTextFile(fi.path());
		addNote(subNote);
		class path subDir = subNote->subNotesDir();
		if (exists(subDir))
			subNote->addFromSubnotesDir(subDir);
		dirs.erase(toQS(encodeToFilename(subNote->name_)));
		dirs.erase(toQS(encodeToFilename(subNote->name_ + attachExt)));
		dirs.erase(toQS(encodeToFilename(subNote->name_ + embedExt)));
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
		subNote->name_ = decodeFromFilename(dirPath.filename());
		addNote(subNote);
		subNote->addFromSubnotesDir(dirPath);
		dirs.erase(toQS(encodeToFilename(subNote->name_ + attachExt)));
		dirs.erase(toQS(encodeToFilename(subNote->name_ + embedExt)));
		dir = dirs.erase(dir);
	}
	for (auto &dir : dirs){ // just in case we happened to have a note name ending on attachExt etc
		const boost::filesystem::path &dirPath = dir.second.path();
		auto subNote = make_shared<Note>();
		subNote->name_ = decodeFromFilename(dirPath.filename());
		addNote(subNote);
		subNote->addFromSubnotesDir(dirPath);
	}
}

void Note::createFromNoteTextFile(const path &fi)
{
	path name = fi.filename();
	ASSERT(name.extension() == Note::textExt);
	name = name.stem();
	name_ = decodeFromFilename(name);
}

void Note::createHierarchyFromRoot(const path &p)
{
	try{
		ASSERT(parent_ == nullptr);
		emit clear();
		subNotes_.clear();
		name_ = toQS(p);
		if (!exists(p))
			throw RecoverableException(QCoreApplication::translate("Filesystem", "directory '%1' doesnt exist").arg(name_));
		addFromSubnotesDir(p);
	}
	catch(...){
		subNotes_.clear();
		name_.clear();
		warning("Can't load notes.");
	}
}

void Note::move(const boost::filesystem::path &newPath, const boost::filesystem::path &newFileName)
{
	ASSERT(is_directory(newPath));
	auto ren = [&](const path &oldPath, const char *ext){
		if (exists(oldPath)){
			path newPathname = newPath / newFileName;
			if (ext)
				newPathname += ext;
			rename(oldPath, newPathname);
		}
	};
	ren(textPathname(), textExt);
	ren(subNotesDir(),  nullptr);
	ren(attachDir(),    attachExt);
	ren(embedDir(),     embedExt);
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

void Note::warning(QString &&msg)
{
	try{
		throw_with_nested(RecoverableException(std::move(msg)));
	}
	catch(...){
		app->reportError(std::current_exception());
	}
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
		cleanUpFileSystem();
	}
	catch(...){
		warning(tr("Cant create note '%1'").arg(name));
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
		parent_->cleanUpFileSystem();
	}
	catch(...){
		throw Exception(tr("Can't delete note '%1':").arg(name_));
	}
}

static const char * UrlRegexps[3] = {
	"<(?:img|source)[^>]+src\\s*=\\s*\"([^\"]+)\"",
	"<(?:img|source)[^>]+src\\s*=\\s*'([^']+)'",
	"<(?:img|source)[^>]+src\\s*=\\s*[^'\"](\\S+)"
};

static
QString changeUrls(QString html, std::function<QString(const QString &)> mapper){
	for (auto r : UrlRegexps){
		QString newHtml;
		int from = 0;
		int to;
		QRegExp reg(r);
		int pos = 0;
		while (true){
			pos = reg.indexIn(html, pos);
			if (pos < 0)
				break;
//			qDebug()<< reg.cap(0);
//			qDebug()<< reg.cap(1);
			pos += reg.cap(0).size();
			auto cap = reg.cap(1);
			QString newUrl = mapper(cap);
			if (newUrl == cap)
				continue;
			to = reg.pos(1);
			newHtml += html.midRef(from, to);
			newHtml += newUrl;
			from = to + cap.size();
		}
		if (from == 0)
			continue;
		newHtml += html.midRef(from, html.size());
		html = move(newHtml);
	}
	return html;
}

static
set<QString> grabUrlsOfEmbeddedFiles(const QString &html)
{
	set<QString> srcs;
	changeUrls(html, [&](const QString &from)->QString {
		srcs.insert(from);
		return from;
	});
	return srcs;
}

path Note::generateEmbedFilename(const boost::filesystem::path &hint)
{
	auto emDir = embedDir();
	if (!exists(emDir))
		create_directory(emDir);
	auto simpleCase = emDir/hint.filename();
	if (simpleCase.string().size() < 100 && !exists(simpleCase))
		return simpleCase;
	path ext = hint.extension();
	path pref = hint.stem();
	ui32 id = 0;
	path retFilename;
	do{
		path fn = pref;
		fn += path(to_string(id));
		fn += ext;
		retFilename = emDir / fn;
		id++;
	} while(exists(retFilename));
	return retFilename;
}

static
QString urlEnc(const QString &s){
	return QString(QUrl::toPercentEncoding(s, "./"));
}

void Note::downloaded(const QString &originalUrl, const QByteArray &content, const QString &error)
{
	try{
		urlsInDownload_.erase(originalUrl);
		if (!error.isNull()){
			throw RecoverableException(tr(
				"Can't download embedded file for '%1'\n"
				"url: %2\n"
				"problem: %3").arg(name_).arg(originalUrl).arg(error));
		}
		QString embedUrl;
		{
			path embed = generateEmbedFilename(toPath(QUrl::fromPercentEncoding(originalUrl.toUtf8())));
			using io = std::ios_base;
			boost::filesystem::fstream out(embed, io::out | io::trunc | io::binary);
			out.exceptions(io::failbit | io::badbit);
			out.write(content.data(), content.size());
			out.close();
			embedUrl = urlEnc("./" + toQS(embed.parent_path().filename() / embed.filename()));
		}
		urlsPatch_[originalUrl] = embedUrl;
		saveTxt(applyPatch(loadTxt()));
	}
	catch(...){
		warning(tr("Problems while getting embedded content for '%1'\n").arg(name_));
	}
}

QString Note::applyPatch(const QString &html)
{
	return changeUrls(html, [&](const QString& from)->QString{
		auto i = urlsPatch_.find(from);
		if (i == urlsPatch_.end())
			return from;
		return i->second;
	});
}

void Note::saveTxt(const QString &txt)
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
		warning(tr("Can't save note '%1' to file '%2'\n").arg(name_).arg(toQS(outFilename)));
	}
}

QString Note::loadTxt()
{
	try{
		auto inFilename = textPathname();
		if (!exists(inFilename))
			return QString();

		using io = std::ios_base;
		boost::filesystem::fstream in(inFilename, io::in | io::binary);
		in.exceptions(io::failbit | io::badbit);
		auto inSize = file_size(inFilename);
		if (inSize == static_cast<uintmax_t>(-1))
			throw RecoverableException(
				tr("Can't determine file size. Is it really a file?"));
		QByteArray ba;
		ba.resize(inSize);
		in.read(ba.data(), ba.size());
		in.close();
		return QString::fromUtf8(ba);
	}
	catch(...){
		warning(tr("Can't load file '%1'\n").arg(toQS(textPathname())));
	}
	return QString();
}

void Note::startEditing()
{
	stopEditing();
	QString html = loadTxt();
	emit noteTextRdy(html, toQS(pathToNote()));
}

void Note::save(QString html)
{
	try{
		html = applyPatch(html);
		set<path> validEmbeds;
		auto urls = grabUrlsOfEmbeddedFiles(html);
		for (auto u : urls){
			if (u.startsWith(".")){
				path em = toPath(QUrl::fromPercentEncoding(u.toUtf8()));
				validEmbeds.insert(em.filename());
				continue;
			}
			QString filePrefix = "file://";
			if (u.startsWith(filePrefix)){
				QString url = u;
				url.remove(0, filePrefix.size());
				url = QUrl::fromPercentEncoding(url.toUtf8());
				auto srcPath = toPath(url);
				path dstPath = generateEmbedFilename(srcPath);
				boost::system::error_code err;
				create_hard_link(srcPath, dstPath, err);
				if (err)
					copy_file(srcPath,dstPath);
				validEmbeds.insert(dstPath.filename());
				auto dstUrl = urlEnc("./" + toQS(dstPath.parent_path().filename() / dstPath.filename()));
				urlsPatch_[u] = dstUrl;
				continue;
			}
			if (urlsInDownload_.find(u) != urlsInDownload_.end())
				continue;
			auto dlr = app->downloader();
			connect(dlr, &Downloader::finished, this, &Note::downloaded, Qt::UniqueConnection);
			dlr->get(u);
			urlsInDownload_.insert(u);
		}
		// just in case we have updated the patch by copeing local file
		html = applyPatch(html);
		saveTxt(html);
		if (exists(embedDir())){
			for (auto&& ef : directory_iterator(embedDir())){ // remove unused embeds
				if (validEmbeds.find(ef.path().filename()) == validEmbeds.end())
					remove(ef.path());
			}
		}
		cleanUpFileSystem();
	}
	catch(...){
		warning(tr("Can't save note '%1'.").arg(name_));
	}
}

void Note::stopEditing()
{
	urlsPatch_.clear();
}


path Note::pathToNote() const
{
	if (parent_)
		return parent_->subNotesDir();
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
		if (exists(embedDir())){
			QString html = loadTxt();
			QString oldUrlPrefix = urlEnc("./" + toQS(encodeToFilename(name_ + embedExt)));
			auto oldPrefLen = oldUrlPrefix.size();
			QString newUrlPrefix = urlEnc("./" + toQS(encodeToFilename(name  + embedExt)));
			html = changeUrls(html, [&](const QString& from)->QString{
				if (!from.startsWith(oldUrlPrefix))
					return from;
				QString ret = from;
				ret.remove(0, oldPrefLen).prepend(newUrlPrefix);
				return ret;
			});
			saveTxt(html);
		}
		move(pathToNote(), encodeToFilename(name));
		name_ = name;
		emit nameChanged(name);
	}
	catch(...){
		error();
	}
}

