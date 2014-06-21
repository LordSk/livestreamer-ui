#include "stream.h"
#include "twitchstream.h"
#include <QtDebug>

StreamItem::StreamItem(QTreeWidget* parent, const QUrl& url)
	:QTreeWidgetItem(parent)
{
	m_url = url;
	m_viewerCount = 0;
	m_online = false;
	m_watching = false;
	m_process = nullptr;

	setIcon(COLUMN_ICON, QIcon(":twitch.ico")); // twitch icon by default
	setText(COLUMN_NAME, getName());
	setText(COLUMN_VIEWERS, "0");
	setTextAlignment(COLUMN_VIEWERS, Qt::AlignRight);

	updateWidgetItem();
}

StreamItem::~StreamItem()
{
	// kill livestreamer if we exit early
	if(m_process) {
		m_process->kill();
	}
}

void StreamItem::updateWidgetItem()
{
	if(m_online) {
		if(m_watching)
			setTextColor(COLUMN_NAME, QColor("blue"));
		else
			setTextColor(COLUMN_NAME, QColor("black"));

		setTextColor(COLUMN_VIEWERS, QColor("red"));
		setText(COLUMN_VIEWERS, QString::number(m_viewerCount));
	}
	else {
		setTextColor(COLUMN_NAME, QColor("grey"));
		setTextColor(COLUMN_VIEWERS, QColor("grey"));
		setText(COLUMN_VIEWERS, QString::number(0));
	}
}

QString StreamItem::getName() const
{
	return m_url.path();
}

void StreamItem::setWatching(bool watching)
{
	m_watching = watching;
}

void StreamItem::on_processFinished(int exitStatus)
{
	setWatching(false);
	updateWidgetItem();

	// livestreamer crashed
	if(exitStatus != QProcess::NormalExit) {
		emit error(ERROR_LS_CRASHED, this);
	}

	m_process = nullptr;
}

QString StreamItem::getUrl() const
{
	return m_url.toString();
}

bool StreamItem::isOnline() const
{
	return m_online;
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
		emit error(ERROR_LS_NOT_FOUND, this);
		return;
	}

	setWatching(true);
	updateWidgetItem();

	QObject::connect(m_process, SIGNAL(finished(int)), this, SLOT(on_processFinished(int)));
}

bool StreamItem::operator==(const StreamItem& other) const
{
	if(m_url == other.m_url)
		return true;
	return false;
}

bool StreamItem::operator<(const QTreeWidgetItem& other) const
{
	int column = treeWidget()->sortColumn();

	switch(column) {
		case COLUMN_ICON: // icon name sort
			return icon(COLUMN_ICON).name().toLower() < other.icon(COLUMN_ICON).name().toLower();
		case COLUMN_NAME: // string sort
			return text(COLUMN_NAME).toLower() < other.text(COLUMN_NAME).toLower();
		case COLUMN_VIEWERS: // int sort
			return text(COLUMN_VIEWERS).toInt() < other.text(COLUMN_VIEWERS).toInt();
	}

	return text(column).toLower() < other.text(column).toLower();
}

StreamItem* createStreamItem(QTreeWidget* parent, QString const& url)
{
	QUrl qurl(url.toLower(), QUrl::StrictMode);

	if(!url.isEmpty() && qurl.isValid() && !qurl.host().isEmpty()) {
		// create stream object
		if(qurl.host().endsWith(TWITCH_NAME)) {
			return new TwitchStreamItem(parent, qurl);
		}

		// host not supported
		throw StreamException(StreamException::HOST_NOT_SUPPORTED);
	}
	else {
		throw StreamException(StreamException::INVALID_URL);
	}

	return new StreamItem(parent, qurl); // should never happen
}
