#include "Config.h"

using namespace boost::filesystem;

Config::Config(QObject *parent) : QObject(parent)
{
	qRegisterMetaType<boost::filesystem::path>();
#ifdef DEBUG
	rootPath_ = toPath(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	rootPath_ /= "OTest";
#else
	rootPath_ = toPath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first());
#endif
	create_directories(rootPath_);
}

