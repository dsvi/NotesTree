#ifndef PRECOMP_H
#define PRECOMP_H

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

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <atomic>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <memory>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "App.h"
#include "Exceptions.h"


namespace std{
template<>
struct hash<class QString>{
  size_t operator()(const QString &s) const
  {
    return qHash(s);
  }
};
}

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
QString toQS(const boost::filesystem::path &p){
	return QString::fromStdWString(p.wstring());
}

inline
boost::filesystem::path toPath(const QString &p){
	return boost::filesystem::path(p.toUtf8());
}

#endif // PRECOMP_H

