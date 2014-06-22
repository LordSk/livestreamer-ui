#include "configpath.h"
#include <QStandardPaths>

static const QString g_configPath(QStandardPaths::locate(QStandardPaths::DocumentsLocation, QString(), QStandardPaths::LocateDirectory)
						  + "/" + CONFIG_DIR_NAME);
