#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QPushButton>
#include <QString>
#include <QTimer>
#include "stream.h"
#include "configpath.h"

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
	void on_actionAddStream_triggered();
	void on_actionRemoveSelected_triggered();
	void on_actionClearAll_triggered();

	// Options menu
	void on_actionSetLivestreamerLocation_triggered();
	void on_actionAutoUpdateStreams_triggered();

	// About menu
	void on_actionAboutLivestreamerUI_triggered();
	void on_actionAboutQt_triggered();

	// Toolbar
	void onAddButton_released();
	void onRemoveButton_released();
	void onWatchButton_released();
	void onUpdateButton_released();

	// Item list
	void on_streamList_itemDoubleClicked(QTreeWidgetItem *item, int column);

	//
	void onStreamStartError(int errorType, QString const& errorTxt);
	void onUpdateTimer();

private:
	Ui::MainWindow *ui;
	QVector<StreamItem*> m_streams;

	QPushButton* m_add;
	QPushButton* m_remove;
	QPushButton* m_watch;
	QPushButton* m_update;

	struct {
		QString livestreamerPath;
		unsigned int autoUpdateStreams;
		unsigned int updateInterval;
	} m_settings;

	QTimer m_updateTimer;

	void addStream();
	void removeStream();
	void watchStream();
	void updateStreams();

	StreamItem* getSelectedStream();

	void loadStreams();
	void saveStreams();

	void loadSettings();
	void saveSettings();
};

#endif // MAINWINDOW_H
