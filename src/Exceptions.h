#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H


class Exception : public std::exception
{
public:
	Exception(const char *translationContext, const char *message);
	template<typename T>
	Exception operator % (const T &t) && {
		msg_  = std::move(msg_).arg(t);
		return std::move(*this);
	}
	const char *what() const noexcept override;
private:
	QString msg_;
	mutable QByteArray what_;
};

#endif // EXCEPTIONS_H
