

#include "styled_window.h"

#include <QList>

#include "utils.h"

#ifdef Q_OS_WIN
#    include "widget_event_helper.h"
#endif

#ifdef Q_OS_MACOS
#    include "macos_helper.h"
#endif
namespace
{

#ifdef Q_OS_WIN

bool updateNativeWindowMargins(HWND hwnd, QMargins margins)
{
    MARGINS winMargins;

    winMargins.cxLeftWidth = margins.left();
    winMargins.cxRightWidth = margins.right();
    winMargins.cyBottomHeight = margins.bottom();
    winMargins.cyTopHeight = margins.top();

    auto hr = DwmExtendFrameIntoClientArea(hwnd, &winMargins);
    return SUCCEEDED(hr);
}
#endif

}  // namespace

namespace ads
{
struct StyledWindow::StyledWindowPrivate
{
    QWidget* leftLayoutWidget_{nullptr};
    QWidget* rightLayoutWidget_{nullptr};
    QScreen* currentScreen_{nullptr};
    QPushButton* maximize_{nullptr};
    QPushButton* minimize_{nullptr};
    QPushButton* close_{nullptr};
    QPushButton* logo_{nullptr};
    QMenuBar* menuBar_{nullptr};
    QToolBar* windowHint_{nullptr};
    QLabel* titleLabel_{nullptr};
    QString windowTitle_{""};

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
    float displayScale_{1.f};

    bool initResize_{false};
};

StyledWindow::StyledWindow(QWidget* parent, Qt::WindowFlags f,
                           QString windowTitle)
    : QMainWindow(parent, f)
{
    d = new StyledWindowPrivate();
    this->setWindowTitle(windowTitle);
    init();
}

StyledWindow::~StyledWindow()
{
    delete d;
}

void StyledWindow::init()
{
    d->justMaximized_ = false;
    d->resizeable_ = true;
    d->displayScale_ = devicePixelRatioF();
#ifdef Q_OS_WIN
    d->titleBar_ = Q_NULLPTR;
    d->borderWidth_ = 5;
    setResizeableAreaWidth(8);
    Qt::WindowFlags flags;
    flags |= Qt::Window;
    flags |= Qt::FramelessWindowHint;
    flags |= Qt::WindowMinMaxButtonsHint;
    flags |= Qt::WindowCloseButtonHint;
    setWindowFlags(flags);

    enableAcrylicWindow(true);
#endif
    initWindowTitle();
}

void StyledWindow::setWindowTitle(QString title)
{
    if (title.length() > 0)
    {
#if defined(Q_OS_LINUX)
        QMainWindow::setWindowTitle(title);
#else
        d->titleLabel_->setText(title);
#endif
    }
}

void StyledWindow::setupMenuBar(QMenuBar* menuBar)
{
#ifdef Q_OS_WIN
    if (!d->leftLayoutWidget_)
    {
        return;
    }
    auto layout = qobject_cast<QHBoxLayout*>(d->leftLayoutWidget_->layout());
    if (menuBar && layout)
    {
        d->menuBar_ = menuBar;
        d->menuBar_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);
        d->menuBar_->setAttribute(Qt::WA_NoMousePropagation);
        d->menuBar_->setMouseTracking(false);
        layout->addWidget(d->menuBar_, 0, Qt::AlignLeft);
        this->setMenuBar(nullptr);
    }

#else
    this->setMenuBar(menuBar);
#endif
}

