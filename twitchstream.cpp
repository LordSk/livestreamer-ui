#include "twitchstream.h"

TwitchStream::TwitchStream(QString const& name)
	: Stream(name)
{
	m_host = TWITCH_NAME;
}

bool TwitchStream::update()
{
	return true;
}

QIcon TwitchStream::getIcon()
{
	return QIcon(":twitch.ico");
}
