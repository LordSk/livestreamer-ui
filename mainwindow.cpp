#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtDebug>
#include <QInputDialog>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	m_updateTimer(this)

{
	ui->setupUi(this);

	// toolbar
	m_add = new QPushButton("+");
	m_remove = new QPushButton("-");
	m_watch = new QPushButton("Watch");
	m_update = new QPushButton("Update");

	m_add->setMinimumWidth(24);
	m_remove->setMinimumWidth(24);

	m_add->setToolTip("Add a stream");
	m_remove->setToolTip("Remove selected stream");
	m_watch->setToolTip("Watch selected stream");
	m_update->setToolTip("Update all streams");

	QObject::connect(m_add, SIGNAL(released()), this, SLOT(onAddButton_released()));
	QObject::connect(m_remove, SIGNAL(released()), this, SLOT(onRemoveButton_released()));
	QObject::connect(m_watch, SIGNAL(released()), this, SLOT(onWatchButton_released()));
	QObject::connect(m_update, SIGNAL(released()), this, SLOT(onUpdateButton_released()));

	auto toolbar = ui->toolBar;
	toolbar->addWidget(m_add);
	toolbar->addWidget(m_remove);
	toolbar->addWidget(m_watch);
	toolbar->addWidget(m_update);

	// list header
	auto streamList = ui->streamList;
	streamList->header()->setSectionResizeMode(QHeaderView::Fixed);
	streamList->setColumnWidth(StreamItem::COLUMN_ICON, 24); // (Icon)
	streamList->setColumnWidth(StreamItem::COLUMN_NAME, 150); // (Name)
	streamList->setColumnWidth(StreamItem::COLUMN_VIEWERS, 60); // (Viewers)
	streamList->setColumnWidth(StreamItem::COLUMN_QUALITY, 1); // (Quality)

	// create the config folder
	QDir configDir(CONFIG_PATH);
	if (!configDir.exists()){
	  configDir.mkdir(".");
	}

	// default settings
	m_settings.livestreamerPath = "livestreamer";
	m_settings.autoUpdateStreams = 0;
	m_settings.updateInterval = 60; // 60 seconds

	loadSettings();
	loadStreams();

	// get viewers and stuff
	updateStreams();

	// auto update
	ui->actionAutoUpdateStreams->setChecked(false);
	if(m_settings.autoUpdateStreams) {
		ui->actionAutoUpdateStreams->setChecked(true);
		m_updateTimer.start(m_settings.updateInterval * 1000);
	}

	connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(onUpdateTimer()));

	 // enable sorting by column
	streamList->setSortingEnabled(true);
	streamList->header()->setSortIndicator(2, Qt::DescendingOrder); // sort by viewer count
}

MainWindow::~MainWindow()
{
	saveSettings();
	saveStreams();

	for(auto s : m_streams) {
		delete s;
	}

	delete ui;
}

void MainWindow::statusStream(const QString& msg)
{
	ui->statusBar->setStyleSheet("QStatusBar { color: blue; }");
	ui->statusBar->showMessage(msg, 10000);
}

void MainWindow::statusValidate(const QString& msg)
{
	ui->statusBar->setStyleSheet("QStatusBar { color: green; }");
	ui->statusBar->showMessage(msg, 10000);
}

void MainWindow::statusError(const QString& msg)
{
	ui->statusBar->setStyleSheet("QStatusBar { color: red; }");
	ui->statusBar->showMessage(msg, 10000);
}

void MainWindow::on_actionAddStream_triggered()
{
	addStream();
}

void MainWindow::on_actionRemoveSelected_triggered()
{
	removeStream();
}

void MainWindow::on_actionClearAll_triggered()
{
	for(auto s : m_streams) {
		delete s;
	}

	m_streams.clear();
}

void MainWindow::on_actionSetLivestreamerLocation_triggered()
{
	QFileDialog dialog(this);

	dialog.setFilter(QDir::Files | QDir::Executable);
	dialog.setDirectory(m_settings.livestreamerPath);
#ifdef Q_OS_WIN
	dialog.setNameFilter(tr("Executable (*.exe)"));
#endif

	if(dialog.exec()) {
		QStringList selected = dialog.selectedFiles();

		if(selected.size() > 0)
			m_settings.livestreamerPath = selected.first();
	}
}

void MainWindow::on_actionAutoUpdateStreams_triggered()
{
	if(ui->actionAutoUpdateStreams->isChecked()) {
		m_settings.autoUpdateStreams = 1;
		m_updateTimer.start(m_settings.updateInterval * 1000);
	}
	else {
		m_settings.autoUpdateStreams = 0;
		m_updateTimer.stop();
	}
}

void MainWindow::on_actionAboutLivestreamerUI_triggered()
{
	// TODO: improve this
	QString aboutTxt = "<p><b>LivestreamerUI</b> is a Qt based user interface for <a href=\"https://github.com/chrippa/livestreamer\">livestreamer</a>.</p>\
			<p>It's totally free and open source. The project is hosted on Github <a href=\"https://github.com/LordSk/livestreamer-ui\">here</a>.</p>\
			<p>You can contact me on twitter <a href=\"https://twitter.com/LordSk_\">@LordSk_</a>.</p>";
	QMessageBox::about(this, "About LivestreamerUI", aboutTxt);
}

void MainWindow::on_actionAboutQt_triggered()
{
	QMessageBox::aboutQt(this);
}