void StyledWindow::initWindowTitle()
{
    d->leftLayoutWidget_ = new QWidget();
    d->rightLayoutWidget_ = new QWidget();
    d->leftLayoutWidget_->setSizePolicy(QSizePolicy::Expanding,
                                        QSizePolicy::Expanding);
    d->rightLayoutWidget_->setSizePolicy(QSizePolicy::Expanding,
                                         QSizePolicy::Expanding);

    auto leftLayout = new QHBoxLayout(d->leftLayoutWidget_);
    leftLayout->setContentsMargins(8, 0, 0, 0);
    leftLayout->setAlignment(Qt::AlignLeft);

    auto rightLayout = new QHBoxLayout(d->rightLayoutWidget_);
    rightLayout->setSpacing(0);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setAlignment(Qt::AlignRight);
    rightLayout->setDirection(QBoxLayout::Direction::RightToLeft);

    d->windowHint_ = this->addToolBar(QObject::tr("Window Toolbar"));
    d->windowHint_->setProperty("class", "window-title-bar");
#ifdef Q_OS_WIN
#    ifdef ADS_EXPERIMENTAL_ACRYLIC_WINDOW
    d->windowHint_->setProperty("class", "window-title-bar-acrylic");
#    endif
#endif
    d->windowHint_->setMovable(false);
    d->windowHint_->setFloatable(false);
    d->windowHint_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    d->windowHint_->layout()->setMargin(0);
    d->windowHint_->layout()->setSpacing(0);

#ifdef Q_OS_WIN
    if (!d->logo_)
    {
        auto lumeIcon = QIcon(":/lume_icon.svg");
        d->logo_ = new QPushButton(lumeIcon, "", this);
        d->logo_->setFixedWidth(21);
        d->logo_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        d->logo_->setProperty("class", "menuWindowBt");
        leftLayout->addWidget(d->logo_, 0, Qt::AlignLeft);
        d->menuHelper_ = new WidgetEventHelper(this);
        d->menuHelper_->SetWidget(d->logo_);
    }
#endif

    d->windowHint_->addWidget(d->leftLayoutWidget_);

    d->titleLabel_ = new QLabel(this);
    d->titleLabel_->setText(d->windowTitle_);
    d->titleLabel_->setWordWrap(false);
    auto font = d->titleLabel_->font();
    font.setWeight(font.Bold);
    d->titleLabel_->setFont(font);
    d->titleLabel_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    d->titleLabel_->setMaximumWidth(292);  // Todo: Make sure we have a good
                                           // maximum width
    d->windowHint_->addWidget(d->titleLabel_);
#ifdef Q_OS_WIN
    d->divider_ = new QWidget(this);
    d->divider_->setProperty("class", "toolbar-divider");
    rightLayout->addWidget(d->divider_, 0, Qt::AlignRight | Qt::AlignVCenter);
#endif
    d->windowHint_->addWidget(d->rightLayoutWidget_);

    this->setProperty("class", "window-main");
    this->addToolBarBreak();
#ifdef Q_OS_WIN

    if (W_10)
    {
        if (!this->isMaximized())
        {
            this->setContentsMargins(
                QMargins(CUSTOM_FRAME_THICKNESS, CUSTOM_FRAME_THICKNESS,
                         CUSTOM_FRAME_THICKNESS, CUSTOM_FRAME_THICKNESS));
        }
        this->setProperty("class", "window-10-main");
    }

    setTitleBar(d->windowHint_);
    addIgnoreWidget(d->leftLayoutWidget_);
    addIgnoreWidget(d->rightLayoutWidget_);
    addIgnoreWidget(d->titleLabel_);
#endif
    setFocusProxy(d->windowHint_);
}

bool StyledWindow::event(QEvent* event)
{
    auto native = QMainWindow::event(event);
#ifdef Q_OS_WIN

    if (event->type() == QEvent::Move)
    {
        if (d->currentScreen_ == nullptr)
        {
            d->currentScreen_ = screen();
        }
        else if (d->currentScreen_ != screen())
        {
            d->currentScreen_ = screen();
            if (d->displayScale_ != devicePixelRatioF())
            {
                d->displayScale_ = devicePixelRatioF();

                // Hack: Force centralWidget to re-calculate size
                auto cw = centralWidget();
                if (cw)
                {
                    cw->adjustSize();
                    cw->updateGeometry();
                }
            }
            SetWindowPos((HWND)effectiveWinId(), NULL, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER
                             | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
                             | SWP_NOACTIVATE);
        }
    }

    if (event->type() == QEvent::Show)
    {
        setResizeable(d->resizeable_);
        constructHintButtons();
        if (!d->initResize_)
        {
            d->initResize_ = true;
            forceRedraw();
        }
    }

    if (event->type() == QEvent::Resize)
    {
        if (d->windowHint_)
        {
            d->windowHint_->resize(width() - d->frames_.right()
                                       - d->frames_.left(),
                                   d->windowHint_->height());
        }
    }

    if (event->type() == QEvent::WindowStateChange)
    {
        if (d->maximize_)
        {
            d->maximize_->setIcon(QIcon(!isMaximized() ?
                                            ":/icons/Icon_Maximize_Window.svg" :
                                            ":/icons/Icon_Restore_Window.svg")
                                      .pixmap(18, 18));
        }
    }
#endif
#ifdef Q_OS_MACOS
    auto type = event->type();
    if (event->type() == QEvent::WindowActivate)
    {
        // HideTitleBar(this);
    }
    if (event->type() == QEvent::Show)
    {
        this->setUnifiedTitleAndToolBarOnMac(true);
        HideTitleBar(this->winId());
    }
#endif
    return native;
}

