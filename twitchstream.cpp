#include "twitchstream.h"
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

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

TwitchStreamItem::TwitchStreamItem(QTreeWidget* parent, QString const& name)
	: StreamItem(parent, name),
	  m_netManager(this)
{
	m_host = TWITCH_NAME;
	setIcon(0, QIcon(":twitch.ico"));

	connect(&m_netManager, &QNetworkAccessManager::finished, this, &TwitchStreamItem::replyFinished);
}

bool TwitchStreamItem::update()
{
	m_netManager.get(QNetworkRequest(QUrl("https://api.twitch.tv/kraken/streams/" + m_name + "?client_id=typums7x8lg9a0esmu4y7vyqitufa3")));
	return true;
}
