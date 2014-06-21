#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtDebug>
#include <QInputDialog>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),

	m_configPath(QStandardPaths::locate(QStandardPaths::DocumentsLocation, QString(), QStandardPaths::LocateDirectory)
				 + "/" + CONFIG_DIR),
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
	streamList->setColumnWidth(0, 24); // first column (website icon)
	streamList->setColumnWidth(1, 150); // second column (Name)
	streamList->setColumnWidth(2, 50); // third column (Viewers)

	// create the config folder
	QDir configDir(m_configPath);
	if (!configDir.exists()){
	  configDir.mkdir(".");
	}

	// default settings
	m_settings.livestreamerPath = "livestreamer";
	m_settings.preferredQuality = QUALITY_BEST;
	m_settings.autoUpdateStreams = 0;
	m_settings.updateInterval = 60; // 60 seconds

	loadSettings();
	loadStreams();

	// set stream quality
	setStreamQuality(m_settings.preferredQuality);

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
	delete ui;

	for(auto s : m_streams) {
		delete s;
	}
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

void MainWindow::on_actionSetQualityLow_triggered()
{
	setStreamQuality(QUALITY_LOW);
}

void MainWindow::on_actionSetQualityMedium_triggered()
{
	setStreamQuality(QUALITY_MEDIUM);
}

void MainWindow::on_actionSetQualityHigh_triggered()
{
	setStreamQuality(QUALITY_HIGH);
}

void MainWindow::on_actionSetQualityBest_triggered()
{
	setStreamQuality(QUALITY_BEST);
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

void MainWindow::onStreamStartError(int errorType, StreamItem* stream)
{
	Q_UNUSED(stream); // TODO fix this;
	QString error("");

	switch(errorType) {
		case StreamItem::ERROR_LS_NOT_FOUND:
			error = "livestreamer not found";
			break;
		case StreamItem::ERROR_LS_CRASHED:
			error = "livestreamer exited prematurely";
			break;
	}

	statusError("Error: " + error);
}

void MainWindow::onUpdateTimer()
{
	updateStreams();
}

QString MainWindow::getQualityStr()
{
	switch(m_settings.preferredQuality) {
	case QUALITY_LOW:
		return "low";
	case QUALITY_MEDIUM:
		return "medium";
	case QUALITY_HIGH:
		return "high";
	case QUALITY_BEST:
		return "best";
	}

	return "best";
}

void MainWindow::setStreamQuality(unsigned int quality)
{
	if(quality >= QUALITY_MAX)
		return;

	m_settings.preferredQuality = quality;

	int i = 0;
	for(auto a : ui->menuPreferredQuality->actions()) {
		if(i == m_settings.preferredQuality)
			a->setChecked(true);
		else
			a->setChecked(false);
		i++;
	}
}

void MainWindow::addStream()
{
	bool ok;
	QString text = QInputDialog::getText(this, "Stream url", "Enter the stream url:", QLineEdit::Normal, "", &ok);

	if (ok && !text.isEmpty()) {
		try {
			StreamItem* newStream = createStreamItem(ui->streamList, text);

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
	stream->watch(m_settings.livestreamerPath, getQualityStr());

	// error signal
	QObject::connect(stream, SIGNAL(error(int,StreamItem*)), this, SLOT(onStreamStartError(int,StreamItem*)));
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
	QFile file(m_configPath + "/" + STREAM_SAVE_FILENAME);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		statusError("Failed to load streams.");
		return;
	}

	while (!file.atEnd()) {
		QString line = file.readLine();
		line.remove('\n');
		try {
			m_streams.append(createStreamItem(ui->streamList, line));
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
	QFile file(m_configPath + "/" + STREAM_SAVE_FILENAME);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return;

	QTextStream out(&file);

	for(auto s : m_streams) {
		out << s->getUrl() << "\n";
	}
}

void MainWindow::loadSettings()
{
	QFile file(m_configPath + "/" + SETTINGS_FILENAME);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		statusError("Failed to load settings.");
		return;
	}

	QString livestreamerPath;
	unsigned int preferredQuality;
	unsigned int autoUpdateStreams;
	unsigned int updateInterval;

	QString line = file.readLine();
	line.remove('\n');
	livestreamerPath = line;

	line = file.readLine();
	line.remove('\n');
	preferredQuality = line.toInt();

	line = file.readLine();
	line.remove('\n');
	autoUpdateStreams = line.toInt();

	line = file.readLine();
	line.remove('\n');
	updateInterval = line.toInt();

	if(livestreamerPath.length() > 1)
		m_settings.livestreamerPath = livestreamerPath;
	if(preferredQuality < QUALITY_MAX)
		m_settings.preferredQuality = preferredQuality;
	if(autoUpdateStreams < 2)
		m_settings.autoUpdateStreams = autoUpdateStreams;
	if(updateInterval > 2 && updateInterval < 60*60*5) // 5h hours max
		m_settings.updateInterval = updateInterval;

	statusValidate("Settings loaded.");
}

void MainWindow::saveSettings()
{
	QFile file(m_configPath + "/" + SETTINGS_FILENAME);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return;

	QTextStream out(&file);

	out << m_settings.livestreamerPath << "\n";
	out << m_settings.preferredQuality << "\n";
	out << m_settings.autoUpdateStreams << "\n";
	out << m_settings.updateInterval << "\n";
}
