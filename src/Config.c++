#include "Config.h"

using namespace std::filesystem;

Config::Config(QObject *parent) : QObject(parent)
{
	qRegisterMetaType<std::filesystem::path>();
	rootPath_ = toPath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first());
	create_directories(rootPath_);
}

QSettings Config::unimportant_settings()
{
	auto fn = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first() + "/window-state.cfg";
	return QSettings(fn, QSettings::Format::IniFormat);
}



