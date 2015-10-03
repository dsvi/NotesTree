#include "Exceptions.h"


Exception::Exception(const char *translationContext, const char *message)
{
	msg_ = QCoreApplication::translate(translationContext, message);
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
