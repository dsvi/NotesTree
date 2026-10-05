#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>

class Downloader : public QObject
{
	Q_OBJECT
public:
	explicit Downloader(QObject *parent = nullptr);

signals:
	void finished(const QString &originalUrl, const QByteArray &a, const QString &error);
public slots:
	void get(const QString &url);

private:
	QNetworkAccessManager downloader_;
};

#endif // DOWNLOADER_H
