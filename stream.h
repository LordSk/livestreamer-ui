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
class Stream: public QObject
{
	Q_OBJECT

	QTreeWidgetItem* m_listItem;
	QProcess* m_process;

	void setWatching(bool watching);

private slots:
	void on_processFinished(int exitStatus);

protected:
	QString m_host;
	QString m_name;
	int m_viewerCount;
	bool m_online;

	virtual QIcon getIcon() const;

public:
	Stream();
	Stream(QString const& name);
	virtual ~Stream();

	virtual bool update();

	void watch();

	QString getUrl() const;

	// visual representation (QTreeWidgetItem)
	void createListItem(QTreeWidget* parent);
	void removeListItem();
	bool sameListItem(QTreeWidgetItem* other) const;

	bool operator==(Stream const& other) const;
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
 * @brief Parse the url and returns a Stream object
 * @param url
 * @return Stream*
 */
Stream* parseStreamUrl(QString const& url);

#endif // STREAM_H
