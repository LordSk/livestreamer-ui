#include "stream.h"
#include "twitchstream.h"
#include "configpath.h"
#include <QtDebug>
#include <QFile>

StreamItem::StreamItem(QTreeWidget* parent, const QUrl& url, const QString& quality)
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
	setTextAlignment(COLUMN_VIEWERS, Qt::AlignRight | Qt::AlignVCenter);

	m_cbQuality = new QComboBox(parent);
	// default qualities
	m_cbQuality->addItem("worst");
	m_cbQuality->addItem("best");

	// select quality
	selectQuality(quality);

	parent->setItemWidget(this, COLUMN_QUALITY, m_cbQuality);

	updateWidgetItem();
}

StreamItem::~StreamItem()
{
	delete m_cbQuality;
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

QString StreamItem::getQuality() const
{
	return m_quality;
}

void StreamItem::setWatching(bool watching)
{
	m_watching = watching;
}

void StreamItem::selectQuality(const QString& quality)
{
	m_quality = quality;
	if(m_cbQuality->findText(quality) == -1) { // quality not found, add an entry for it
		m_cbQuality->addItem(quality);
	}
	m_cbQuality->setCurrentText(quality);
}

void StreamItem::onProcessFinished(int exitStatus)
{
	setWatching(false);
	updateWidgetItem();

	// livestreamer crashed
	if(exitStatus != QProcess::NormalExit) {
		emit error(ERROR_LS_CRASHED, this);
	}

	m_process = nullptr;

	QFile file(CONFIG_PATH + "/" + getName() + ".log");
	if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return;

	QTextStream out(&file);
	for(auto s : m_processLog) {
		out << s << "\n";
	}
}

void StreamItem::onProcessStdOut()
{
	QString line = m_process->readAllStandardOutput();
	line.remove('\n');
	line.remove('\r');

	if(!line.isEmpty()) {
		m_processLog.append(line);

		if(line.startsWith("[cli][info] ")) {
			line.remove("[cli][info] ");

			// update quality combobox to add available qualities
			if(line.startsWith("Available streams: ")) {
				line.remove("Available streams: ");
				auto qualityList = line.split(", ");

				m_cbQuality->clear();
				m_cbQuality->addItem("best");
				for(QString s : qualityList) {
					if(s.count("worst") == 0 && s.count("best") == 0) {
						m_cbQuality->addItem(s);
					}
				}
				m_cbQuality->addItem("worst");

				// reselect quality since we cleared
				selectQuality(m_quality);
			}
		}
	}
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

void StreamItem::watch(QString livestreamerPath)
{
	// process already running, abort
	if(m_process)
		return;

	QString program = livestreamerPath;
	QStringList arguments;
	arguments << getUrl() << getQuality();

	m_process = new QProcess(this);
	m_process->start(program, arguments);
	m_process->setProcessChannelMode(QProcess::MergedChannels);

	if(!m_process->waitForStarted()) {
		emit error(ERROR_LS_NOT_FOUND, this);
		return;
	}

	setWatching(true);
	updateWidgetItem();

	QObject::connect(m_process, SIGNAL(finished(int)), this, SLOT(onProcessFinished(int)));
	QObject::connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(onProcessStdOut()));
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

StreamItem* createStreamItem(QTreeWidget* parent, QString const& url, QString const& quality)
{
	QUrl qurl(url.toLower(), QUrl::StrictMode);

	if(!url.isEmpty() && qurl.isValid() && !qurl.host().isEmpty()) {
		// create stream object
		if(qurl.host().endsWith(TWITCH_NAME)) {
			return new TwitchStreamItem(parent, qurl, quality);
		}

		// host not supported
		throw StreamException(StreamException::HOST_NOT_SUPPORTED);
	}
	else {
		throw StreamException(StreamException::INVALID_URL);
	}

	return new StreamItem(parent, qurl, quality); // should never happen
}
