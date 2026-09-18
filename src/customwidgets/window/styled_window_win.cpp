#include "styled_window_p.h"

#ifdef Q_OS_WIN

namespace ads
{
float nativeWindowDpr(HWND hwnd, float fallback)
{
    if (!hwnd)
    {
        return fallback;
    }

    const auto dpi = GetDpiForWindow(hwnd);
    if (dpi == 0)
    {
        return fallback;
    }

    return static_cast<float>(dpi) / static_cast<float>(USER_DEFAULT_SCREEN_DPI);
}

// The maximize button doubles as restore, so its glyph tracks window state.
const char* maximizeIconPath(bool maximized)
{
    return maximized ? ":/icons/Icon_Restore_Window.svg" :
                       ":/icons/Icon_Maximize_Window.svg";
}

HRESULT forceDarkMode(HWND hwnd)
{
    BOOL value = TRUE;
    return DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value,
                                 sizeof(value));
}

void StyledWindow::setResizeable(bool resizeable)
{
    d->resizeable_ = resizeable;
    HWND hwnd = reinterpret_cast<HWND>(this->winId());
    LONG_PTR style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
    LONG_PTR exStyle = ::GetWindowLongPtr(hwnd, GWL_EXSTYLE);

    style &= ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);
    style |= WS_POPUP | WS_CAPTION | WS_THICKFRAME;

    if (windowFlags() & Qt::WindowMaximizeButtonHint)
    {
        style |= WS_MAXIMIZEBOX;
    }

    if (windowFlags() & Qt::WindowMinimizeButtonHint)
    {
        style |= WS_MINIMIZEBOX;
    }

    exStyle |= WS_EX_COMPOSITED;

    ::SetWindowLongPtr(hwnd, GWL_STYLE, style);
    ::SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
    ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER
                       | SWP_FRAMECHANGED | SWP_NOACTIVATE);

    const MARGINS shadow = {1, 1, 1, 1};
    DwmExtendFrameIntoClientArea(hwnd, &shadow);
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
    titlebar->setFocusPolicy(Qt::NoFocus);
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

