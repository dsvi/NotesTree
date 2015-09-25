#include "FilenameEncoder.h"


QString DecodeFromFilename(const QString &fn)
{
	QString ret;
	ret.reserve(fn.size());
	for (auto c = fn.begin(); c != fn.end(); ++c){
		if (*c == delimChar){
			QString num;
			for (int i = 2; --i >= 0;){
				if (++c == fn.end()){
					qWarning()<< "Wrong num after delimiter in file name: " << fn;
					return ret;
				}
				num += *c;
			}
			bool ok;
			int code = num.toInt(&ok, 16);
			if (!ok){
				qWarning()<< "Wrong num after delimiter in file name: " << fn;
				return ret;
			}
			ret += QChar(code);
		}
		else{
			ret += *c;
		}
	}
	return ret;
}

QString EncodeToFilename(const QString &name)
{
	QString ret;
	ret.reserve(name.size());
	for (auto c:name){
		if (c == delimChar || c == L'/' || c == L'\\'){
			ret += delimChar;
			QString num;
			num.setNum(c.unicode(), 16);
			ASSERT(num.length() <= 2);
			if (num.length() < 2)
				ret += L'0';
			ret += num;
		}
		else{
			ret += c;
		}
	}
	return ret;
}
