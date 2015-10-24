#include "Exceptions.h"


Exception::Exception(QString &&msg)
{
	msg_ = std::move(msg);
}

void Exception::append(const QString &someMore)
{
	msg_.append(someMore);
}

const char *Exception::what() const noexcept
{
	try{
		what_ = msg_.toUtf8();
	}
	catch(...){
		return "Exception:: not enough memory to even format the string";
	}
	return what_.data();
}

bool isRecoverable(const std::exception &e)
{
	if (dynamic_cast<const RecoverableException*>(&e))
		return true;
	try {
		std::rethrow_if_nested(e);
	} catch(const std::exception& e) {
		return isRecoverable(e);
	}
	return false;
}

bool isRecoverable(std::exception_ptr ep)
{
	try{
		std::rethrow_exception(ep);
	}
	catch(const std::exception &e){
		return isRecoverable(e);
	}
}
