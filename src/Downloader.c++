#include "Downloader.h"

Downloader::Downloader(QObject *parent) : QObject(parent), downloader_(this)
{
	downloader_.setAutoDeleteReplies(true);
	connect(&downloader_, &QNetworkAccessManager::finished, this, [=, this](QNetworkReply *reply){
		downloadFinished(reply);
	});
}

void Downloader::get(const QString &originalUrl)
{
	if (toOriginalUrl_.contains(originalUrl))
		return;
	auto url = QUrl(originalUrl);
	toOriginalUrl_[originalUrl] = originalUrl;
	downloader_.get(QNetworkRequest(url));
}

void Downloader::downloadFinished(QNetworkReply *reply)
{
	QString url = reply->request().url().toString();
	auto src = toOriginalUrl_[url];
	ASSERT(!src.isNull());
	toOriginalUrl_.erase(url);
	if (reply->error() != QNetworkReply::NoError){
		QString msg = tr("Original url: %1\n").arg(src);
		if (src != url)
			msg += tr("Redirect: %1\n").arg(url);
		msg += tr("Problem: %1\n").arg(reply->errorString());
		emit finished(src, QByteArray(), msg);
		return;
	}
	auto redir = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	if (redir.isNull()){
		emit finished(src, reply->readAll(), QString());
		return;
	}
	QUrl reUrl = redir.toUrl();
	if (reUrl.isEmpty()){
		QString msg = tr("url: %1\nreturned empty redirect").arg(src);
		emit finished(src, QByteArray(), msg);
		return;
	}
	if (reUrl.isRelative())
		reUrl = reply->request().url().resolved(reUrl);
	auto newUrl = reUrl.toString();
	if (toOriginalUrl_.contains(newUrl))
		return; // TODO: ideally have to check if original is different
	toOriginalUrl_[newUrl] = src;
	downloader_.get(QNetworkRequest(reUrl));
}

