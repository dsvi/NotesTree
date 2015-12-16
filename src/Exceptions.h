#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

bool isRecoverable(const std::exception &e);
bool isRecoverable(std::exception_ptr e);

class Exception : public std::exception
{
public:
	explicit
	Exception(QString &&msg);
	void append(const QString &someMoreInfo);
	const char *what() const noexcept override;
private:
	QString msg_;
	mutable QByteArray what_;
};

class RecoverableException : public Exception
{
public:
	explicit
	RecoverableException(QString &&msg) :
		Exception(std::move(msg)){};
};

Q_DECLARE_METATYPE(std::exception_ptr)

#endif // EXCEPTIONS_H
