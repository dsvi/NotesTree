#ifndef CONFIG_H
#define CONFIG_H


class Config : public QObject
{
	Q_OBJECT
public:
	explicit Config(QObject *parent = 0);
	boost::filesystem::path root();
	void emitRootChanged();

signals:
	void rootChanged(const boost::filesystem::path &path);
public slots:

private:
	boost::filesystem::path rootPath_;

};

inline
boost::filesystem::path Config::root()
{
	return rootPath_;
}
inline
void Config::emitRootChanged()
{
	emit rootChanged(rootPath_);
}

Q_DECLARE_METATYPE(boost::filesystem::path)

#endif // CONFIG_H
