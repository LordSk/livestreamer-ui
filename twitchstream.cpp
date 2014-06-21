#include "twitchstream.h"
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

QString TwitchStreamItem::getName() const
{
	return m_url.path().split("/", QString::SkipEmptyParts).first();
}

void TwitchStreamItem::replyFinished(QNetworkReply* reply)
{
	if(reply->error() == QNetworkReply::NoError) {
		QString strReply = (QString)reply->readAll();
		QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
		QJsonObject jsonObject = jsonResponse.object();
		QJsonValue stream = jsonObject.value("stream");

		m_online = false;
		m_viewerCount = 0;
		if(!stream.isNull() && !stream.isUndefined()) {
			m_online = true;
			m_viewerCount = stream.toObject()["viewers"].toInt();
		}

		updateWidgetItem();
	}

	delete reply;
}

TwitchStreamItem::TwitchStreamItem(QTreeWidget* parent, const QUrl& url, const QString& quality)
	: StreamItem(parent, url, quality),
	  m_netManager(this)
{
	setIcon(COLUMN_ICON, QIcon(":twitch.ico"));
	setText(COLUMN_NAME, getName());

	connect(&m_netManager, &QNetworkAccessManager::finished, this, &TwitchStreamItem::replyFinished);
}

bool TwitchStreamItem::update()
{
	m_netManager.get(QNetworkRequest(QUrl("https://api.twitch.tv/kraken/streams/" + getName() + "?client_id=typums7x8lg9a0esmu4y7vyqitufa3")));
	return true;
}
