#ifndef ADS_WIN32_EVENT_UTILS
#define ADS_WIN32_EVENT_UTILS

#ifdef WIN32

#undef NOMINMAX

#include <windows.h>
#include <WinUser.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <objidl.h> // Fixes error C2504: 'IUnknown'
#include <gdiplus.h>
#include <GdiPlusColor.h>
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "user32.lib")

#include <QOperatingSystemVersion>
#include <QLabel>
#include <QToolBar>
#include <QMenuBar>
#include <QPushButton>
#include <QEvent>
#include <QVBoxLayout>

constexpr int CUSTOM_FRAME_THICKNESS = 1;
constexpr int TITLE_BAR_HEIGHT = 32;
#define W_10 (QSysInfo::productVersion().contains("10") && QOperatingSystemVersion::current().microVersion() < 21327)
const QString CUSTOM_FRAME_STYLE_ENABLE =
    QString::fromStdString("{ border: " + std::to_string(CUSTOM_FRAME_THICKNESS) + "px solid #BEBEBE }");
#define CUSTOM_FRAME_STYLE_DISABLE "{ border: 0px }"
#undef MIN
#undef MAX
#undef min
#undef max

#endif
#endif