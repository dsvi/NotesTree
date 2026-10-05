#include "Downloader.h"

Downloader::Downloader(QObject *parent) : QObject(parent), downloader_(this)
{
	downloader_.setAutoDeleteReplies(true);
}

void Downloader::get(const QString &originalUrl)
{
	auto *reply = downloader_.get(QNetworkRequest(QUrl(originalUrl)));
	connect(reply, &QNetworkReply::finished, this, [this, reply, originalUrl]{
		if (reply->error() != QNetworkReply::NoError){
			QString msg = tr("Original url: %1\n").arg(originalUrl);
			if (reply->url() != QUrl(originalUrl))
				msg += tr("Redirect: %1\n").arg(reply->url().toString());
			msg += tr("Problem: %1\n").arg(reply->errorString());
			emit finished(originalUrl, QByteArray(), msg);
			return;
		}
		emit finished(originalUrl, reply->readAll(), QString());
	});
}
