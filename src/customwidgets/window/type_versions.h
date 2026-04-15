#pragma once
#ifndef TYPE_VERSION
#define TYPE_VERSION
#include <QObject>

#    if (QT_VERSION_MAJOR == 6)
#define Q_RESULT_TYPE qintptr *
#    else
#define Q_RESULT_TYPE long *
#    endif

#endif
