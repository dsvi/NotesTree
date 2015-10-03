#include "FileSystem.h"

void Rename(const QString &from, const QString &to)
{
	QDir dir;
	if (!dir.rename(from, to))
		throw Exception("Filesystem", "Can not rename\n'%1'\nto\n'%2'.") % from % to;
}

QString GetParentDir(const QDir &d)
{
	QDir p = d;
	if ( !p.cdUp() )
		throw Exception("Filesystem", "'%1'\nhas no parent directory.") % d.absolutePath();
	return p.absolutePath() + "/";
}

bool RemoveIfEmpty(QDir &d){
	if (d.entryList(QDir::AllEntries|QDir::NoDotAndDotDot).empty())
		return d.removeRecursively();
	return false;
}
