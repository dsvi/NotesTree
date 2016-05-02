#include "Config.h"

using namespace boost::filesystem;
using namespace boost::property_tree;

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

void Config::saveUnimportantConfig(const Config::ptree &pt)
{
	try{
		write_info(QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first().toStdString(), pt);
	}
	catch(...){
		app->reportError(std::current_exception(), tr("While trying to save configuration file:"));
	}
}

Config::ptree Config::laodUnimportantConfig()
{
	try{
		auto path = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first().toStdString();
		ptree pt;
		if (exists(path))
			read_info(path, pt);
		return pt;
	}
	catch(...){
		app->reportError(std::current_exception(), tr("While trying to load configuration file:"));
	}
	return ptree();
}

