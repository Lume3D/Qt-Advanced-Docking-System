#ifndef ADS_TYPE_VERSIONS_H
#define ADS_TYPE_VERSIONS_H

#include <QObject>

#if (QT_VERSION_MAJOR == 6)
#    define Q_RESULT_TYPE qintptr*
#else
#    define Q_RESULT_TYPE long*
#endif

#endif  // ADS_TYPE_VERSIONS_H
