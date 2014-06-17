#ifndef TWITCHSTREAM_H
#define TWITCHSTREAM_H

#include "stream.h"

#define TWITCH_NAME "twitch.tv"

class TwitchStreamItem : public StreamItem
{

public:
	TwitchStreamItem(QTreeWidget* parent, QString const& name);

	virtual bool update();
};

#endif // TWITCHSTREAM_H
