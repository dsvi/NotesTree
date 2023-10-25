#ifndef CONFIG_H
#define CONFIG_H


class Config : public QObject
{
	Q_OBJECT
public:
	explicit Config(QObject *parent = 0);
	std::filesystem::path root();
	void emitRootChanged();

	using ptree = boost::property_tree::ptree;
	void  saveUnimportantConfig(const ptree &pt);
	ptree laodUnimportantConfig();


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

template <typename Ch, typename Traits>
std::basic_ostream<Ch, Traits>&	operator << (std::basic_ostream<Ch, Traits>&s, const QByteArray &ba){
	s << ba.toHex().data();
	return s;
}

template <typename Ch, typename Traits>
std::basic_istream<Ch, Traits>&	operator >> (std::basic_istream<Ch, Traits>&s, QByteArray &ba){
	while(true) {
		auto c = s.peek();
		if (!s.good())
			break;
		s.get();
		ba.append(c);
	}
	ba = QByteArray::fromHex(ba);
	return s;
}

#endif // CONFIG_H