QMenuBar* StyledWindow::menuBar()
{
#ifdef Q_OS_WIN

    return d->menuBar_;
#else
    return QMainWindow::menuBar();
#endif
}

void StyledWindow::setIcon(QIcon icon)
{
#ifdef Q_OS_WIN
    d->logo_ = new QPushButton(icon, "", this);
    d->logo_->setFixedWidth(21);
    d->logo_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    d->logo_->setProperty("class", "menuWindowBt");
#endif
}

void StyledWindow::setSubToolbar(QToolBar* toolbar)
{
    if (toolbar)
    {
        auto layout = qobject_cast<QHBoxLayout*>(d->rightLayoutWidget_->layout());
#ifdef Q_OS_WIN
        if (layout && d->divider_)
        {
            auto in = layout->indexOf(d->divider_);
            layout->insertWidget(in - 1, toolbar, 0,
                                 Qt::AlignRight | Qt::AlignVCenter);
        }
#else
        if (layout)
        {
            layout->insertWidget(0, toolbar, 0,
                                 Qt::AlignRight | Qt::AlignVCenter);
        }
#endif
    }
}

#ifdef Q_OS_WIN

void StyledWindow::setResizeable(bool resizeable)
{
    bool visible = isVisible();
    d->resizeable_ = resizeable;
    HWND hwnd = (HWND)this->winId();
    DWORD style = ::GetWindowLong(hwnd, GWL_STYLE) | WS_POPUP | WS_CAPTION;

    if (d->resizeable_)
    {
        style |= WS_THICKFRAME;
    }

    if (windowFlags() & Qt::WindowMaximizeButtonHint)
    {
        style |= WS_MAXIMIZEBOX;
    }

    if (windowFlags() & Qt::WindowMinimizeButtonHint)
    {
        style |= WS_MINIMIZEBOX;
    }

    ::SetWindowLong(hwnd, GWL_STYLE, style);

    const MARGINS shadow = {1, 1, 1, 1};
    DwmExtendFrameIntoClientArea(hwnd, &shadow);
    setVisible(visible);
}

bool StyledWindow::isResizeable()
{
    return d->resizeable_;
}

void StyledWindow::setResizeableAreaWidth(int width)
{
    if (1 > width)
        width = 1;
    d->borderWidth_ = width;
}

void StyledWindow::setTitleBar(QWidget* titlebar)
{
    d->titleBar_ = titlebar;
    if (!titlebar)
        return;
    connect(titlebar, SIGNAL(destroyed(QObject*)), this,
            SLOT(onTitleBarDestroyed()));
}

void StyledWindow::onTitleBarDestroyed()
{
    if (d->titleBar_ == QObject::sender())
    {
        d->titleBar_ = Q_NULLPTR;
    }
}

void StyledWindow::addIgnoreWidget(QWidget* widget)
{
    if (!widget)
        return;
    if (d->whiteList_.contains(widget))
        return;
    d->whiteList_.append(widget);
}

