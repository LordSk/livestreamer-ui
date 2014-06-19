#ifndef STREAM_H
#define STREAM_H

#include <QString>
#include <QIcon>
#include <QException>
#include <QTreeWidget>
#include <QProcess>

/**
 * @brief Base stream class
 */
class StreamItem: public QObject, public QTreeWidgetItem
{
	Q_OBJECT

	QProcess* m_process;

	enum {
		COLUMN_ICON,
		COLUMN_NAME,
		COLUMN_VIEWERS,
	};

	void setWatching(bool watching);

private slots:
	void on_processFinished(int exitStatus);

protected:
	QString m_host;
	QString m_name;
	int m_viewerCount;
	bool m_online;
	bool m_watching;

	void updateWidgetItem();

public:
	StreamItem(QTreeWidget* parent, QString const& name);
	virtual ~StreamItem();

	virtual bool update();
	void watch(QString livestreamerPath, QString quality);
	QString getUrl() const;
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
		LIVESTREAMER_FAILED
	};
};

/**
 * @brief Parse the url and returns a new StreamItem object
 * @param url
 * @return Stream*
 */
StreamItem* createStreamItem(QTreeWidget* parent, QString const& url);

#endif // STREAM_H
