
#include "styled_window.h"

#include <QList>

#include "utils.h"

#ifdef WIN32
#    include "widget_event_helper.h"
#endif

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
    QMargins nativeMargins_;
    QMargins frames_;

    int borderWidth_{0};
    bool justMaximized_{false};
    bool justMinimized_{false};
    bool resizeable_{true};

    WidgetEventHelper* maximizeHelper_{nullptr};
    WidgetEventHelper* minimizeHelper_{nullptr};
    WidgetEventHelper* closeHelper_{nullptr};
    WidgetEventHelper* menuHelper_{nullptr};
    float displayScale_{1.f};
};

StyledWindow::StyledWindow(QWidget* parent, Qt::WindowFlags f,
                           QString windowTitle)
    : QMainWindow(parent, f)
{
    d = new StyledWindowPrivate();
#ifdef WIN32
    d->windowTitle_ = windowTitle;
    init();
#else
    setWindowTitle(windowTitle);
#endif
}

StyledWindow::~StyledWindow()
{
#ifdef WIN32
    d->minimizeHelper_ = nullptr;
    d->maximizeHelper_ = nullptr;
    d->closeHelper_ = nullptr;
#endif
}

void StyledWindow::init()
{
#ifdef WIN32
    d->titleBar_ = Q_NULLPTR;
    d->borderWidth_ = 5;
    d->justMaximized_ = false;
    d->resizeable_ = true;
    d->displayScale_ = devicePixelRatioF();

    setWindowFlags(windowFlags() | Qt::Window | Qt::FramelessWindowHint);
    setResizeableAreaWidth(8);
    setResizeable(d->resizeable_);
    initWindowTitle();
#endif
}

void StyledWindow::setWindowTitle(QString title)
{
#ifdef WIN32
    if (title.length() > 0)
    {
        d->titleLabel_->setText(title);
    }
#endif
    // the UI tests (project) are checking the qt window title
    // so change the actual Qt main window title too.
    QWidget::setWindowTitle(title);
}

void StyledWindow::setupMenuBar(QMenuBar* menuBar)
{
#ifdef WIN32
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
    if (this->menuBar())
    {
        this->setMenuBar(menuBar);
    }
#endif
}

QMenuBar* StyledWindow::menuBar()
{
    return d->menuBar_;
}

