#ifndef TWITCHSTREAM_H
#define TWITCHSTREAM_H

#include "stream.h"

#define TWITCH_NAME "twitch.tv"

class TwitchStream : public Stream
{
protected:
	virtual QIcon getIcon();

public:
	TwitchStream(QString const& name);

	virtual bool update();
};

#endif // TWITCHSTREAM_H