void StyledWindow::constructHintButtons()
{
    if (!d->rightLayoutWidget_)
    {
        return;
    }

    auto layout = qobject_cast<QHBoxLayout*>(d->rightLayoutWidget_->layout());
    int minimumWidth = 0;
    if (layout)
    {
        if (!d->minimizeHelper_)
        {
            d->minimizeHelper_ = new WidgetEventHelper(this);
        }
        if (windowFlags() & Qt::WindowMinimizeButtonHint)
        {
            if (!d->minimize_)
            {
                auto minimizeIcon = QIcon(":/icons/Icon_Minimize_Window.svg");
                d->minimize_ = new QPushButton(minimizeIcon, "", this);
                d->minimize_->setProperty("class", "minimizeWindowBt");
                d->minimize_->setSizePolicy(QSizePolicy::Fixed,
                                            QSizePolicy::Expanding);
                d->minimizeHelper_->SetWidget(d->minimize_);
                QObject::connect(d->minimize_, &QAbstractButton::released, this,
                                 [this]() {
                                     if (this->isOutOfWidget(d->minimize_))
                                     {
                                         return;
                                     }
                                     this->showMinimized();
                                 });
            }

            layout->insertWidget(0, d->minimize_, 0, Qt::AlignRight);
            minimumWidth += d->minimize_->width();
        }
        else
        {
            d->minimize_ = nullptr;
        }

        if (!d->maximizeHelper_)
        {
            d->maximizeHelper_ = new WidgetEventHelper(this);
        }
        if (windowFlags() & Qt::WindowMaximizeButtonHint)
        {
            if (!d->maximize_)
            {
                auto maximizeIcon = QIcon(this->isMaximized() ?
                                              ":/icons/Icon_Restore_Window.svg" :
                                              ":/icons/Icon_Maximize_Window.svg");
                d->maximize_ = new QPushButton(maximizeIcon, "", this);
                d->maximize_->setProperty("class", "maximizeWindowBt");
                d->maximize_->setSizePolicy(QSizePolicy::Fixed,
                                            QSizePolicy::Expanding);
                d->maximizeHelper_->SetWidget(d->maximize_);

                QObject::connect(
                    d->maximize_, &QAbstractButton::released, this, [this]() {
                        if (this->isOutOfWidget(d->maximize_))
                        {
                            return;
                        }
                        if (this->isMaximized())
                        {
                            if (W_10)
                            {
                                this->setContentsMargins(
                                    QMargins(CUSTOM_FRAME_THICKNESS,
                                             CUSTOM_FRAME_THICKNESS,
                                             CUSTOM_FRAME_THICKNESS,
                                             CUSTOM_FRAME_THICKNESS));
                            }
                            this->showNormal();
                        }
                        else
                        {
                            if (W_10)
                            {
                                this->setContentsMargins(QMargins());
                            }
                            this->showMaximized();
                        }
                    });
            }

            layout->insertWidget(0, d->maximize_, 0, Qt::AlignRight);
            minimumWidth += d->maximize_->width();
        }
        else
        {
            d->maximize_ = nullptr;
        }

        if (!d->closeHelper_)
        {
            d->closeHelper_ = new WidgetEventHelper(this);
        }
        if (windowFlags() & Qt::WindowCloseButtonHint)
        {
            if (!d->close_)
            {
                auto closeIcon = QIcon(":/icons/Icon_Close_Window.svg");
                d->close_ = new QPushButton(closeIcon, "", this);
                d->close_->setProperty("class", "closeWindowBt");
                d->close_->setSizePolicy(QSizePolicy::Fixed,
                                         QSizePolicy::Expanding);
                d->closeHelper_->SetWidget(d->close_);

                QObject::connect(d->close_, &QAbstractButton::released, this,
                                 [this]() {
                                     if (this->isOutOfWidget(d->close_))
                                     {
                                         return;
                                     }
                                     this->close();
                                 });
            }
            layout->insertWidget(0, d->close_, 0, Qt::AlignRight);
            minimumWidth += d->close_->width();
        }
        else
        {
            d->close_ = nullptr;
        }

        d->rightLayoutWidget_->setMinimumWidth(minimumWidth);
    }
}

void StyledWindow::setContentsMargins(const QMargins& margins)
{
    QMainWindow::setContentsMargins(margins + d->frames_);
    d->margins_ = margins;
}

void StyledWindow::setContentsMargins(int left, int top, int right, int bottom)
{
    QMainWindow::setContentsMargins(
        left + d->frames_.left(), top + d->frames_.top(),
        right + d->frames_.right(), bottom + d->frames_.bottom());
    d->margins_.setLeft(left);
    d->margins_.setTop(top);
    d->margins_.setRight(right);
    d->margins_.setBottom(bottom);
}

void StyledWindow::forceRedraw()
{
    // Enforce resize with small amount
    this->resize(this->size().width() - 1, this->size().height());
    this->resize(this->size().width() + 1, this->size().height());
}

bool StyledWindow::isOutOfWidget(QWidget* widget)
{
    auto rect = widget->rect();
    auto pos = widget->mapFromGlobal(QCursor::pos());
    if (pos.x() > rect.left() && pos.x() < rect.right() && pos.y() > rect.top()
        && pos.y() < rect.bottom())
    {
        return false;
    }

    widget->setAttribute(Qt::WA_UnderMouse, false);
    widget->update();
    return true;
}

QMenu* StyledWindow::createPopupMenu()
{
    return nullptr;
}

