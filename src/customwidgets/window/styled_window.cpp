#include "styled_window.h"

#include "utils.h"

#ifdef WIN32
#    include "widget_event_helper.h"
#endif

namespace ads
{

StyledWindow::StyledWindow(QWidget* parent, Qt::WindowFlags f,
                           QString windowTitle)
    : QMainWindow(parent, f)
{
#ifdef WIN32
    windowTitle_ = windowTitle;
    init();
#else
    setWindowTitle(windowTitle);
#endif
}

StyledWindow::~StyledWindow()
{
#ifdef WIN32
    minimizeHelper_ = nullptr;
    maximizeHelper_ = nullptr;
    closeHelper_ = nullptr;
#endif
}

void StyledWindow::init()
{
#ifdef WIN32
    titleBar_ = Q_NULLPTR;
    borderWidth_ = 5;
    justMaximized_ = false;
    resizeable_ = true;
    displayScale_ = devicePixelRatioF();

    setWindowFlags(windowFlags() | Qt::Window | Qt::FramelessWindowHint);
    setResizeableAreaWidth(8);
    setResizeable(resizeable_);
    initWindowTitle();
#endif
}

void StyledWindow::setWindowTitle(QString title)
{
#ifdef WIN32
    if (title.length() > 0)
    {
        titleLabel_->setText(title);
    }
#endif
    // the UI tests (project) are checking the qt window title
    // so change the actual Qt main window title too.
    QWidget::setWindowTitle(title);
}

void StyledWindow::setupMenuBar(QMenuBar* menuBar)
{
#ifdef WIN32
    if (!leftLayoutWidget_)
    {
        return;
    }
    auto layout = qobject_cast<QHBoxLayout*>(leftLayoutWidget_->layout());
    if (menuBar && layout)
    {
        menuBar_ = menuBar;
        menuBar_->setSizePolicy(QSizePolicy::MinimumExpanding,
                                QSizePolicy::Maximum);
        menuBar_->setAttribute(Qt::WA_NoMousePropagation);
        menuBar_->setMouseTracking(false);
        layout->addWidget(menuBar_, 0, Qt::AlignLeft);
        this->setMenuBar(nullptr);
    }

#else
    if (this->menuBar())
    {
        this->setMenuBar(menuBar);
    }
#endif
}

#ifdef WIN32

void StyledWindow::setResizeable(bool resizeable)
{
    bool visible = isVisible();
    resizeable_ = resizeable;
    HWND hwnd = (HWND)this->winId();
    DWORD style =
        (::GetWindowLong(hwnd, GWL_STYLE) | WS_POPUP | WS_CAPTION | WS_BORDER)
        & ~WS_MAXIMIZE;

    if (resizeable_)
    {
        style = style | WS_THICKFRAME | WS_SIZEBOX;
    }
    else
    {
        style = style & ~WS_THICKFRAME & ~WS_SIZEBOX;
    }

    if (windowFlags() & Qt::WindowMaximizeButtonHint)
    {
        style = style | WS_MAXIMIZEBOX;
    }
    else
    {
        style = style & ~WS_MAXIMIZEBOX;
    }

    if (windowFlags() & Qt::WindowMinimizeButtonHint)
    {
        ::SetWindowLong(hwnd, GWL_STYLE, style | WS_MINIMIZEBOX);
    }
    else
    {
        ::SetWindowLong(hwnd, GWL_STYLE, style & ~WS_MINIMIZEBOX);
    }

    const MARGINS shadow = {1, 1, 1, 1};
    DwmExtendFrameIntoClientArea(HWND(winId()), &shadow);
    setVisible(visible);
}

bool StyledWindow::isResizeable()
{
    return resizeable_;
}

void StyledWindow::setResizeableAreaWidth(int width)
{
    if (1 > width)
        width = 1;
    borderWidth_ = width;
}

void StyledWindow::setTitleBar(QWidget* titlebar)
{
    titleBar_ = titlebar;
    if (!titlebar)
        return;
    connect(titlebar, SIGNAL(destroyed(QObject*)), this,
            SLOT(onTitleBarDestroyed()));
}

void StyledWindow::onTitleBarDestroyed()
{
    if (titleBar_ == QObject::sender())
    {
        titleBar_ = Q_NULLPTR;
    }
}

void StyledWindow::addIgnoreWidget(QWidget* widget)
{
    if (!widget)
        return;
    if (whiteList_.contains(widget))
        return;
    whiteList_.append(widget);
}

void StyledWindow::constructHintButtons()
{
    if (!rightLayoutWidget_)
    {
        return;
    }

    auto layout = qobject_cast<QHBoxLayout*>(rightLayoutWidget_->layout());
    int minimumWidth = 0;
    if (layout)
    {
        closeHelper_ = new WidgetEventHelper(this);
        if (windowFlags() & Qt::WindowCloseButtonHint)
        {
            if (!close_)
            {
                auto closeIcon = QIcon(":/icons/Icon_Close_Window.svg");
                close_ = new QPushButton(closeIcon, "", this);
                close_->setProperty("class", "closeWindowBt");
                close_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
                closeHelper_->SetWidget(close_);

                QObject::connect(close_, &QAbstractButton::released, this,
                                 [this]() {
                                     if (this->isOutOfWidget(close_))
                                     {
                                         return;
                                     }
                                     this->close();
                                 });
            }
            layout->addWidget(close_, 0, Qt::AlignRight);
            minimumWidth += close_->width();
        }

        maximizeHelper_ = new WidgetEventHelper(this);
        if (windowFlags() & Qt::WindowMaximizeButtonHint)
        {
            if (!maximize_)
            {
                auto maximizeIcon = QIcon(this->isMaximized() ?
                                              ":/icons/Icon_Restore_Window.svg" :
                                              ":/icons/Icon_Maximize_Window.svg");
                maximize_ = new QPushButton(maximizeIcon, "", this);
                maximize_->setProperty("class", "maximizeWindowBt");
                maximize_->setSizePolicy(QSizePolicy::Fixed,
                                         QSizePolicy::Expanding);
                maximizeHelper_->SetWidget(maximize_);

                QObject::connect(
                    maximize_, &QAbstractButton::released, this, [this]() {
                        if (this->isOutOfWidget(maximize_))
                        {
                            return;
                        }
                        if (this->isMaximized())
                        {
                            this->showNormal();
                            if (W_10)
                            {
                                this->setStyleSheet(
                                    QString("QMainWindow ")
                                        .append(CUSTOM_FRAME_STYLE_ENABLE));
                            }
                        }
                        else
                        {
                            this->showMaximized();
                            if (W_10)
                            {
                                this->setStyleSheet(
                                    QString("QMainWindow ")
                                        .append(CUSTOM_FRAME_STYLE_DISABLE));
                            }
                        }
                    });
            }

            layout->addWidget(maximize_, 0, Qt::AlignRight);
            minimumWidth += maximize_->width();
        }

        minimizeHelper_ = new WidgetEventHelper(this);
        if (windowFlags() & Qt::WindowMinimizeButtonHint)
        {
            if (!minimize_)
            {
                auto minimizeIcon = QIcon(":/icons/Icon_Minimize_Window.svg");
                minimize_ = new QPushButton(minimizeIcon, "", this);
                minimize_->setProperty("class", "minimizeWindowBt");
                minimize_->setSizePolicy(QSizePolicy::Fixed,
                                         QSizePolicy::Expanding);
                minimizeHelper_->SetWidget(minimize_);
                QObject::connect(minimize_, &QAbstractButton::released, this,
                                 [this]() {
                                     if (this->isOutOfWidget(minimize_))
                                     {
                                         return;
                                     }
                                     this->showMinimized();
                                 });
            }

            layout->addWidget(minimize_, 0, Qt::AlignRight);
            minimumWidth += minimize_->width();
        }
        rightLayoutWidget_->setMinimumWidth(minimumWidth);
    }
}

void StyledWindow::initWindowTitle()
{
    leftLayoutWidget_ = new QWidget();
    rightLayoutWidget_ = new QWidget();
    leftLayoutWidget_->setSizePolicy(QSizePolicy::Expanding,
                                     QSizePolicy::Expanding);
    rightLayoutWidget_->setSizePolicy(QSizePolicy::Expanding,
                                      QSizePolicy::Expanding);

    auto leftLayout = new QHBoxLayout(leftLayoutWidget_);
    leftLayout->setContentsMargins(8, 0, 0, 0);
    leftLayout->setAlignment(Qt::AlignLeft);

    auto rightLayout = new QHBoxLayout(rightLayoutWidget_);
    rightLayout->setSpacing(0);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setAlignment(Qt::AlignRight);
    rightLayout->setDirection(QBoxLayout::Direction::RightToLeft);

    windowHint_ = this->addToolBar(QObject::tr("Window Toolbar"));
    windowHint_->setProperty("class", "window-title-bar");
    windowHint_->setMovable(false);
    windowHint_->setFloatable(false);
    windowHint_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    windowHint_->layout()->setMargin(0);
    windowHint_->layout()->setSpacing(0);

    auto lumeIcon = QIcon(":/lume_icon.svg");
    logo_ = new QPushButton(lumeIcon, "", this);
    logo_->setFixedWidth(21);
    logo_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    logo_->setProperty("class", "menuWindowBt");
    leftLayout->addWidget(logo_, 0, Qt::AlignLeft);
    menuHelper_ = new WidgetEventHelper(this);
    menuHelper_->SetWidget(logo_);

    windowHint_->addWidget(leftLayoutWidget_);

    titleLabel_ = new QLabel(this);
    titleLabel_->setWordWrap(false);
    titleLabel_->setText(windowTitle_);
    auto font = titleLabel_->font();
    font.setWeight(font.Bold);
    titleLabel_->setFont(font);
    titleLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    windowHint_->addWidget(titleLabel_);

    windowHint_->addWidget(rightLayoutWidget_);

    this->setAutoFillBackground(true);
    this->setProperty("class", "window-main");
    this->addToolBarBreak();

    if (W_10)
    {
        setContentsMargins(margins_ + CUSTOM_FRAME_THICKNESS);
        setStyleSheet(QString("QMainWindow ").append(CUSTOM_FRAME_STYLE_ENABLE));
    }

    addIgnoreWidget(leftLayoutWidget_);
    addIgnoreWidget(rightLayoutWidget_);
    setTitleBar(windowHint_);
    addIgnoreWidget(titleLabel_);
    setFocusProxy(windowHint_);
}

void StyledWindow::setContentsMargins(const QMargins& margins)
{
    QMainWindow::setContentsMargins(margins + frames_);
    margins_ = margins;
}

void StyledWindow::setContentsMargins(int left, int top, int right, int bottom)
{
    QMainWindow::setContentsMargins(left + frames_.left(), top + frames_.top(),
                                    right + frames_.right(),
                                    bottom + frames_.bottom());
    margins_.setLeft(left);
    margins_.setTop(top);
    margins_.setRight(right);
    margins_.setBottom(bottom);
}

// Avoid artifacts when moving to another physical screen
void StyledWindow::moveEvent(QMoveEvent* event)
{
    if (currentScreen_ == nullptr)
    {
        currentScreen_ = screen();
    }
    else if (currentScreen_ != screen())
    {
        currentScreen_ = screen();
        if (displayScale_ != devicePixelRatioF())
        {
            displayScale_ = devicePixelRatioF();

            // Hack: Force centralWidget to re-calculate size
            auto cw = centralWidget();
            if (cw)
            {
                cw->adjustSize();
                cw->updateGeometry();
            }
        }

        SetWindowPos((HWND)winId(), NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER
                         | SWP_FRAMECHANGED | SWP_NOACTIVATE);
    }

    QMainWindow::moveEvent(event);
}

void StyledWindow::showEvent(QShowEvent* event)
{
    constructHintButtons();
    QMainWindow::showEvent(event);
}

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

void StyledWindow::resizeEvent(QResizeEvent* event)
{
    if (titleBar_)
    {
        auto newHeight =
            qMax(TITLE_BAR_HEIGHT,
                 (int)(titleBar_->geometry().height() * displayScale_));

        if (newHeight != nativeMargins_.top())
        {
            nativeMargins_.setTop(newHeight);
            updateNativeWindowMargins(HWND(effectiveWinId()), nativeMargins_);
        }

        windowHint_->setMaximumWidth(this->geometry().width());
    }

    QMainWindow::resizeEvent(event);
}

bool StyledWindow::event(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        maximize_->setIcon(QIcon(!isMaximized() ?
                                     ":/icons/Icon_Maximize_Window.svg" :
                                     ":/icons/Icon_Restore_Window.svg")
                               .pixmap(18, 18));
    }

