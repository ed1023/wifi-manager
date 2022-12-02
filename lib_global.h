#ifndef LIB_GLOBAL_H
#define LIB_GLOBAL_H

#include <QtCore/qglobal.h>
#include <string>

// macros for Logging/Debuging with file and line information
#define STR(x) #x
#define STRINGIFY(x) STR(x)
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define FILE_NAME_AND_LINE std::string(__FILENAME__) + "(" + STRINGIFY(__LINE__) + "): "
#define FILE_NAME_AND_LINE_CSTR (std::string(__FILENAME__) + "(" + STRINGIFY(__LINE__) + "): ").c_str()

#if defined(LIB_LIBRARY)
#  define LIBSHARED_EXPORT Q_DECL_EXPORT
#else
#  define LIBSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // LIB_GLOBAL_H
