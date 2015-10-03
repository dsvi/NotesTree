#ifndef PRECOMP_H
#define PRECOMP_H

#include <QtCore>
#include <QtGui>
#include <QtDebug>
#include <QApplication>
#include <QMainWindow>
#include <QMessageBox>

#include <exception>
#include <memory>
#include <vector>
#include <stack>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <cstring>
#include <memory>

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

#endif // PRECOMP_H

// shit below is to fix QtCreator clang autocompletion
//#define _GLIBCXX_BEGIN_NAMESPACE(name) namespace name {
//#define _GLIBCXX_END_NAMESPACE }
//#define _GLIBCXX_BEGIN_NAMESPACE_CONTAINER
//#define _GLIBCXX_END_NAMESPACE_CONTAINER
//#define _GLIBCXX_END_NAMESPACE_VERSION
//#define _GLIBCXX_BEGIN_NAMESPACE_VERSION
//#define __glibcxx_function_requires(...)
//#define __try try
//#define __catch catch
//#define __cplusplus 999999L
