#include "stream.h"
#include "twitchstream.h"
#include <QUrl>
#include <QtDebug>

StreamItem::StreamItem(QTreeWidget* parent, QString const& name)
	:QTreeWidgetItem(parent)
{
	m_host = "";
	m_name = name;
	m_viewerCount = 0;
	m_online = false;
	m_watching = false;
	m_process = nullptr;

	setIcon(0, QIcon(":twitch.ico")); // twitch icon by default
	setText(1, m_name);
	setText(2, "0");

	updateInfos();
}

StreamItem::~StreamItem()
{
	// kill livestreamer if we exit early
	if(m_process) {
		m_process->kill();
	}
}

void StreamItem::updateInfos()
{
	if(m_online) {
		if(!m_watching)
			setTextColor(1, QColor("black"));
		setTextColor(2, QColor("red"));
		setText(2, QString::number(m_viewerCount));
	}
	else {
		setTextColor(1, QColor("grey"));
		setTextColor(2, QColor("grey"));
	}
}

void StreamItem::setWatching(bool watching)
{
	m_watching = watching;

	if(watching)
		setTextColor(1, QColor("blue"));
	else
		setTextColor(1, QColor("black"));
}

void StreamItem::on_processFinished(int exitStatus)
{
	Q_UNUSED(exitStatus)
	setWatching(false);
	m_process = nullptr;
}

QString StreamItem::getUrl() const
{
	return m_host + "/" + m_name;
}

bool StreamItem::update()
{
	return true;
}

void StreamItem::watch(QString livestreamerPath, QString quality)
{
	// process already running, abort
	if(m_process)
		return;

	QString program = livestreamerPath;
	QStringList arguments;
	arguments << getUrl() << quality;

	m_process = new QProcess(this);
	m_process->start(program, arguments);

	if(!m_process->waitForStarted()) {
		throw StreamException(StreamException::LIVESTREAMER_FAILED);
	}

	setWatching(true);

	QObject::connect(m_process, SIGNAL(finished(int)), this, SLOT(on_processFinished(int)));
}

bool StreamItem::operator==(const StreamItem& other) const
{
	if(m_host == other.m_host && m_name == other.m_name)
		return true;
	return false;
}

StreamItem* createStreamItem(QTreeWidget* parent, QString const& url)
{
	// pre parsing via QUrl
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
			return new TwitchStreamItem(parent, name);
		}

		// host not supported
		throw StreamException(StreamException::HOST_NOT_SUPPORTED);
	}
	else {
		throw StreamException(StreamException::INVALID_URL);
	}

	return new StreamItem(parent, ""); // should never happen
}
