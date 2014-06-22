#ifndef STREAM_H
#define STREAM_H

#include <QString>
#include <QIcon>
#include <QException>
#include <QTreeWidget>
#include <QProcess>
#include <QUrl>
#include <QComboBox>

/**
 * @brief Base stream class
 */
class StreamItem: public QObject, public QTreeWidgetItem
{
	Q_OBJECT

	QProcess* m_process;
	QStringList m_processLog;

	void setWatching(bool watching);
	void selectQuality(QString const& quality);

private slots:
	void onProcessFinished(int exitStatus);
	void onProcessStdOut();

signals:
	void error(int errorType, StreamItem* stream);

protected:
	QUrl m_url;
	int m_viewerCount;
	bool m_online;
	bool m_watching;
	QComboBox* m_cbQuality;
	QString m_quality;

	void updateWidgetItem();

public:
	enum {
		COLUMN_ICON,
		COLUMN_NAME,
		COLUMN_VIEWERS,
		COLUMN_QUALITY
	};

	enum {
		ERROR_LS_NOT_FOUND,
		ERROR_LS_CRASHED
	};

	StreamItem(QTreeWidget* parent, QUrl const& url, QString const& quality);
	virtual ~StreamItem();

	virtual bool update();
	void watch(QString livestreamerPath);
	QString getUrl() const;
	virtual QString getName() const;
	QString getQuality() const;
	bool isOnline() const;

	bool operator==(StreamItem const& other) const;
	virtual bool operator<(const QTreeWidgetItem &other) const;
};

/**
 * @brief Basic exception class
 */
class StreamException : public QException
{
	int m_type;

public:
	StreamException(int type) { m_type = type; }
	void raise() const { throw *this; }
	StreamException *clone() const { return new StreamException(*this); }
	int getType() { return m_type; }

	enum {
		INVALID_URL,
		HOST_NOT_SUPPORTED,
	};
};

/**
 * @brief Parse the url and returns a new StreamItem object
 * @param url
 * @return Stream*
 */
StreamItem* createStreamItem(QTreeWidget* parent, QString const& url, QString const& quality);

#endif // STREAM_H