// Applies the look and event wiring shared by every title-bar chrome button.
void StyledWindow::initHintButton(QPushButton* button, const char* cssClass,
                                  WidgetEventHelper* helper)
{
    button->setProperty("class", cssClass);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    button->setFocusPolicy(Qt::NoFocus);
    helper->SetWidget(button);
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
                initHintButton(d->minimize_, "minimizeWindowBt",
                               d->minimizeHelper_);
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
                auto maximizeIcon = QIcon(maximizeIconPath(this->isMaximized()));
                d->maximize_ = new QPushButton(maximizeIcon, "", this);
                initHintButton(d->maximize_, "maximizeWindowBt",
                               d->maximizeHelper_);

                QObject::connect(
                    d->maximize_, &QAbstractButton::released, this, [this]() {
                        if (this->isOutOfWidget(d->maximize_))
                        {
                            return;
                        }
                        const auto hwnd =
                            reinterpret_cast<HWND>(this->effectiveWinId());
                        if (!hwnd)
                        {
                            return;
                        }

                        d->pendingStateResizePaint_ = true;
                        redrawWindowNow(hwnd);

                        const auto command = this->isMaximized() ? SC_RESTORE :
                                                                   SC_MAXIMIZE;
                        SendMessageW(hwnd, WM_SYSCOMMAND, command, 0);
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
                initHintButton(d->close_, "closeWindowBt", d->closeHelper_);

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

void StyledWindow::syncWindowHintGeometry()
{
    ::SetWindowPos((HWND)this->winId(), nullptr, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOOWNERZORDER
                       | SWP_FRAMECHANGED | SWP_NOACTIVATE);
#    if QT_VERSION_MAJOR >= 6
    if (!d->windowHint_)
    {
        return;
    }

    d->windowHint_->updateGeometry();
    if (auto* layout = d->windowHint_->layout())
    {
        layout->invalidate();
        layout->activate();
    }

    const auto size = d->windowHint_->size();
    QResizeEvent rsEvent(size, size);
    QApplication::sendEvent(d->windowHint_, &rsEvent);
#    endif
}

bool StyledWindow::isTitleBarChrome(const QWidget* widget) const
{
    if (!widget)
    {
        return false;
    }

    if (widget == d->titleBar_)
    {
        return true;
    }

    return d->titleBar_
           && d->titleBar_->isAncestorOf(const_cast<QWidget*>(widget));
}

void StyledWindow::rememberFocusedWidget(QWidget* widget)
{
    if (!widget || widget->window() != this || isTitleBarChrome(widget))
    {
        return;
    }

    d->lastFocusedWidget_ = widget;
}

void StyledWindow::restoreClientFocus()
{
    if (!isVisible() || isMinimized() || !isActiveWindow()
        || QApplication::activePopupWidget())
    {
        return;
    }

    QWidget* current = QApplication::focusWidget();
    if (current && current->window() == this && !isTitleBarChrome(current))
    {
        rememberFocusedWidget(current);
        return;
    }

    QWidget* target = d->lastFocusedWidget_;
    if (!target || target->window() != this || isTitleBarChrome(target)
        || target->focusPolicy() == Qt::NoFocus || !target->isVisible()
        || !target->isEnabled())
    {
        target = nullptr;
    }

    if (!target && centralWidget())
    {
        auto* centralFocus = centralWidget()->focusWidget();
        if (centralFocus && centralFocus->window() == this
            && !isTitleBarChrome(centralFocus)
            && centralFocus->focusPolicy() != Qt::NoFocus
            && centralFocus->isVisible() && centralFocus->isEnabled())
        {
            target = centralFocus;
        }
    }

    if (!target && centralWidget()
        && centralWidget()->focusPolicy() != Qt::NoFocus)
    {
        target = centralWidget();
    }

    if (target)
    {
        target->setFocus(Qt::ActiveWindowFocusReason);
        d->lastFocusedWidget_ = target;
    }
}

void StyledWindow::queueRestoreClientFocus()
{
    if (d->focusRestoreQueued_)
    {
        return;
    }

    d->focusRestoreQueued_ = true;
    QMetaObject::invokeMethod(
        this,
        [this]() {
            d->focusRestoreQueued_ = false;
            restoreClientFocus();
        },
        Qt::QueuedConnection);
}

void StyledWindow::updateWindowDpr(float dpr, QRect rect, WId wid)
{
    d->displayScale_ = dpr;
    {
        SetWindowPos((HWND)wid, NULL, rect.left(), rect.top(), rect.width(),
                     rect.height(), SWP_NOZORDER | SWP_NOACTIVATE);

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
        syncWindowHintGeometry();
    }
}

// Repaints the window and all children immediately. RDW_NOERASE is the
// default here because erasing the background first is what produces the
// white flash during maximize/restore.
void StyledWindow::redrawWindowNow(HWND hwnd, bool eraseBackground)
{
    UINT flags = RDW_INVALIDATE | RDW_ALLCHILDREN;
    if (!eraseBackground)
    {
        flags |= RDW_NOERASE;
    }
    RedrawWindow(hwnd, nullptr, nullptr, flags);
    update();
    if (auto* window = windowHandle())
    {
        window->requestUpdate();
    }
}

// Returns false when a refresh is already pending, so the caller can leave
// the message for DefWindowProc instead of swallowing it.
bool StyledWindow::scheduleDarkModeRefresh()
{
    if (d->darkModeSettingGuard_)
    {
        return false;
    }
    d->darkModeSettingGuard_ = true;
    QTimer::singleShot(kDarkModeRefreshDelayMs, [this]() {
        forceDarkMode(HWND(internalWinId()));
        d->darkModeSettingGuard_ = false;
    });
    return true;
}

// Anchors the system menu under the logo button, falling back to the
// window's top-left corner when there is no logo.
QPoint StyledWindow::systemMenuAnchor() const
{
    RECT rect;
    GetWindowRect(reinterpret_cast<HWND>(winId()), &rect);
    QPoint point(rect.left, rect.top);
    if (d->logo_)
    {
        const auto dpr = this->devicePixelRatioF();
        const auto geometry = d->logo_->geometry().bottomLeft();
        point = QPoint(point.x() + geometry.x() + (dpr * kSystemMenuOffsetX),
                       point.y() + geometry.y() + (dpr * kSystemMenuOffsetY));
    }
    return point;
}

void StyledWindow::forceRedraw()
{
    auto* window = windowHandle();
    auto* screen = QApplication::screenAt(window->geometry().center());
    if (!screen)
    {
        screen = window->screen();
    }
    window->setScreen(nullptr);
    window->requestUpdate();
    window->setScreen(screen);
    window->requestUpdate();

    const bool isMax = this->isMaximized();
    if (!this->isMinimized())
    {
        this->showMinimized();
        if (isMax)
        {
            this->showMaximized();
        }
        else
        {
            this->showNormal();
        }
    }
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

void StyledWindow::initWindowBackground(bool transparent)
{
    HWND hwnd = (HWND)this->window()->winId();
    if (!d->backgroundBrush_)
    {
        d->backgroundBrush_ = CreateSolidBrush(RGB(0x10, 0x10, 0x10));
    }
    if (d->backgroundBrush_)
    {
        SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND,
                        reinterpret_cast<LONG_PTR>(d->backgroundBrush_));
    }

    HMODULE hUser = GetModuleHandleA("user32.dll");
    if (hUser)
    {
        pfnSetWindowCompositionAttribute setWindowCompositionAttribute =
            (pfnSetWindowCompositionAttribute)GetProcAddress(
                hUser, "SetWindowCompositionAttribute");

        if (setWindowCompositionAttribute)
        {
            ACCENT_POLICY accent = {transparent ?
                                        ACCENT_ENABLE_ACRYLICBLURBEHIND :
                                        ACCENT_ENABLE_GRADIENT,
                                    0, 0, 0};
            accent.GradientColor = kWindowBackdropGradient;

            WINDOWCOMPOSITIONATTRIBDATA data{WCA_ACCENT_POLICY, &accent,
                                             sizeof(accent)};
            setWindowCompositionAttribute(hwnd, &data);
        }
    }
}

void StyledWindow::updateWindowFrameAttributes()
{
    const auto hwnd = reinterpret_cast<HWND>(this->effectiveWinId());
    if (!hwnd)
    {
        return;
    }

    if (QOperatingSystemVersion::current().microVersion() > 22000)
    {
        const DWORD captionColor = 0x00101010;
        DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &captionColor,
                              sizeof(captionColor));
    }
}

void StyledWindow::setWindowCloaked(bool cloaked)
{
    if (d->cloaked_ == cloaked)
    {
        return;
    }

    const auto hwnd = reinterpret_cast<HWND>(this->effectiveWinId());
    if (!hwnd)
    {
        return;
    }

    const BOOL cloak = cloaked ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_CLOAK, &cloak, sizeof(cloak));
    d->cloaked_ = cloaked;
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

void StyledWindow::showSystemMenu(QWidget* widget, const QPoint& pos)
{
    auto hwnd = reinterpret_cast<HWND>(widget->effectiveWinId());
    if (d->sysMenu_)
    {
        UpdateWindow(hwnd);

        if (this->isMaximized())
        {
            EnableMenuItem(d->sysMenu_, SC_MOVE, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(d->sysMenu_, SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(d->sysMenu_, SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(d->sysMenu_, SC_RESTORE, MF_BYCOMMAND | MF_ENABLED);
        }
        else
        {
            EnableMenuItem(d->sysMenu_, SC_MOVE, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(d->sysMenu_, SC_SIZE, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(d->sysMenu_, SC_MAXIMIZE, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(d->sysMenu_, SC_RESTORE, MF_BYCOMMAND | MF_GRAYED);
        }
        int command = TrackPopupMenu(d->sysMenu_,
                                     TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
                                     pos.x(), pos.y(), 0, hwnd, nullptr);
        if (command)
        {
            SendMessage(hwnd, WM_SYSCOMMAND, command, 0);
        }
    }
}

bool StyledWindow::onSysKeyDown(tagMSG* msg, Q_RESULT_TYPE result)
{
    if (msg->wParam == VK_SPACE)
    {
        showSystemMenu(this, systemMenuAnchor());
    }
    return false;
}

bool StyledWindow::onNcCalcSize(tagMSG* msg, Q_RESULT_TYPE result)
{
    if (this->isVisible())
    {
        const bool isMaximized = ::IsZoomed(msg->hwnd) != FALSE;
        const auto rect =
            msg->wParam ?
                &(reinterpret_cast<LPNCCALCSIZE_PARAMS>(msg->lParam))->rgrc[0] :
                reinterpret_cast<LPRECT>(msg->lParam);

        if (!isMaximized)
        {
            const RECT oriRect = *rect;
            const auto oriResult = ::DefWindowProcW(msg->hwnd, WM_NCCALCSIZE,
                                                    msg->wParam, msg->lParam);
            if (oriResult)
            {
                *result = oriResult;
                return true;
            }
            // In normal state Qt6 can drift away from the visible HWND
            // bounds if we keep DefWindowProc's hidden frame insets here.
            // Preserve the original rect so the Qt client matches the
            // actual visible window bounds.
            *rect = oriRect;
        }
        *result = false;
        return true;
    }
    return false;
}

bool StyledWindow::onNcHitTest(tagMSG* msg, Q_RESULT_TYPE result)
{
    *result = 0;
    for (const auto& button : d->chromeButtons())
    {
        if (button.helper)
        {
            button.helper->HandleMouseMove();
        }
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

    const QPoint pos = d->titleBar_->mapFromGlobal(QCursor::pos());
    if (isOutOfWidget(d->titleBar_))
        return false;

    QWidget* child = d->titleBar_->childAt(pos);
    if (!child)
    {
        *result = HTCAPTION;
        return true;
    }

    for (const auto& button : d->chromeButtons())
    {
        if (button.helper && button.helper->Widget()
            && button.helper->Widget() == child)
        {
            button.helper->SetWidgetRectFlag(true);
            *result = button.hitTest;
            return true;
        }
    }
    if (d->whiteList_.contains(child))
    {
        *result = HTCAPTION;
        return true;
    }
    return false;
}

bool StyledWindow::onDisplayChange(tagMSG* msg, Q_RESULT_TYPE result)
{
    qDebug() << ("DISPLAYS Changed\n");
    // DPI LOST AFTER ADD OR REMOVE DISPLAY
    QTimer::singleShot(1000, [this]() { forceRedraw(); });
    return false;
}

bool StyledWindow::onDpiChanged(tagMSG* msg, Q_RESULT_TYPE result)
{
    const auto dpi = static_cast<UINT>(HIWORD(msg->wParam));
    const auto dpr = static_cast<float>(dpi)
                     / static_cast<float>(USER_DEFAULT_SCREEN_DPI);
    RECT* const rect = (RECT*)msg->lParam;
    qDebug() << ("DPI Changed: ") << dpr;
    d->displayScale_ = dpr;
    updateWindowDpr(dpr,
                    QRect(rect->left, rect->top, rect->right - rect->left,
                          rect->bottom - rect->top),
                    (WId)msg->hwnd);
    return false;
}

bool StyledWindow::onSize(tagMSG* msg, Q_RESULT_TYPE result)
{
    const bool wasJustMinimized = d->justMinimized_;
    if (msg->wParam == SIZE_RESTORED && d->justMinimized_)
    {
        d->justMinimized_ = false;
    }

    if (msg->wParam == SIZE_MINIMIZED)
    {
        d->justMinimized_ = true;
        d->pendingStateResizePaint_ = false;
    }
    else if (msg->wParam == SIZE_MAXIMIZED
             || (msg->wParam == SIZE_RESTORED && !wasJustMinimized))
    {
        d->pendingStateResizePaint_ = true;
        redrawWindowNow(msg->hwnd);
    }
    return false;
}

bool StyledWindow::onGetMinMaxInfo(tagMSG* msg, Q_RESULT_TYPE result)
{
    if (::IsZoomed(msg->hwnd))
    {
        RECT frame = {0, 0, 0, 0};
        AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);

        const auto dpr = nativeWindowDpr(msg->hwnd, d->displayScale_);

        d->frames_.setLeft(abs(frame.left) / dpr + 0.5);
        d->frames_.setTop(abs(frame.bottom) / dpr + 0.5);
        d->frames_.setRight(abs(frame.right) / dpr + 0.5);
        d->frames_.setBottom(abs(frame.bottom) / dpr + 0.5);

        QMainWindow::setContentsMargins(d->frames_.left() + d->margins_.left(),
                                        d->frames_.top() + d->margins_.top(),
                                        d->frames_.right() + d->margins_.right(),
                                        d->frames_.bottom()
                                            + d->margins_.bottom());
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

bool StyledWindow::onLButtonUp(tagMSG* msg, Q_RESULT_TYPE result)
{
    if (!d->chromeHelpersReady())
    {
        return false;
    }

    for (const auto& button : d->chromeButtons())
    {
        button.helper->HandleMouseRelease(result, false);
    }
    return false;
}

bool StyledWindow::onNcMouseLeave(tagMSG* msg, Q_RESULT_TYPE result)
{
    if (!d->chromeHelpersReady())
    {
        return false;
    }

    for (const auto& button : d->chromeButtons())
    {
        button.helper->SetWidgetRectFlag(false);
    }
    for (const auto& button : d->chromeButtons())
    {
        button.helper->HandleMouseMove();
    }

    return false;
}

bool StyledWindow::onEraseBackground(tagMSG* msg, Q_RESULT_TYPE result)
{
    if (d->pendingStateResizePaint_ || d->cloakPending_)
    {
        *result = 1;
        return true;
    }
    return false;
}

bool StyledWindow::onNcUahDraw(tagMSG* msg, Q_RESULT_TYPE result)
{
    *result = 0;
    return true;
}

bool StyledWindow::onMouseMove(tagMSG* msg, Q_RESULT_TYPE result)
{
    if (!d->chromeHelpersReady())
    {
        return false;
    }
    *result = 0;
    for (const auto& button : d->chromeButtons())
    {
        if (button.helper->IsFirstMove())
        {
            button.helper->SetFirstMove(false);
            button.helper->SendMouseRelease(false);
        }
    }

    for (const auto& button : d->chromeButtons())
    {
        button.helper->HandleMouseMove();
    }

    if (!d->titleBar_)
        return false;

    const QPoint pos = d->titleBar_->mapFromGlobal(QCursor::pos());
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
        for (const auto& button : d->chromeButtons())
        {
            if (button.helper->Widget() && button.helper->Widget() == child)
            {
                button.helper->SetWidgetRectFlag(true);
            }
        }
    }
    return false;
}

bool StyledWindow::onNcLButtonDown(tagMSG* msg, Q_RESULT_TYPE result)
{
    if (!d->chromeHelpersReady())
    {
        return false;
    }
    for (const auto& button : d->chromeButtons())
    {
        button.helper->HandleMouseMove();
    }
    if (auto* helper = d->chromeHelperFor(msg->wParam))
    {
        if (helper->HandleMousePress(result))
            return true;
    }
    return false;
}

bool StyledWindow::onNcLButtonUp(tagMSG* msg, Q_RESULT_TYPE result)
{
    if (!d->chromeHelpersReady())
    {
        return false;
    }
    if (auto* helper = d->chromeHelperFor(msg->wParam))
    {
        if (helper->HandleMouseRelease(result))
            return true;
    }

    for (const auto& button : d->chromeButtons())
    {
        button.helper->ReleaseFlag();
    }
    return false;
}

bool StyledWindow::onNcLButtonDblClk(tagMSG* msg, Q_RESULT_TYPE result)
{
    if (msg->wParam == HTCAPTION)
    {
        d->pendingStateResizePaint_ = true;
        redrawWindowNow(msg->hwnd);

        *result =
            DefWindowProcW(msg->hwnd, msg->message, msg->wParam, msg->lParam);
        return true;
    }
    return false;
}

bool StyledWindow::onEnterSizeMove(tagMSG* msg, Q_RESULT_TYPE result)
{
    d->inSizeMove_ = true;
    return false;
}

bool StyledWindow::onExitSizeMove(tagMSG* msg, Q_RESULT_TYPE result)
{
    d->inSizeMove_ = false;
    redrawWindowNow(msg->hwnd, /*eraseBackground=*/true);
    return false;
}

bool StyledWindow::onStyleChanged(tagMSG* msg, Q_RESULT_TYPE result)
{
    if (msg->wParam == GWL_STYLE)
    {
        const auto* style = reinterpret_cast<const STYLESTRUCT*>(msg->lParam);
        constexpr DWORD kFrameStyleMask = WS_CAPTION | WS_THICKFRAME
                                          | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
        // Modal dialogs temporarily toggle owner styles such as
        // WS_DISABLED. Ignore those changes so we do not force a full
        // frame refresh and nudge the window position.
        if (style
            && (((style->styleOld ^ style->styleNew) & kFrameStyleMask) != 0))
        {
            setResizeable(d->resizeable_);
            constructHintButtons();
        }
    }
    return false;
}

bool StyledWindow::onSetFocus(tagMSG* msg, Q_RESULT_TYPE result)
{
    queueRestoreClientFocus();
    return false;
}

bool StyledWindow::onActivate(tagMSG* msg, Q_RESULT_TYPE result)
{
    if (LOWORD(msg->wParam) != WA_INACTIVE)
    {
        queueRestoreClientFocus();
    }
    return false;
}

bool StyledWindow::onThemeChanged(tagMSG* msg, Q_RESULT_TYPE result)
{
    if (scheduleDarkModeRefresh())
    {
        *result = 0;
        return true;
    }
    return false;
}

bool StyledWindow::onSettingChange(tagMSG* msg, Q_RESULT_TYPE result)
{
    if (wcscmp(reinterpret_cast<LPCWSTR>(msg->lParam), L"ImmersiveColorSet") == 0)
    {
        if (scheduleDarkModeRefresh())
        {
            *result = 0;
            return true;
        }
    }
    return false;
}

bool StyledWindow::onNcActivate(tagMSG* msg, Q_RESULT_TYPE result)
{
    *result =
        DefWindowProcW(HWND(effectiveWinId()), WM_NCACTIVATE, msg->wParam, -1);
    return false;
}

bool StyledWindow::onWindowPosChanging(tagMSG* msg, Q_RESULT_TYPE result)
{
    const auto windowPos = reinterpret_cast<LPWINDOWPOS>(msg->lParam);
    if (!d->inSizeMove_ && (d->pendingStateResizePaint_ || d->cloakPending_))
    {
        windowPos->flags |= SWP_NOCOPYBITS;
    }
    return false;
}

bool StyledWindow::onPowerBroadcast(tagMSG* msg, Q_RESULT_TYPE result)
{
    switch (msg->wParam)
    {
    case PBT_APMRESUMEAUTOMATIC:
    {
        qDebug() << ("PBT_APMRESUMEAUTOMATIC  received\n");
        // DPI LOST AFTER RESUME FROM SLEEP
        QTimer::singleShot(100, [this]() {
            RECT rect;
            const auto hwnd = reinterpret_cast<HWND>(this->winId());
            GetWindowRect(hwnd, &rect);
            updateWindowDpr(nativeWindowDpr(hwnd, d->displayScale_),
                            QRect(rect.left, rect.top, rect.right - rect.left,
                                  rect.bottom - rect.top),
                            this->winId());
        });
        break;
    }
    case PBT_APMPOWERSTATUSCHANGE:
    {
        qDebug() << ("PBT_APMPOWERSTATUSCHANGE  received\n");
        break;
    }
    case PBT_APMRESUMESUSPEND:
    {
        qDebug() << ("PBT_APMRESUMESUSPEND  received\n");
        break;
    }
    case PBT_APMSUSPEND:
    {
        qDebug() << ("PBT_APMSUSPEND  received\n");
        break;
    }
    }
    return false;
}

bool StyledWindow::nativeEvent(const QByteArray& eventType, void* message,
                               Q_RESULT_TYPE result)
{
#    if (QT_VERSION == QT_VERSION_CHECK(5, 11, 1))
    MSG* msg = *reinterpret_cast<MSG**>(message);
#    else
    MSG* msg = reinterpret_cast<MSG*>(message);
#    endif

    switch (msg->message)
    {
    case WM_SYSKEYDOWN: return onSysKeyDown(msg, result);
    case WM_NCCALCSIZE: return onNcCalcSize(msg, result);
    case WM_NCHITTEST: return onNcHitTest(msg, result);
    case WM_DISPLAYCHANGE: return onDisplayChange(msg, result);
    case WM_DPICHANGED: return onDpiChanged(msg, result);
    case WM_SIZE: return onSize(msg, result);
    case WM_GETMINMAXINFO: return onGetMinMaxInfo(msg, result);
    case WM_LBUTTONUP: return onLButtonUp(msg, result);
    case WM_NCMOUSELEAVE: return onNcMouseLeave(msg, result);
    case WM_ERASEBKGND: return onEraseBackground(msg, result);
    case WM_NCUAHDRAWCAPTION:
    case WM_NCUAHDRAWFRAME: return onNcUahDraw(msg, result);
    case WM_MOUSEMOVE: return onMouseMove(msg, result);
    case WM_NCLBUTTONDOWN: return onNcLButtonDown(msg, result);
    case WM_NCLBUTTONUP: return onNcLButtonUp(msg, result);
    case WM_NCLBUTTONDBLCLK: return onNcLButtonDblClk(msg, result);
    case WM_ENTERSIZEMOVE: return onEnterSizeMove(msg, result);
    case WM_EXITSIZEMOVE: return onExitSizeMove(msg, result);
    case WM_STYLECHANGED: return onStyleChanged(msg, result);
    case WM_SETFOCUS: return onSetFocus(msg, result);
    case WM_ACTIVATE: return onActivate(msg, result);
    case WM_THEMECHANGED: return onThemeChanged(msg, result);
    case WM_SETTINGCHANGE: return onSettingChange(msg, result);
    case WM_NCACTIVATE: return onNcActivate(msg, result);
    case WM_WINDOWPOSCHANGING: return onWindowPosChanging(msg, result);
    case WM_POWERBROADCAST: return onPowerBroadcast(msg, result);
    default: break;
    }
    return false;
}
}  // namespace ads

#endif  // Q_OS_WIN
