#include "styled_window_p.h"

#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)

#    include <QLayout>
#    include <QToolBar>

// Fallback implementation of the platform hooks declared in styled_window.h,
// used on platforms with no native frame integration. The window keeps its
// Qt-drawn title bar and every hook either does nothing or defers to
// QMainWindow.

namespace ads
{

void StyledWindow::platformInit()
{}

void StyledWindow::platformShutdown()
{}

void StyledWindow::platformApplyWindowFlags(Qt::WindowFlags& flags)
{
    Q_UNUSED(flags);
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
    Q_UNUSED(event);
}

void StyledWindow::platformHandleEvent(QEvent* event)
{
    Q_UNUSED(event);
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

#endif  // !Q_OS_WIN && !Q_OS_MACOS
