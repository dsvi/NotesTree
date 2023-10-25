#include "Downloader.h"

Downloader::Downloader(QObject *parent) : QObject(parent)
{
	connect(&downloader_, &QNetworkAccessManager::finished, this, [=, this](QNetworkReply *reply){
		downloadFinished(reply);
	});
}

void Downloader::get(const QString &originalUrl)
{
	auto url = QUrl(originalUrl);
	downloader_.get(QNetworkRequest(url));
	toOriginalUrl_[url.toString(QUrl::NormalizePathSegments)] = originalUrl;
}

void Downloader::downloadFinished(QNetworkReply *reply)
{
	QString url = reply->request().url().toString(QUrl::NormalizePathSegments);
	auto src = toOriginalUrl_[url];
	ASSERT(!src.isNull());
	if (reply->error() != QNetworkReply::NoError){
		QString msg = tr("Original url: %1\n").arg(src);
		if (src != url)
			msg += tr("Redirect: %1\n").arg(url);
		msg += tr("Problem: %1\n").arg(reply->errorString());
		emit finished(src, QByteArray(), msg);
		toOriginalUrl_.erase(url);
		return;
	}
	auto redir = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	if (redir.isNull()){
		emit finished(src, reply->readAll(), QString());
		toOriginalUrl_.erase(url);
		return;
	}
	QUrl reUrl = redir.toUrl();
	if (reUrl.isRelative())
		reUrl = reply->request().url().resolved(reUrl);
	toOriginalUrl_.erase(url);
	toOriginalUrl_[reUrl.toString(QUrl::NormalizePathSegments)] = std::move(src);
	downloader_.get(QNetworkRequest(reUrl));
}

