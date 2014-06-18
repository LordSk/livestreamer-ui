#ifndef TWITCHSTREAM_H
#define TWITCHSTREAM_H

#include "stream.h"
#include <QtNetwork/QNetworkAccessManager>

#define TWITCH_NAME "twitch.tv"

class TwitchStreamItem : public StreamItem
{
	QNetworkAccessManager m_netManager;

private slots:
	void replyFinished(QNetworkReply* reply);

public:
	TwitchStreamItem(QTreeWidget* parent, QString const& name);

	virtual bool update();
};

#endif // TWITCHSTREAM_H
