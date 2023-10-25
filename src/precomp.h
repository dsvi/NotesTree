#ifndef PRECOMP_H
#define PRECOMP_H

// TODO: remove
#define _LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QtCore>
#include <QtDebug>
#include <QtGui>
#include <QToolButton>
#include <QtWebKitWidgets>

#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <atomic>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <memory>
#include <mutex>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "App.h"
#include "Exceptions.h"

#ifdef DEBUG
	#define ASSERT(x) Q_ASSERT(x)
#else
	#define ASSERT(x)
#endif

// defines non const this. use 'mut_this->...'
#define mut_this const_cast<std::add_pointer<std::remove_const<std::remove_pointer<decltype(this)>::type>::type>::type>(this)

/// Signed 8-bit integer
typedef int8_t                   si8;
/// Signed 16-bit integer
typedef int16_t                  si16;
/// Signed 32-bit integer
typedef int32_t                  si32;
/// Signed 64-bit integer
typedef int64_t                  si64;

/// Unsigned 8-bit integer
typedef uint8_t                  ui8;
/// Unsigned 16-bit integer
typedef uint16_t                 ui16;
/// Unsigned 32-bit integer
typedef uint32_t                 ui32;
/// Unsigned 64-bit integer
typedef uint64_t                 ui64;

inline
QString toQS(const std::filesystem::path &p){
	return QString::fromStdString(p.string());
}

inline
std::filesystem::path toPath(const QString &p){
	return std::filesystem::path(p.toUtf8().constBegin());
}

inline
size_t utf8len(std::string_view str){
	size_t len = 0;
	for (unsigned char c : str)
		if ((c & 0xC0) != 0x80)
			len++;
	return len;
}

#endif // PRECOMP_H

