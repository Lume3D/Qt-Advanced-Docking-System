#include "styled_window_p.h"

#ifdef Q_OS_MACOS

#    include <QApplication>
#    include <QLayout>
#    include <QMouseEvent>
#    include <QToolBar>

#    include "macos_helper.h"

// macOS implementation of the platform hooks declared in styled_window.h.
// The native title bar is hidden and the toolbar synthesises the non-client
// mouse events Qt would otherwise never see.

namespace ads
{

void StyledWindow::platformInit()
{}

void StyledWindow::platformShutdown()
{}

void StyledWindow::platformApplyWindowFlags(Qt::WindowFlags& flags)
{
    flags |= Qt::WindowFullscreenButtonHint;
    flags |= Qt::CustomizeWindowHint;

#    if QT_VERSION_MAJOR >= 6
    this->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
#    endif
}

void StyledWindow::platformStyleTitleBar()
{}

void StyledWindow::platformAddTitleBarLogo(QHBoxLayout* leftLayout)
{
    Q_UNUSED(leftLayout);
}

void StyledWindow::platformAddTitleBarDivider(QHBoxLayout* rightLayout)
{
    Q_UNUSED(rightLayout);
}

void StyledWindow::platformFinishTitleBar()
{}

void StyledWindow::platformFilterToolBarEvent(QToolBar* toolBar, QEvent* event)
{
    Q_UNUSED(toolBar);

    if (event->type() == QEvent::MouseButtonPress)
    {
        auto* e = static_cast<QMouseEvent*>(event);
        if (e && e->button() == Qt::LeftButton)
        {
            auto nonClientEvent =
                QMouseEvent(QEvent::NonClientAreaMouseButtonPress, e->localPos(),
                            e->button(), e->buttons(), e->modifiers());
            QApplication::sendEvent(this, &nonClientEvent);
        }
        if (e && !this->isActiveWindow())
        {
            auto activateEvent = QEvent(QEvent::WindowActivate);
            QApplication::sendEvent(this, &activateEvent);
        }
    }
    else if (event->type() == QEvent::MouseButtonRelease)
    {
        auto* e = static_cast<QMouseEvent*>(event);
        if (e && e->button() == Qt::LeftButton)
        {
            auto nonClientEvent = QEvent(QEvent::NonClientAreaMouseButtonRelease);
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
}

void StyledWindow::platformHandleEvent(QEvent* event)
{
    if (event->type() != QEvent::DeferredDelete)
    {
        HideTitleBar(this->winId());
    }
}

void StyledWindow::platformSetupMenuBar(QMenuBar* menuBar)
{
    this->setMenuBar(menuBar);
}

QMenuBar* StyledWindow::platformMenuBar()
{
    return QMainWindow::menuBar();
}

void StyledWindow::platformSetIcon(const QIcon& icon)
{
    Q_UNUSED(icon);
}

void StyledWindow::platformSetSubToolbar(QToolBar* toolbar)
{
    auto layout = qobject_cast<QHBoxLayout*>(d->rightLayoutWidget_->layout());
    if (layout)
    {
        layout->insertWidget(0, toolbar, 0, Qt::AlignRight | Qt::AlignVCenter);
    }
}

}  // namespace ads

#endif  // Q_OS_MACOS