void StyledWindow::activateTitleBar(bool activated)
{
    if (!d->windowHint_)
    {
        return;
    }

    if (activated)
    {
        d->windowHint_->setStyleSheet("QToolBar {background-color: #0C0C0C;}");

        if (d->menuBar_)
        {
            d->menuBar_->setStyleSheet(R"(
                QMenuBar {background-color: #0C0C0C;} 
                QMenuBar::item:disabled {background-color: #0C0C0C;}
                )");
        }
    }
    else
    {
        d->windowHint_->setStyleSheet("QToolBar {background-color: #242424;}");

        if (d->menuBar_)
        {
            d->menuBar_->setStyleSheet(R"(
                QMenuBar {background-color: #242424;}
                QMenuBar::item:disabled {background-color: #242424;}
                )");
        }
    }
}

#ifdef WIN32

void StyledWindow::setResizeable(bool resizeable)
{
    bool visible = isVisible();
    d->resizeable_ = resizeable;
    HWND hwnd = (HWND)this->winId();
    DWORD style =
        (::GetWindowLong(hwnd, GWL_STYLE) | WS_POPUP | WS_CAPTION | WS_BORDER)
        & ~WS_MAXIMIZE;

    if (d->resizeable_)
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
            layout->addWidget(d->close_, 0, Qt::AlignRight);
            minimumWidth += d->close_->width();
        }
        else
        {
            d->close_ = nullptr;
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

            layout->addWidget(d->maximize_, 0, Qt::AlignRight);
            minimumWidth += d->maximize_->width();
        }
        else
        {
            d->maximize_ = nullptr;
        }

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

            layout->addWidget(d->minimize_, 0, Qt::AlignRight);
            minimumWidth += d->minimize_->width();
        }
        else
        {
            d->minimize_ = nullptr;
        }
        d->rightLayoutWidget_->setMinimumWidth(minimumWidth);
    }
}

void StyledWindow::initWindowTitle()
{
    d->leftLayoutWidget_ = new QWidget();
    d->rightLayoutWidget_ = new QWidget();
    d->leftLayoutWidget_->setSizePolicy(QSizePolicy::MinimumExpanding,
                                        QSizePolicy::Expanding);
    d->rightLayoutWidget_->setSizePolicy(QSizePolicy::MinimumExpanding,
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
    d->windowHint_->setMovable(false);
    d->windowHint_->setFloatable(false);
    d->windowHint_->setSizePolicy(QSizePolicy::MinimumExpanding,
                                  QSizePolicy::Fixed);
    d->windowHint_->layout()->setMargin(0);
    d->windowHint_->layout()->setSpacing(0);

    auto lumeIcon = QIcon(":/lume_icon.svg");
    d->logo_ = new QPushButton(lumeIcon, "", this);
    d->logo_->setFixedWidth(21);
    d->logo_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    d->logo_->setProperty("class", "menuWindowBt");
    leftLayout->addWidget(d->logo_, 0, Qt::AlignLeft);
    d->menuHelper_ = new WidgetEventHelper(this);
    d->menuHelper_->SetWidget(d->logo_);

    d->windowHint_->addWidget(d->leftLayoutWidget_);

    d->titleLabel_ = new QLabel(this);
    d->titleLabel_->setText(d->windowTitle_);
    d->titleLabel_->setWordWrap(false);
    auto font = d->titleLabel_->font();
    font.setWeight(font.Bold);
    d->titleLabel_->setFont(font);
    d->titleLabel_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    // d->titleLabel_->setMaximumWidth(292);  // Todo: Make sure we have a good
    //                                        // maximum width
    d->windowHint_->addWidget(d->titleLabel_);

    d->windowHint_->addWidget(d->rightLayoutWidget_);

    this->setAutoFillBackground(true);
    this->setProperty("class", "window-main");
    this->addToolBarBreak();

    if (W_10)
    {
        setContentsMargins(d->margins_ + CUSTOM_FRAME_THICKNESS);
        setStyleSheet(QString("QMainWindow ").append(CUSTOM_FRAME_STYLE_ENABLE));
    }

    addIgnoreWidget(d->leftLayoutWidget_);
    addIgnoreWidget(d->rightLayoutWidget_);
    setTitleBar(d->windowHint_);
    addIgnoreWidget(d->titleLabel_);
    setFocusProxy(d->windowHint_);
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

// Avoid artifacts when moving to another physical screen
void StyledWindow::moveEvent(QMoveEvent* event)
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
    if (d->titleBar_)
    {
        auto newHeight =
            qMax(TITLE_BAR_HEIGHT,
                 (int)(d->titleBar_->geometry().height() * d->displayScale_));

        if (newHeight != d->nativeMargins_.top())
        {
            d->nativeMargins_.setTop(newHeight);
            updateNativeWindowMargins(HWND(effectiveWinId()), d->nativeMargins_);
        }

        d->windowHint_->setMaximumWidth(this->width());
    }

    QMainWindow::resizeEvent(event);
}

bool StyledWindow::event(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        d->maximize_->setIcon(QIcon(!isMaximized() ?
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
        QMainWindow::setContentsMargins(d->margins_);
        d->frames_ = QMargins();
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
            if (d->whiteList_.contains(child))
            {
                *result = HTCAPTION;
                return true;
            }
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

            activateTitleBar();  // Hack: require to show activate color after
                                 // restoring window from minimized
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

    case WM_ACTIVATE:
    {
        auto newHeight =
            qMax(TITLE_BAR_HEIGHT,
                 (int)(d->titleBar_->geometry().height() * d->displayScale_));
        auto result = false;

        if (newHeight != d->nativeMargins_.top())
        {
            d->nativeMargins_.setTop(newHeight);
            result = updateNativeWindowMargins(HWND(effectiveWinId()),
                                               d->nativeMargins_);
        }
        return result;
    }

    // Handle Mouse Event from native win32 and serve the gesture to Qt
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
        activateTitleBar(msg->wParam);
    }

    default: break;
    }

    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif
}  // namespace ads