#    ifdef ADS_EXPERIMENTAL_ACRYLIC_WINDOW
// Experimental: Acrylic Background
typedef enum _WINDOWCOMPOSITIONATTRIB
{
    WCA_UNDEFINED = 0,
    WCA_NCRENDERING_ENABLED = 1,
    WCA_NCRENDERING_POLICY = 2,
    WCA_TRANSITIONS_FORCEDISABLED = 3,
    WCA_ALLOW_NCPAINT = 4,
    WCA_CAPTION_BUTTON_BOUNDS = 5,
    WCA_NONCLIENT_RTL_LAYOUT = 6,
    WCA_FORCE_ICONIC_REPRESENTATION = 7,
    WCA_EXTENDED_FRAME_BOUNDS = 8,
    WCA_HAS_ICONIC_BITMAP = 9,
    WCA_THEME_ATTRIBUTES = 10,
    WCA_NCRENDERING_EXILED = 11,
    WCA_NCADORNMENTINFO = 12,
    WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
    WCA_VIDEO_OVERLAY_ACTIVE = 14,
    WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
    WCA_DISALLOW_PEEK = 16,
    WCA_CLOAK = 17,
    WCA_CLOAKED = 18,
    WCA_ACCENT_POLICY = 19,
    WCA_FREEZE_REPRESENTATION = 20,
    WCA_EVER_UNCLOAKED = 21,
    WCA_VISUAL_OWNER = 22,
    WCA_HOLOGRAPHIC = 23,
    WCA_EXCLUDED_FROM_DDA = 24,
    WCA_PASSIVEUPDATEMODE = 25,
    WCA_LAST = 26
} WINDOWCOMPOSITIONATTRIB;

