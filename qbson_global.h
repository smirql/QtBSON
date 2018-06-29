#ifndef QBSON_GLOBAL_H
#define QBSON_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QBSON_LIBRARY)
#  define QBSONSHARED_EXPORT Q_DECL_EXPORT
#else
#  define QBSONSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // QBSON_GLOBAL_H
