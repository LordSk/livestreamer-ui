#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtDebug>
#include <QInputDialog>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>

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

	loadStreams();
}

MainWindow::~MainWindow()
{
	saveStreams();
	delete ui;

	for(auto s : m_streams) {
		delete s;
	}
}

void MainWindow::statusMsg(const QString& msg)
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
	auto selected = ui->streamList->selectedItems();
}

void MainWindow::on_actionClear_all_triggered()
{
	for(auto s : m_streams) {
		s->removeListItem();
		delete s;
	}

	m_streams.clear();
}

void MainWindow::on_streamList_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
	Q_UNUSED(column)

	for(auto s : m_streams) {
		if(s->sameListItem(item)) {
			try {
				s->watch();
			}
			catch(StreamException &e) {
				Q_UNUSED(e)
				statusMsg("Error: couldn't start livestreamer");
			}
			break;
		}
	}
}

void MainWindow::actionAddStream()
{
	bool ok;
	QString text = QInputDialog::getText(this, "Stream url", "Enter the stream url:", QLineEdit::Normal, "", &ok);

	if (ok && !text.isEmpty()) {
		try {
			Stream* newStream = parseStreamUrl(text);

			// check for duplicates
			bool duplicate = false;
			for(auto s : m_streams) {
				if(*s == *newStream) {
					duplicate = true;
					statusError("Error: duplicate.");
				}
			}

			if(!duplicate) {
				m_streams.append(newStream);
				m_streams.back()->createListItem(ui->streamList);
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
			m_streams.append(parseStreamUrl(line));
			m_streams.back()->createListItem(ui->streamList);
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

	statusMsg("Streams loaded.");
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
