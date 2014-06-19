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
				 + "/" + CONFIG_DIR)

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

	m_settings.livestreamerPath = "livestreamer";
	m_settings.preferredQuality = QUALITY_BEST;
	m_settings.autoUpdateStreams = 0;
	m_settings.updateInterval = 60;

	loadSettings();

	updateStreamQuality(m_settings.preferredQuality);

	loadStreams();

	// get viewers and stuff
	actionUpdateStreams();

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
	ui->statusBar->showMessage(msg, 5000);
}

void MainWindow::statusValidate(const QString& msg)
{
	ui->statusBar->setStyleSheet("QStatusBar { color: green; }");
	ui->statusBar->showMessage(msg);
}

void MainWindow::statusError(const QString& msg)
{
	ui->statusBar->setStyleSheet("QStatusBar { color: red; }");
	ui->statusBar->showMessage(msg);
}

void MainWindow::on_actionAdd_stream_triggered()
{
	actionAddStream();
}

void MainWindow::on_actionRemove_selected_triggered()
{
	actionRemoveStream();
}

void MainWindow::on_actionClear_all_triggered()
{
	for(auto s : m_streams) {
		delete s;
	}

	m_streams.clear();
}

void MainWindow::on_actionLivestreamer_location_triggered()
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

void MainWindow::on_actionLow_triggered()
{
	updateStreamQuality(QUALITY_LOW);
}

void MainWindow::on_actionMedium_triggered()
{
	updateStreamQuality(QUALITY_MEDIUM);
}

void MainWindow::on_actionHigh_triggered()
{
	updateStreamQuality(QUALITY_HIGH);
}

void MainWindow::on_actionBest_triggered()
{
	updateStreamQuality(QUALITY_BEST);
}

void MainWindow::onAddButton_released()
{
	actionAddStream();
}

void MainWindow::onRemoveButton_released()
{
	actionRemoveStream();
}

void MainWindow::onWatchButton_released()
{
	actionWatchStream();
}

void MainWindow::onUpdateButton_released()
{
	actionUpdateStreams();
}

void MainWindow::on_streamList_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
	Q_UNUSED(item)
	Q_UNUSED(column)

	actionWatchStream();
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

void MainWindow::updateStreamQuality(unsigned int quality)
{
	if(quality >= QUALITY_MAX)
		return;

	m_settings.preferredQuality = quality;

	int i = 0;
	for(auto a : ui->menuPreferred_quality->actions()) {
		if(i == m_settings.preferredQuality)
			a->setChecked(true);
		else
			a->setChecked(false);
		i++;
	}
}

void MainWindow::actionAddStream()
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

void MainWindow::actionRemoveStream()
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

void MainWindow::actionWatchStream()
{
	StreamItem* s = getSelectedStream();

	if(!s)
		return;

	try {
		statusStream(s->getUrl() + " starting...");
		s->watch(m_settings.livestreamerPath, getQualityStr());
	}
	catch(StreamException &e) {
		Q_UNUSED(e)
		statusError("Error: couldn't start livestreamer.");
	}
}

void MainWindow::actionUpdateStreams()
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
		out << "http://" + s->getUrl() << "\n";
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
