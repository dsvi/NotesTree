#ifndef DOWNLOADER_H
#define DOWNLOADER_H


class Downloader : public QObject
{
	Q_OBJECT
public:
	explicit Downloader(QObject *parent = 0);

signals:
	void finished(const QString &originalUrl, const QByteArray &a, const QString &error);
public slots:
	void get(const QString &url);

private:
	std::map<QString, QString> toOriginalUrl_;
	QNetworkAccessManager downloader_;
	void downloadFinished(QNetworkReply *reply);
};

#endif // DOWNLOADER_H