    return QMainWindow::event(event);
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

void StyledWindow::showFullScreen()
{
    if (isMaximized())
    {
        QMainWindow::setContentsMargins(margins_);
        frames_ = QMargins();
    }
    QMainWindow::showFullScreen();
}

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
        NCCALCSIZE_PARAMS& params =
            *reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
        if (params.rgrc[0].top != 0)
            --params.rgrc[0].top;
        *result = WVR_REDRAW;
        return true;
    }

    case WM_NCHITTEST:
    {
        *result = 0;
        if (menuHelper_)
        {
            menuHelper_->HandleMouseMove();
        }

        if (minimizeHelper_)
        {
            minimizeHelper_->HandleMouseMove();
        }

        if (maximizeHelper_)
        {
            maximizeHelper_->HandleMouseMove();
        }

        if (closeHelper_)
        {
            closeHelper_->HandleMouseMove();
        }

        const LONG borderWidth = borderWidth_;
        RECT winrect;
        GetWindowRect(HWND(effectiveWinId()), &winrect);

        long x = GET_X_LPARAM(msg->lParam);
        long y = GET_Y_LPARAM(msg->lParam);

        if (resizeable_)
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

        if (!titleBar_)
            return false;

        QPoint pos;
        // pos = titleBar_->mapFromGlobal(QPoint(x / dpr, y / dpr));
        pos = titleBar_->mapFromGlobal(QCursor::pos());
        if (isOutOfWidget(titleBar_))
            return false;

        QWidget* child = titleBar_->childAt(pos);
        if (!child)
        {
            *result = HTCAPTION;
            return true;
        }
        else
        {
            if (whiteList_.contains(child))
            {
                *result = HTCAPTION;
                return true;
            }
            if (menuHelper_ && menuHelper_->Widget())
            {
                if (menuHelper_->Widget() == child)
                {
                    menuHelper_->SetWidgetRectFlag(true);
                    *result = HTSYSMENU;
                    return true;
                }
            }
            if (minimizeHelper_ && minimizeHelper_->Widget())
            {
                if (minimizeHelper_->Widget() == child)
                {
                    minimizeHelper_->SetWidgetRectFlag(true);
                    *result = HTMINBUTTON;
                    return true;
                }
            }
            if (maximizeHelper_ && maximizeHelper_->Widget())
            {
                if (maximizeHelper_->Widget() == child)
                {
                    maximizeHelper_->SetWidgetRectFlag(true);
                    *result = HTMAXBUTTON;
                    return true;
                }
            }
            if (closeHelper_ && closeHelper_->Widget())
            {
                if (closeHelper_->Widget() == child)
                {
                    closeHelper_->SetWidgetRectFlag(true);
                    *result = HTCLOSE;
                    return true;
                }
            }
        }
        return false;
    }

    case WM_DPICHANGED:
    {
        auto scale = devicePixelRatioF();
        if (displayScale_ != scale)
        {
            displayScale_ = scale;
            if (rightLayoutWidget_)
            {
                auto minimumWidth = 0;
                if (close_)
                {
                    minimumWidth += close_->width();
                }
                if (maximize_)
                {
                    minimumWidth += maximize_->width();
                }
                if (minimize_)
                {
                    minimumWidth += minimize_->width();
                }
                rightLayoutWidget_->setMinimumWidth(minimumWidth);
            }
            adjustSize();
            update();
        }

        return QMainWindow::nativeEvent(eventType, message, result);
    }

    case WM_SIZE:
    {
        if (msg->wParam == SIZE_RESTORED && justMinimized_)
        {
            justMinimized_ = false;
        }

        if (msg->wParam == SIZE_MINIMIZED)
        {
            justMinimized_ = true;
        }
        return QMainWindow::nativeEvent(eventType, message, result);
    }

    case WM_GETMINMAXINFO:
    {
        if (::IsZoomed(msg->hwnd))
        {
            RECT frame = {0, 0, 0, 0};
            AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);

            double dpr = this->devicePixelRatioF();

            frames_.setLeft(abs(frame.left) / dpr + 0.5);
            frames_.setTop(abs(frame.bottom) / dpr + 0.5);
            frames_.setRight(abs(frame.right) / dpr + 0.5);
            frames_.setBottom(abs(frame.bottom) / dpr + 0.5);

            QMainWindow::setContentsMargins(frames_.left() + margins_.left(),
                                            frames_.top() + margins_.top(),
                                            frames_.right() + margins_.right(),
                                            frames_.bottom() + margins_.bottom());
            justMaximized_ = true;
        }
        else
        {
            if (justMaximized_)
            {
                QMainWindow::setContentsMargins(margins_);
                frames_ = QMargins();
                justMaximized_ = false;
            }
        }
        return false;
    }

    case WM_ACTIVATE:
    {
        auto newHeight =
            qMax(TITLE_BAR_HEIGHT,
                 (int)(titleBar_->geometry().height() * displayScale_));
        auto result = false;

        if (newHeight != nativeMargins_.top())
        {
            nativeMargins_.setTop(newHeight);
            result =
                updateNativeWindowMargins(HWND(effectiveWinId()), nativeMargins_);
        }
        return result;
    }

    // Handle Mouse Event from native win32 and serve the gesture to Qt
    case WM_LBUTTONUP:
    {
        if (!(minimizeHelper_ && maximizeHelper_ && closeHelper_ && menuHelper_))
        {
            return false;
        }

        menuHelper_->HandleMouseRelease(result, false);
        minimizeHelper_->HandleMouseRelease(result, false);
        maximizeHelper_->HandleMouseRelease(result, false);
        closeHelper_->HandleMouseRelease(result, false);
        return false;
    }

    case WM_NCMOUSELEAVE:
    {
        if (!(minimizeHelper_ && maximizeHelper_ && closeHelper_ && menuHelper_))
        {
            return false;
        }

        menuHelper_->SetWidgetRectFlag(false);
        minimizeHelper_->SetWidgetRectFlag(false);
        maximizeHelper_->SetWidgetRectFlag(false);
        closeHelper_->SetWidgetRectFlag(false);
        menuHelper_->HandleMouseMove();
        minimizeHelper_->HandleMouseMove();
        maximizeHelper_->HandleMouseMove();
        closeHelper_->HandleMouseMove();

        return QMainWindow::nativeEvent(eventType, message, result);
    }

    case WM_MOUSEMOVE:
    {
        if (!(minimizeHelper_ && maximizeHelper_ && closeHelper_ && menuHelper_))
        {
            return false;
        }
        *result = 0;
        long x = GET_X_LPARAM(msg->lParam);
        long y = GET_Y_LPARAM(msg->lParam);
        if (menuHelper_->IsFirstMove())
        {
            menuHelper_->SetFirstMove(false);
            menuHelper_->SendMouseRelease(false);
        }
        if (minimizeHelper_->IsFirstMove())
        {
            minimizeHelper_->SetFirstMove(false);
            minimizeHelper_->SendMouseRelease(false);
        }
        if (maximizeHelper_->IsFirstMove())
        {
            maximizeHelper_->SetFirstMove(false);
            maximizeHelper_->SendMouseRelease(false);
        }
        if (closeHelper_->IsFirstMove())
        {
            closeHelper_->SetFirstMove(false);
            closeHelper_->SendMouseRelease(false);
        }

        menuHelper_->HandleMouseMove();
        minimizeHelper_->HandleMouseMove();
        maximizeHelper_->HandleMouseMove();
        closeHelper_->HandleMouseMove();

        if (!titleBar_)
            return false;

        QPoint pos;
        // pos = titleBar_->mapFromGlobal(QPoint(x / dpr, y / dpr));
        pos = titleBar_->mapFromGlobal(QCursor::pos());
        if (isOutOfWidget(titleBar_))
            return false;

        QWidget* child = titleBar_->childAt(pos);
        if (child)
        {
            if (whiteList_.contains(child))
            {
                *result = HTCAPTION;
                return true;
            }
            if (menuHelper_->Widget())
            {
                if (menuHelper_->Widget() == child)
                {
                    menuHelper_->SetWidgetRectFlag(true);
                }
            }
            if (minimizeHelper_->Widget())
            {
                if (minimizeHelper_->Widget() == child)
                {
                    minimizeHelper_->SetWidgetRectFlag(true);
                }
            }
            if (maximizeHelper_->Widget())
            {
                if (maximizeHelper_->Widget() == child)
                {
                    maximizeHelper_->SetWidgetRectFlag(true);
                }
            }
            if (closeHelper_->Widget())
            {
                if (closeHelper_->Widget() == child)
                {
                    closeHelper_->SetWidgetRectFlag(true);
                }
            }
        }
        return false;
    }

    case WM_NCLBUTTONDOWN:
    {
        if (!(minimizeHelper_ && maximizeHelper_ && closeHelper_ && menuHelper_))
        {
            return false;
        }
        menuHelper_->HandleMouseMove();
        minimizeHelper_->HandleMouseMove();
        maximizeHelper_->HandleMouseMove();
        closeHelper_->HandleMouseMove();
        if (msg->wParam == HTMENU)
        {
            if (menuHelper_->HandleMousePress(result))
                return true;
        }
        else if (msg->wParam == HTMINBUTTON)
        {
            if (minimizeHelper_->HandleMousePress(result))
                return true;
        }
        else if (msg->wParam == HTMAXBUTTON)
        {
            if (maximizeHelper_->HandleMousePress(result))
                return true;
        }
        else if (msg->wParam == HTCLOSE)
        {
            if (closeHelper_->HandleMousePress(result))
                return true;
        }
        return false;
    }

    case WM_NCLBUTTONUP:
    {
        if (!(minimizeHelper_ && maximizeHelper_ && closeHelper_ && menuHelper_))
        {
            return false;
        }
        if (msg->wParam == HTMENU)
        {
            if (menuHelper_->HandleMouseRelease(result))
                return true;
        }
        else if (msg->wParam == HTMINBUTTON)
        {
            if (minimizeHelper_->HandleMouseRelease(result))
                return true;
        }
        else if (msg->wParam == HTMAXBUTTON)
        {
            if (maximizeHelper_->HandleMouseRelease(result))
                return true;
        }
        else if (msg->wParam == HTCLOSE)
        {
            if (closeHelper_->HandleMouseRelease(result))
                return true;
        }

        menuHelper_->ReleaseFlag();
        minimizeHelper_->ReleaseFlag();
        maximizeHelper_->ReleaseFlag();
        closeHelper_->ReleaseFlag();
        return false;
    }

    default: return QMainWindow::nativeEvent(eventType, message, result);
    }
}
#endif
}  // namespace ads