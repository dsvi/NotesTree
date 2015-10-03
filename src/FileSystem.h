#ifndef FILESYSTEM_H
#define FILESYSTEM_H

/// returns absolute path with "/" at end. or throws, if d is root
QString GetParentDir(const QDir &d);
/// rename file or dir
void Rename(const QString &from, const QString &to);
/// returns true if was empty and removed
bool RemoveIfEmpty(QDir &d);

#endif // FILESYSTEM_H
