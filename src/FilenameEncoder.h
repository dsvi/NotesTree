#ifndef FILENAMEENCODER_H
#define FILENAMEENCODER_H

// encodes or decodes symbols like "/" etc

QString DecodeFromFilename(const QString &fn);
QString EncodeToFilename(const QString &name);

constexpr
QString::value_type delimChar = L'|';

#endif // FILENAMEENCODER_H
