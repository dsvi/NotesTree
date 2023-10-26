#ifndef CONFIG_H
#define CONFIG_H

class Config : public QObject
{
	Q_OBJECT
public:
	explicit Config(QObject *parent = 0);
	std::filesystem::path root();
	void emitRootChanged();
	
	QSettings unimportant_settings();

signals:
	void rootChanged(const std::filesystem::path &path);
public slots:

private:
	std::filesystem::path rootPath_;

};

inline
std::filesystem::path Config::root()
{
	return rootPath_;
}
inline
void Config::emitRootChanged()
{
	emit rootChanged(rootPath_);
}

Q_DECLARE_METATYPE(std::filesystem::path)

#endif // CONFIG_H