typedef struct _WINDOWCOMPOSITIONATTRIBDATA
{
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIBDATA;

typedef enum _ACCENT_STATE
{
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,  // RS4 1803
    ACCENT_ENABLE_HOSTBACKDROP = 5,       // RS5 1809
    ACCENT_INVALID_STATE = 6
} ACCENT_STATE;

typedef struct _ACCENT_POLICY
{
    ACCENT_STATE AccentState;
    DWORD AccentFlags;
    DWORD GradientColor;
    DWORD AnimationId;
} ACCENT_POLICY;

typedef BOOL(WINAPI* pfnGetWindowCompositionAttribute)(
    HWND, WINDOWCOMPOSITIONATTRIBDATA*);
typedef BOOL(WINAPI* pfnSetWindowCompositionAttribute)(
    HWND, WINDOWCOMPOSITIONATTRIBDATA*);
#    endif

void StyledWindow::enableAcrylicWindow(bool enable)
{
#    ifdef ADS_EXPERIMENTAL_ACRYLIC_WINDOW
    setAttribute(Qt::WA_TranslucentBackground, true);
    HWND hwnd = (HWND)this->winId();
    HMODULE hUser = GetModuleHandle("user32.dll");
    if (hUser)
    {
        pfnSetWindowCompositionAttribute setWindowCompositionAttribute =
            (pfnSetWindowCompositionAttribute)GetProcAddress(
                hUser, "SetWindowCompositionAttribute");

        if (setWindowCompositionAttribute)
        {
            ACCENT_POLICY accent = {enable ? ACCENT_ENABLE_ACRYLICBLURBEHIND :
                                             ACCENT_DISABLED,
                                    0, 0, 0};
            accent.GradientColor = 0x010000000;

            WINDOWCOMPOSITIONATTRIBDATA data{WCA_ACCENT_POLICY, &accent,
                                             sizeof(accent)};
            setWindowCompositionAttribute(hwnd, &data);
        }
    }
#    endif
}

bool StyledWindow::updateNativeWindowMargins(HWND hwnd, QMargins margins)
{
    MARGINS winMargins;

    winMargins.cxLeftWidth = margins.left();
    winMargins.cxRightWidth = margins.right();
    winMargins.cyBottomHeight = margins.bottom();
    winMargins.cyTopHeight = margins.top();

    auto hr = DwmExtendFrameIntoClientArea(hwnd, &winMargins);
    return SUCCEEDED(hr);
}

void StyledWindow::showFullScreen()
{
    if (isMaximized())
    {
        QMainWindow::setContentsMargins(d->margins_);
        d->frames_ = QMargins();
    }
    QMainWindow::showFullScreen();
}

#    ifndef WM_NCUAHDRAWCAPTION
#        define WM_NCUAHDRAWCAPTION (0x00AE)
#    endif

#    ifndef WM_NCUAHDRAWFRAME
#        define WM_NCUAHDRAWFRAME (0x00AF)
#    endif

bool StyledWindow::nativeEvent(const QByteArray& eventType, void* message,
                               long* result)
{
#    if (QT_VERSION == QT_VERSION_CHECK(5, 11, 1))
    MSG* msg = *reinterpret_cast<MSG**>(message);
#    else
    MSG* msg = reinterpret_cast<MSG*>(message);
#    endif

    switch (msg->message)
    {
    case WM_NCCALCSIZE:
    {
        // NCCALCSIZE_PARAMS& params =
        //     *reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
        // if (params.rgrc[0].top != 0)
        //     params.rgrc[0].top = 0;
        *result = WVR_REDRAW;
        return true;
    }

    case WM_NCHITTEST:
    {
        *result = 0;
        if (d->menuHelper_)
        {
            d->menuHelper_->HandleMouseMove();
        }

        if (d->minimizeHelper_)
        {
            d->minimizeHelper_->HandleMouseMove();
        }

        if (d->maximizeHelper_)
        {
            d->maximizeHelper_->HandleMouseMove();
        }

        if (d->closeHelper_)
        {
            d->closeHelper_->HandleMouseMove();
        }

        const LONG borderWidth = d->borderWidth_;
        RECT winrect;
        GetWindowRect(HWND(effectiveWinId()), &winrect);

        long x = GET_X_LPARAM(msg->lParam);
        long y = GET_Y_LPARAM(msg->lParam);

        if (d->resizeable_)
        {
            bool resizeWidth = minimumWidth() != maximumWidth();
            bool resizeHeight = minimumHeight() != maximumHeight();

            if (resizeWidth)
            {
                // left border
                if (x >= winrect.left && x < winrect.left + borderWidth)
                {
                    *result = HTLEFT;
                }
                // right border
                if (x < winrect.right && x >= winrect.right - borderWidth)
                {
                    *result = HTRIGHT;
                }
            }
            if (resizeHeight)
            {
                // bottom border
                if (y < winrect.bottom && y >= winrect.bottom - borderWidth)
                {
                    *result = HTBOTTOM;
                }
                // top border
                if (y >= winrect.top && y < winrect.top + borderWidth)
                {
                    *result = HTTOP;
                }
            }
            if (resizeWidth && resizeHeight)
            {
                // bottom left corner
                if (x >= winrect.left && x < winrect.left + borderWidth
                    && y < winrect.bottom && y >= winrect.bottom - borderWidth)
                {
                    *result = HTBOTTOMLEFT;
                }
                // bottom right corner
                if (x < winrect.right && x >= winrect.right - borderWidth
                    && y < winrect.bottom && y >= winrect.bottom - borderWidth)
                {
                    *result = HTBOTTOMRIGHT;
                }
                // top left corner
                if (x >= winrect.left && x < winrect.left + borderWidth
                    && y >= winrect.top && y < winrect.top + borderWidth)
                {
                    *result = HTTOPLEFT;
                }
                // top right corner
                if (x < winrect.right && x >= winrect.right - borderWidth
                    && y >= winrect.top && y < winrect.top + borderWidth)
                {
                    *result = HTTOPRIGHT;
                }
            }
        }
        if (0 != *result)
            return true;

        if (!d->titleBar_)
            return false;

        QPoint pos;
        // pos = d->titleBar_->mapFromGlobal(QPoint(x / dpr, y / dpr));
        pos = d->titleBar_->mapFromGlobal(QCursor::pos());
        if (isOutOfWidget(d->titleBar_))
            return false;

        QWidget* child = d->titleBar_->childAt(pos);
        if (!child)
        {
            *result = HTCAPTION;
            return true;
        }
        else
        {
            if (d->menuHelper_ && d->menuHelper_->Widget())
            {
                if (d->menuHelper_->Widget() == child)
                {
                    d->menuHelper_->SetWidgetRectFlag(true);
                    *result = HTSYSMENU;
                    return true;
                }
            }
            if (d->minimizeHelper_ && d->minimizeHelper_->Widget())
            {
                if (d->minimizeHelper_->Widget() == child)
                {
                    d->minimizeHelper_->SetWidgetRectFlag(true);
                    *result = HTMINBUTTON;
                    return true;
                }
            }
            if (d->maximizeHelper_ && d->maximizeHelper_->Widget())
            {
                if (d->maximizeHelper_->Widget() == child)
                {
                    d->maximizeHelper_->SetWidgetRectFlag(true);
                    *result = HTMAXBUTTON;
                    return true;
                }
            }
            if (d->closeHelper_ && d->closeHelper_->Widget())
            {
                if (d->closeHelper_->Widget() == child)
                {
                    d->closeHelper_->SetWidgetRectFlag(true);
                    *result = HTCLOSE;
                    return true;
                }
            }
            if (d->whiteList_.contains(child))
            {
                *result = HTCAPTION;
                return true;
            }
        }
        return false;
    }

    case WM_DPICHANGED:
    {
        auto scale = devicePixelRatioF();
        if (d->displayScale_ != scale)
        {
            d->displayScale_ = scale;
            if (d->rightLayoutWidget_)
            {
                auto minimumWidth = 0;
                if (d->close_)
                {
                    minimumWidth += d->close_->width();
                }
                if (d->maximize_)
                {
                    minimumWidth += d->maximize_->width();
                }
                if (d->minimize_)
                {
                    minimumWidth += d->minimize_->width();
                }
                d->rightLayoutWidget_->setMinimumWidth(minimumWidth);
            }
            adjustSize();
            update();
        }

        break;
    }

    case WM_SIZE:
    {
        if (msg->wParam == SIZE_RESTORED && d->justMinimized_)
        {
            d->justMinimized_ = false;
        }

        if (msg->wParam == SIZE_MINIMIZED)
        {
            d->justMinimized_ = true;
        }
        break;
    }

    case WM_GETMINMAXINFO:
    {
        if (::IsZoomed(msg->hwnd))
        {
            RECT frame = {0, 0, 0, 0};
            AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);

            double dpr = this->devicePixelRatioF();

            d->frames_.setLeft(abs(frame.left) / dpr + 0.5);
            d->frames_.setTop(abs(frame.bottom) / dpr + 0.5);
            d->frames_.setRight(abs(frame.right) / dpr + 0.5);
            d->frames_.setBottom(abs(frame.bottom) / dpr + 0.5);

            QMainWindow::setContentsMargins(
                d->frames_.left() + d->margins_.left(),
                d->frames_.top() + d->margins_.top(),
                d->frames_.right() + d->margins_.right(),
                d->frames_.bottom() + d->margins_.bottom());
            d->justMaximized_ = true;
        }
        else
        {
            if (d->justMaximized_)
            {
                QMainWindow::setContentsMargins(d->margins_);
                d->frames_ = QMargins();
                d->justMaximized_ = false;
            }
        }
        return false;
    }

    // case WM_ACTIVATE:
    // {
    //     switch (msg->wParam)
    //     {
    //     case WA_INACTIVE:
    //     {
    //         qDebug() << windowTitle() << ": WA_INACTIVE";
    //     }
    //     break;

    //     case WA_ACTIVE:
    //     default:
    //     {
    //         qDebug() << windowTitle() << ": WA_ACTIVE";
    //     }
    //     break;
    //     }
    //     return false;
    // }

    // Handle Mouse Event from native Q_OS_WIN and serve the gesture to Qt
    case WM_LBUTTONUP:
    {
        if (!(d->minimizeHelper_ && d->maximizeHelper_ && d->closeHelper_
              && d->menuHelper_))
        {
            return false;
        }

        d->menuHelper_->HandleMouseRelease(result, false);
        d->minimizeHelper_->HandleMouseRelease(result, false);
        d->maximizeHelper_->HandleMouseRelease(result, false);
        d->closeHelper_->HandleMouseRelease(result, false);
        return false;
    }

    case WM_NCMOUSELEAVE:
    {
        if (!(d->minimizeHelper_ && d->maximizeHelper_ && d->closeHelper_
              && d->menuHelper_))
        {
            return false;
        }

        d->menuHelper_->SetWidgetRectFlag(false);
        d->minimizeHelper_->SetWidgetRectFlag(false);
        d->maximizeHelper_->SetWidgetRectFlag(false);
        d->closeHelper_->SetWidgetRectFlag(false);
        d->menuHelper_->HandleMouseMove();
        d->minimizeHelper_->HandleMouseMove();
        d->maximizeHelper_->HandleMouseMove();
        d->closeHelper_->HandleMouseMove();

        break;
    }
    case WM_NCPAINT:
    {
        return false;
    }
    case WM_NCUAHDRAWCAPTION:
    case WM_NCUAHDRAWFRAME:
    {
        *result = 0;
        return true;
    }
    case WM_MOUSEMOVE:
    {
        if (!(d->minimizeHelper_ && d->maximizeHelper_ && d->closeHelper_
              && d->menuHelper_))
        {
            return false;
        }
        *result = 0;
        long x = GET_X_LPARAM(msg->lParam);
        long y = GET_Y_LPARAM(msg->lParam);
        if (d->menuHelper_->IsFirstMove())
        {
            d->menuHelper_->SetFirstMove(false);
            d->menuHelper_->SendMouseRelease(false);
        }
        if (d->minimizeHelper_->IsFirstMove())
        {
            d->minimizeHelper_->SetFirstMove(false);
            d->minimizeHelper_->SendMouseRelease(false);
        }
        if (d->maximizeHelper_->IsFirstMove())
        {
            d->maximizeHelper_->SetFirstMove(false);
            d->maximizeHelper_->SendMouseRelease(false);
        }
        if (d->closeHelper_->IsFirstMove())
        {
            d->closeHelper_->SetFirstMove(false);
            d->closeHelper_->SendMouseRelease(false);
        }

        d->menuHelper_->HandleMouseMove();
        d->minimizeHelper_->HandleMouseMove();
        d->maximizeHelper_->HandleMouseMove();
        d->closeHelper_->HandleMouseMove();

        if (!d->titleBar_)
            return false;

        QPoint pos;
        // pos = d->titleBar_->mapFromGlobal(QPoint(x / dpr, y / dpr));
        pos = d->titleBar_->mapFromGlobal(QCursor::pos());
        if (isOutOfWidget(d->titleBar_))
            return false;

        QWidget* child = d->titleBar_->childAt(pos);
        if (child)
        {
            if (d->whiteList_.contains(child))
            {
                *result = HTCAPTION;
                return true;
            }
            if (d->menuHelper_->Widget())
            {
                if (d->menuHelper_->Widget() == child)
                {
                    d->menuHelper_->SetWidgetRectFlag(true);
                }
            }
            if (d->minimizeHelper_->Widget())
            {
                if (d->minimizeHelper_->Widget() == child)
                {
                    d->minimizeHelper_->SetWidgetRectFlag(true);
                }
            }
            if (d->maximizeHelper_->Widget())
            {
                if (d->maximizeHelper_->Widget() == child)
                {
                    d->maximizeHelper_->SetWidgetRectFlag(true);
                }
            }
            if (d->closeHelper_->Widget())
            {
                if (d->closeHelper_->Widget() == child)
                {
                    d->closeHelper_->SetWidgetRectFlag(true);
                }
            }
        }
        return false;
    }

    case WM_NCLBUTTONDOWN:
    {
        if (!(d->minimizeHelper_ && d->maximizeHelper_ && d->closeHelper_
              && d->menuHelper_))
        {
            return false;
        }
        d->menuHelper_->HandleMouseMove();
        d->minimizeHelper_->HandleMouseMove();
        d->maximizeHelper_->HandleMouseMove();
        d->closeHelper_->HandleMouseMove();
        if (msg->wParam == HTMENU)
        {
            if (d->menuHelper_->HandleMousePress(result))
                return true;
        }
        else if (msg->wParam == HTMINBUTTON)
        {
            if (d->minimizeHelper_->HandleMousePress(result))
                return true;
        }
        else if (msg->wParam == HTMAXBUTTON)
        {
            if (d->maximizeHelper_->HandleMousePress(result))
                return true;
        }
        else if (msg->wParam == HTCLOSE)
        {
            if (d->closeHelper_->HandleMousePress(result))
                return true;
        }
        return false;
    }

    case WM_NCLBUTTONUP:
    {
        if (!(d->minimizeHelper_ && d->maximizeHelper_ && d->closeHelper_
              && d->menuHelper_))
        {
            return false;
        }
        if (msg->wParam == HTMENU)
        {
            if (d->menuHelper_->HandleMouseRelease(result))
                return true;
        }
        else if (msg->wParam == HTMINBUTTON)
        {
            if (d->minimizeHelper_->HandleMouseRelease(result))
                return true;
        }
        else if (msg->wParam == HTMAXBUTTON)
        {
            if (d->maximizeHelper_->HandleMouseRelease(result))
                return true;
        }
        else if (msg->wParam == HTCLOSE)
        {
            if (d->closeHelper_->HandleMouseRelease(result))
                return true;
        }

        d->menuHelper_->ReleaseFlag();
        d->minimizeHelper_->ReleaseFlag();
        d->maximizeHelper_->ReleaseFlag();
        d->closeHelper_->ReleaseFlag();
        return false;
    }

    case WM_STYLECHANGED:
    {
        if (msg->wParam == GWL_STYLE)
        {
            setResizeable(d->resizeable_);
            constructHintButtons();
        }
        break;
    }

    case WM_NCACTIVATE:
    {
        *result = DefWindowProcW(HWND(effectiveWinId()), WM_NCACTIVATE,
                                 msg->wParam, -1);
        break;
    }

    case WM_WINDOWPOSCHANGING:
    {
        const auto windowPos = reinterpret_cast<LPWINDOWPOS>(msg->lParam);
        windowPos->flags |= SWP_NOCOPYBITS;
    }
    break;

    default: break;
    }

    return false;
}
#endif
}  // namespace ads
