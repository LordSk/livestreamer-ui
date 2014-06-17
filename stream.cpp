#include "stream.h"
#include "twitchstream.h"
#include <QUrl>
#include <QtDebug>

Stream::Stream()
	: Stream("")
{
}

Stream::Stream(QString const& name)
{
	m_host = "";
	m_name = name;
	m_viewerCount = 0;
	m_online = false;
	m_listItem = nullptr;
	m_process = nullptr;
}

Stream::~Stream()
{
	// kill livestreamer if we exit early
	if(m_process) {
		m_process->kill();
	}
}

void Stream::setWatching(bool watching)
{
	if(watching)
		m_listItem->setTextColor(1, QColor("blue"));
	else
		m_listItem->setTextColor(1, QColor("black"));
}

void Stream::on_processFinished(int exitStatus)
{
	Q_UNUSED(exitStatus)
	setWatching(false);
	m_process = nullptr;
}

QIcon Stream::getIcon() const
{
	return QIcon(":twitch.ico");
}

QString Stream::getUrl() const
{
	return "http://" + m_host + "/" + m_name;
}

bool Stream::update()
{
	return true;
}

void Stream::watch()
{
	// process already running, abort
	if(m_process)
		return;

	QString program = "livestreamer";
	QStringList arguments;
	arguments << getUrl() << "best";

	m_process = new QProcess(this);
	m_process->start(program, arguments);

	if(!m_process->waitForStarted()) {
		throw StreamException(StreamException::LIVESTREAMER_FAILED);
	}

	setWatching(true);

	QObject::connect(m_process, SIGNAL(finished(int)), this, SLOT(on_processFinished(int)));
}

void Stream::createListItem(QTreeWidget* parent)
{
	m_listItem = new QTreeWidgetItem(parent);
	m_listItem->setIcon(0, getIcon());
	m_listItem->setText(1, m_name);
	m_listItem->setText(2, "0");
}

void Stream::removeListItem()
{
	if(m_listItem)
		delete m_listItem;
}

bool Stream::sameListItem(QTreeWidgetItem* other) const
{
	if(m_listItem) {
		if(m_listItem == other && m_listItem->text(1) == other->text(1))
			return true;
	}

	return false;
}

bool Stream::operator==(const Stream& other) const
{
	if(m_host == other.m_host && m_name == other.m_name)
		return true;
	return false;
}

Stream* parseStreamUrl(QString const& url)
{
	QUrl qurl(url, QUrl::StrictMode);

	if(!url.isEmpty() && qurl.isValid() && !qurl.host().isEmpty()) {
		// stream host
		QStringList hostSplit = qurl.host().split(".", QString::SkipEmptyParts);
		if(hostSplit.first() == "www") {
			hostSplit.removeFirst();
		}
		QString host = hostSplit.join(".");

		// stream name
		QString name = qurl.path().split("/", QString::SkipEmptyParts)[0];

		// create stream object
		if(host == TWITCH_NAME) {
			return new TwitchStream(name);
		}

		// host not supported
		throw StreamException(StreamException::HOST_NOT_SUPPORTED);
	}
	else {
		throw StreamException(StreamException::INVALID_URL);
	}

	return new Stream("");
}
