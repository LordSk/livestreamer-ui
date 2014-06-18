#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QPushButton>
#include <QString>
#include "stream.h"

#define CONFIG_DIR "LivestreamerUI"
#define STREAM_SAVE_FILENAME "streams.list"
#define SETTINGS_FILENAME "settings.cfg"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	void statusStream(QString const& msg);
	void statusValidate(QString const& msg);
	void statusError(QString const& msg);

private slots:
	// Stream menu
	void on_actionAdd_stream_triggered();
	void on_actionRemove_selected_triggered();
	void on_actionClear_all_triggered();

	// Options menu
	void on_actionLivestreamer_location_triggered();
	void on_actionLow_triggered();
	void on_actionMedium_triggered();
	void on_actionHigh_triggered();
	void on_actionBest_triggered();

	// Toolbar
	void onAddButton_released();
	void onRemoveButton_released();
	void onWatchButton_released();
	void onUpdateButton_released();

	// Item list
	void on_streamList_itemDoubleClicked(QTreeWidgetItem *item, int column);

private:
	Ui::MainWindow *ui;
	QVector<StreamItem*> m_streams;

	QPushButton* m_add;
	QPushButton* m_remove;
	QPushButton* m_watch;
	QPushButton* m_update;

	const QString m_configPath;

	enum Quality {
		QUALITY_LOW=0,
		QUALITY_MEDIUM,
		QUALITY_HIGH,
		QUALITY_BEST,
		QUALITY_MAX
	};

	struct {
		QString livestreamerPath;
		unsigned int preferredQuality;
		unsigned int autoUpdateStreams;
		unsigned int updateInterval;
	} m_settings;

	QString getQualityStr();
	void updateStreamQuality(unsigned int quality);

	void actionAddStream();
	void actionRemoveStream();
	void actionWatchStream();
	void actionUpdateStreams();

	StreamItem* getSelectedStream();

	void loadStreams();
	void saveStreams();

	void loadSettings();
	void saveSettings();
};

#endif // MAINWINDOW_H