void MainWindow::onAddButton_released()
{
	addStream();
}

void MainWindow::onRemoveButton_released()
{
	removeStream();
}

void MainWindow::onWatchButton_released()
{
	watchStream();
}

void MainWindow::onUpdateButton_released()
{
	updateStreams();
}

void MainWindow::on_streamList_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
	Q_UNUSED(item)
	Q_UNUSED(column)

	watchStream();
}

void MainWindow::onStreamStartError(int errorType, const QString& errorTxt)
{
	switch(errorType) {
		case StreamItem::ERROR_LS_NOT_FOUND:
			statusError("livestreamer not found");
			break;
		// TODO: should we use this?
		/*case StreamItem::ERROR_LS_CRASHED:
			statusError("livestreamer exited prematurely");
			break;*/
		case StreamItem::ERROR_LS_ERROR:
			statusError(errorTxt);
			break;
	}
}

void MainWindow::onUpdateTimer()
{
	updateStreams();
}

void MainWindow::addStream()
{
	bool ok;
	QString text = QInputDialog::getText(this, "Stream url", "Enter the stream url:", QLineEdit::Normal, "", &ok);

	if (ok && !text.isEmpty()) {
		try {
			StreamItem* newStream = createStreamItem(ui->streamList, text, "best");

			// check for duplicates
			bool duplicate = false;
			for(auto s : m_streams) {
				if(*s == *newStream) {
					duplicate = true;
					delete newStream;
					statusError("Error: duplicate.");
				}
			}

			if(!duplicate) {
				m_streams.append(newStream);
				newStream->update(); // update new stream
			}
		}
		catch(StreamException &e) {
			switch(e.getType()) {
			case StreamException::INVALID_URL:
				statusError("Invalid url.");
				break;
			case StreamException::HOST_NOT_SUPPORTED:
				statusError("Host not supported.");
				break;
			}
		}
	}
}

void MainWindow::removeStream()
{
	StreamItem* stream = getSelectedStream();

	if(stream) {
		int i = 0;
		for(auto s : m_streams) { // not efficient
			if(*s == *stream) {
				m_streams.remove(i);
				delete s;
				return;
			}

			i++;
		}
	}
}

void MainWindow::watchStream()
{
	StreamItem* stream = getSelectedStream();

	if(!stream || !stream->isOnline())
		return;

	statusStream(stream->getName() + " starting...");
	stream->watch(m_settings.livestreamerPath);

	// error signal
	QObject::connect(stream, SIGNAL(error(int,QString const&)), this, SLOT(onStreamStartError(int,QString const&)));
}

void MainWindow::updateStreams()
{
	for(auto s : m_streams) {
		s->update();
	}
}

StreamItem* MainWindow::getSelectedStream()
{
	auto selectedItems = ui->streamList->selectedItems();

	if(selectedItems.size() > 0) {
		return static_cast<StreamItem*>(selectedItems.first());
	}

	return nullptr;
}

void MainWindow::loadStreams()
{
	QFile file(CONFIG_PATH + "/" + STREAM_SAVE_FILENAME);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		statusError("Failed to load streams.");
		return;
	}

	while (!file.atEnd()) {
		QString line = file.readLine();
		line.remove('\n');
		QStringList split = line.split(" ", QString::SkipEmptyParts);

		QString url = split.first();
		QString quality = "best";
		if(split.size() > 1) {
			quality = split.last();
		}

		try {
			m_streams.append(createStreamItem(ui->streamList, url, quality));
		}
		catch(StreamException &e) {
			switch(e.getType()) {
			case StreamException::INVALID_URL:
				statusError("Loading: Invalid url.");
				break;
			case StreamException::HOST_NOT_SUPPORTED:
				statusError("Loading: Host not supported.");
				break;
			}
		}
	}

	statusValidate("Streams loaded.");
}

void MainWindow::saveStreams()
{
	QFile file(CONFIG_PATH + "/" + STREAM_SAVE_FILENAME);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return;

	QTextStream out(&file);

	for(auto s : m_streams) {
		out << s->getUrl() << " " << s->getQuality() << "\n";
	}
}

void MainWindow::loadSettings()
{
	QFile file(CONFIG_PATH + "/" + SETTINGS_FILENAME);
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		statusError("Failed to load settings.");
		return;
	}

	QString livestreamerPath;
	unsigned int autoUpdateStreams;
	unsigned int updateInterval;

	QString line = file.readLine();
	line.remove('\n');
	livestreamerPath = line;

	line = file.readLine();
	line.remove('\n');
	autoUpdateStreams = line.toInt();

	line = file.readLine();
	line.remove('\n');
	updateInterval = line.toInt();

	if(livestreamerPath.length() > 1)
		m_settings.livestreamerPath = livestreamerPath;
	if(autoUpdateStreams < 2)
		m_settings.autoUpdateStreams = autoUpdateStreams;
	if(updateInterval > 2 && updateInterval < 60*60*5) // 5h hours max
		m_settings.updateInterval = updateInterval;

	statusValidate("Settings loaded.");
}

void MainWindow::saveSettings()
{
	QFile file(CONFIG_PATH + "/" + SETTINGS_FILENAME);
	if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return;

	QTextStream out(&file);

	out << m_settings.livestreamerPath << "\n";
	out << m_settings.autoUpdateStreams << "\n";
	out << m_settings.updateInterval << "\n";
}
