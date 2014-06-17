#include "twitchstream.h"

TwitchStreamItem::TwitchStreamItem(QTreeWidget* parent, QString const& name)
	: StreamItem(parent, name)
{
	m_host = TWITCH_NAME;

	setIcon(0, QIcon(":twitch.ico"));
}

bool TwitchStreamItem::update()
{
	return true;
}
