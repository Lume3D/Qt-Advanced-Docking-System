#ifndef ADS_STYLED_WINDOW_P_H
#define ADS_STYLED_WINDOW_P_H

// Private to the styled_window translation units. StyledWindowPrivate and
// the Win32 helpers below are shared between styled_window.cpp and
// styled_window_win.cpp; nothing outside this directory may include this.

#include <QPointer>

#include <array>

#include "styled_window.h"
#include "utils.h"

#ifdef Q_OS_WIN
#    include "widget_event_helper.h"
#    include "win_utils.h"
#endif

namespace ads
{
struct StyledWindow::StyledWindowPrivate
{
    QWidget* leftLayoutWidget_{nullptr};
    QWidget* rightLayoutWidget_{nullptr};
    QPushButton* maximize_{nullptr};
    QPushButton* minimize_{nullptr};
    QPushButton* close_{nullptr};
    QPushButton* logo_{nullptr};
    QMenuBar* menuBar_{nullptr};
    QToolBar* windowHint_{nullptr};
    QLabel* titleLabel_{nullptr};
    QString windowTitle_{"ADS"};

    QWidget* titleBar_{nullptr};
    QList<QWidget*> whiteList_;
    QMargins margins_;
    QMargins frames_;
    QWidget* divider_{nullptr};

    int borderWidth_{0};
    bool justMaximized_{false};
    bool justMinimized_{false};
    bool resizeable_{true};

    WidgetEventHelper* maximizeHelper_{nullptr};
    WidgetEventHelper* minimizeHelper_{nullptr};
    WidgetEventHelper* closeHelper_{nullptr};
    WidgetEventHelper* menuHelper_{nullptr};

    // The title-bar chrome buttons paired with the hit-test code identifying
    // each one. Several native messages are fanned out to all four in this
    // order, and the WM_NC*BUTTON* messages arrive carrying a hit-test code
    // that has to be mapped back to the button owning it.
    struct ChromeButton
    {
        WidgetEventHelper* helper;
        LONG hitTest;
    };

    std::array<ChromeButton, 4> chromeButtons() const
    {
        return {{{menuHelper_, HTSYSMENU},
                 {minimizeHelper_, HTMINBUTTON},
                 {maximizeHelper_, HTMAXBUTTON},
                 {closeHelper_, HTCLOSE}}};
    }

    bool chromeHelpersReady() const
    {
        return menuHelper_ && minimizeHelper_ && maximizeHelper_ && closeHelper_;
    }

    WidgetEventHelper* chromeHelperFor(WPARAM hitTest) const
    {
        for (const auto& button : chromeButtons())
        {
            if (button.hitTest == static_cast<LONG>(hitTest))
            {
                return button.helper;
            }
        }
        return nullptr;
    }
    float displayScale_{1.f};

    bool initResize_{false};

#ifdef Q_OS_WIN
    QWindow* proxyWindow_{nullptr};
    HMENU sysMenu_{nullptr};
    HBRUSH backgroundBrush_{nullptr};
    HPOWERNOTIFY suspendResumeNotification_{nullptr};
    bool cloakPending_{false};
    bool cloaked_{false};
    bool inSizeMove_{false};
    bool pendingStateResizePaint_{false};
    bool uncloakQueued_{false};
    bool focusRestoreQueued_{false};
    bool darkModeSettingGuard_{false};
    QPointer<QWidget> lastFocusedWidget_;
#endif
};

#ifdef Q_OS_WIN

// The system reports a theme change before it has finished applying it;
// re-asserting dark mode after a short delay is what makes it stick.
constexpr int kDarkModeRefreshDelayMs = 100;
// Offsets of the system menu from the logo button's bottom-left corner,
// in device-independent pixels.
constexpr int kSystemMenuOffsetX = 4;
constexpr int kSystemMenuOffsetY = 12;
// Edge length of the maximize/restore glyph, in device-independent pixels.
constexpr int kHintIconSize = 18;

// Defined in styled_window_win.cpp.
float nativeWindowDpr(HWND hwnd, float fallback);
const char* maximizeIconPath(bool maximized);
HRESULT forceDarkMode(HWND hwnd);

#endif  // Q_OS_WIN

}  // namespace ads

#endif  // ADS_STYLED_WINDOW_P_H
