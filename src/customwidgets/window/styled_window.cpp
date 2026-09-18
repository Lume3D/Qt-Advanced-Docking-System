#include <QCoreApplication>
#include <QLayout>

#include "styled_window_p.h"

#ifdef Q_OS_MACOS
#    include "macos_helper.h"
#endif

namespace ads
{

StyledWindow::StyledWindow(QWidget* parent, Qt::WindowFlags f,
                           QString windowTitle)
    : QMainWindow(parent, f)
{
    d = new StyledWindowPrivate();
    QMainWindow::setWindowTitle(windowTitle);
    init();
}

StyledWindow::~StyledWindow()
{
#ifdef Q_OS_WIN
    if (d->suspendResumeNotification_)
    {
        UnregisterSuspendResumeNotification(d->suspendResumeNotification_);
    }
    if (d->backgroundBrush_)
    {
        DeleteObject(d->backgroundBrush_);
    }
    delete d->proxyWindow_;
#endif
    delete d;
}

void StyledWindow::init()
{
    d->justMaximized_ = false;
    d->resizeable_ = true;
    d->displayScale_ = devicePixelRatioF();

#ifdef Q_OS_WIN
    QTimer::singleShot(0, [this]() {
        d->proxyWindow_ = new QWindow();
        d->proxyWindow_->setBaseSize({0, 0});
        d->sysMenu_ = GetSystemMenu((HWND)d->proxyWindow_->winId(), FALSE);
    });
    d->titleBar_ = Q_NULLPTR;
    setResizeableAreaWidth(8);

    QObject::connect(
        qApp, &QApplication::focusChanged, this,
        [this](QWidget*, QWidget* now) { rememberFocusedWidget(now); });
#endif

    Qt::WindowFlags flags;
    flags |= Qt::Window;
    flags |= Qt::WindowMinMaxButtonsHint;
    flags |= Qt::WindowCloseButtonHint;
#ifdef Q_OS_WIN
    flags |= Qt::FramelessWindowHint;
#elif defined(__APPLE__)
    flags |= Qt::WindowFullscreenButtonHint;
    flags |= Qt::CustomizeWindowHint;

#    if QT_VERSION_MAJOR >= 6
    this->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
#    endif
#endif

    setWindowFlags(flags);
    initWindowTitle();
    this->setAttribute(Qt::WA_TranslucentBackground);
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
bool StyledWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (qobject_cast<QToolBar*>(watched))
    {
#ifdef __APPLE__
        if (event->type() == QEvent::MouseButtonPress)
        {
            auto* e = static_cast<QMouseEvent*>(event);
            if (e && e->button() == Qt::LeftButton)
            {
                auto nonClientEvent = QMouseEvent(
                    QEvent::NonClientAreaMouseButtonPress, e->localPos(),
                    e->button(), e->buttons(), e->modifiers());
                QApplication::sendEvent(this, &nonClientEvent);
            }
            if (e && !this->isActiveWindow())
            {
                auto event = QEvent(QEvent::WindowActivate);
                QApplication::sendEvent(this, &event);
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            auto* e = static_cast<QMouseEvent*>(event);
            if (e && e->button() == Qt::LeftButton)
            {
                auto nonClientEvent =
                    QEvent(QEvent::NonClientAreaMouseButtonRelease);
                QApplication::sendEvent(this, &nonClientEvent);
            }
        }
        else if (event->type() == QEvent::MouseButtonDblClick)
        {
            auto* e = static_cast<QMouseEvent*>(event);
            if (e && e->button() == Qt::LeftButton)
            {
                auto nonClientEvent =
                    QEvent(QEvent::NonClientAreaMouseButtonDblClick);
                QApplication::sendEvent(this, &nonClientEvent);
                if (isMaximized())
                {
                    showNormal();
                }
                else
                {
                    showMaximized();
                }
            }
        }
#elif defined(_WIN32)
        if (event->type() == QEvent::Resize)
        {
            updateWindowFrameAttributes();

            auto* widget = qobject_cast<QToolBar*>(watched);
            const auto titleBarHeight = static_cast<int>(
                widget->size().height() * d->displayScale_ + 0.5f);
            MARGINS m = {0, 0, titleBarHeight, 0};
            DwmExtendFrameIntoClientArea((HWND)this->effectiveWinId(), &m);
        }
#endif
    }

    return QMainWindow::eventFilter(watched, event);
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
    d->windowHint_->window()->setContextMenuPolicy(Qt::NoContextMenu);
    d->windowHint_->installEventFilter(this);
#ifdef Q_OS_WIN
#    ifdef ADS_EXPERIMENTAL_ACRYLIC_WINDOW
    d->windowHint_->setProperty("class", "window-title-bar-acrylic");
#    endif
#endif
    d->windowHint_->setMovable(false);
    d->windowHint_->setFloatable(false);
    d->windowHint_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    d->windowHint_->layout()->setContentsMargins(0, 0, 0, 0);
    d->windowHint_->layout()->setSpacing(0);
    d->windowHint_->setWindowFlags(Qt::WindowTitleHint);
#ifdef Q_OS_WIN
    if (!d->logo_)
    {
        auto lumeIcon = QIcon(":/lume_icon.svg");
        setIcon(lumeIcon);
        leftLayout->addWidget(d->logo_, 0, Qt::AlignLeft);
        d->menuHelper_ = new WidgetEventHelper(this);
        d->menuHelper_->SetWidget(d->logo_);
    }
#endif

    d->windowHint_->addWidget(d->leftLayoutWidget_);

    d->titleLabel_ = new QLabel(this);
    d->titleLabel_->setText(this->windowTitle());
    d->titleLabel_->setWordWrap(false);
    auto font = d->titleLabel_->font();
    font.setWeight(font.Bold);
    d->titleLabel_->setFont(font);
    d->titleLabel_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    connect(this, &QMainWindow::windowTitleChanged, this,
            [this](QString title) { this->d->titleLabel_->setText(title); });

    d->windowHint_->addWidget(d->titleLabel_);
#ifdef Q_OS_WIN
    d->divider_ = new QWidget(this);
    d->divider_->setProperty("class", "toolbar-divider");
    rightLayout->addWidget(d->divider_, 0, Qt::AlignRight | Qt::AlignVCenter);
    d->divider_->setVisible(false);
#endif
    d->windowHint_->addWidget(d->rightLayoutWidget_);

    this->setProperty("class", "window-main");
    this->addToolBarBreak();
#ifdef Q_OS_WIN

    if (W_10)
    {
        if (!this->isMaximized())
        {
            this->setContentsMargins(QMargins(FRAME_THICKNESS, FRAME_THICKNESS,
                                              FRAME_THICKNESS, FRAME_THICKNESS));
        }
        this->setProperty("class", "window-10-main");
    }

    setTitleBar(d->windowHint_);
    addIgnoreWidget(d->leftLayoutWidget_);
    addIgnoreWidget(d->rightLayoutWidget_);
    addIgnoreWidget(d->titleLabel_);
#endif
}

bool StyledWindow::event(QEvent* event)
{
    auto native = QMainWindow::event(event);
#ifdef Q_OS_WIN
    if (event->type() == QEvent::Paint)
    {
        d->pendingStateResizePaint_ = false;

        if (!d->cloakPending_)
        {
            return native;
        }

        if (!d->uncloakQueued_)
        {
            d->uncloakQueued_ = true;
            QMetaObject::invokeMethod(
                this,
                [this]() {
                    d->uncloakQueued_ = false;
                    if (!d->cloakPending_ || !isVisible() || isMinimized())
                    {
                        return;
                    }

                    d->cloakPending_ = false;
                    setWindowCloaked(false);
                },
                Qt::QueuedConnection);
        }
    }
    if (event->type() == QEvent::ScreenChangeInternal)
    {
        RECT rect;
        const auto hwnd = reinterpret_cast<HWND>(this->effectiveWinId());
        GetWindowRect(hwnd, &rect);
        const auto dpr = nativeWindowDpr(hwnd, this->devicePixelRatioF());
        if (dpr != d->displayScale_)
        {
            updateWindowDpr(dpr,
                            QRect(rect.left, rect.top, rect.right - rect.left,
                                  rect.bottom - rect.top),
                            this->effectiveWinId());
        }
    }
    if (event->type() == QEvent::Resize)
    {
        syncWindowHintGeometry();
    }
    if (event->type() == QEvent::Show)
    {
        if (!d->initResize_)
        {
            d->initResize_ = true;

            RECT rect;
            const auto hwnd = reinterpret_cast<HWND>(this->effectiveWinId());
            GetWindowRect(hwnd, &rect);
            updateWindowDpr(nativeWindowDpr(hwnd, this->devicePixelRatioF()),
                            QRect(rect.left, rect.top, rect.right - rect.left,
                                  rect.bottom - rect.top),
                            this->effectiveWinId());

            setResizeable(d->resizeable_);
            constructHintButtons();
            // Register for receiving WM_POWERBROADCAST event
            d->suspendResumeNotification_ = RegisterSuspendResumeNotification(
                reinterpret_cast<HANDLE>(this->winId()),
                DEVICE_NOTIFY_WINDOW_HANDLE);

            auto style = GetClassLong((HWND)this->winId(), GCL_STYLE);
            style &= ~(CS_VREDRAW | CS_HREDRAW);
            SetClassLongPtr((HWND)this->winId(), GCL_STYLE, style);

            initWindowBackground(false);
            forceDarkMode(HWND(effectiveWinId()));

            updateWindowFrameAttributes();
            d->cloakPending_ = true;
            d->uncloakQueued_ = false;
            setWindowCloaked(true);
            update();
            if (auto* window = windowHandle())
            {
                window->requestUpdate();
            }
        }
        syncWindowHintGeometry();
    }

    if (event->type() == QEvent::WindowStateChange)
    {
        if (isVisible() && !isMinimized())
        {
            updateWindowFrameAttributes();
        }
        syncWindowHintGeometry();
        if (d->maximize_)
        {
            d->maximize_->setIcon(QIcon(maximizeIconPath(isMaximized()))
                                      .pixmap(kHintIconSize, kHintIconSize));
        }
    }
    if (event->type() == QEvent::WindowActivate)
    {
        queueRestoreClientFocus();
    }
#endif
#ifdef Q_OS_MACOS
    if (event->type() != QEvent::DeferredDelete)
    {
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
    d->logo_->setFocusPolicy(Qt::NoFocus);
    d->logo_->setProperty("class", "menuWindowBt");

    QObject::connect(d->logo_, &QAbstractButton::released, this,
                     [this]() { showSystemMenu(this, systemMenuAnchor()); });
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
            if (!toolbar->children().empty())
            {
                d->divider_->setVisible(true);
            };
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

}  // namespace ads
