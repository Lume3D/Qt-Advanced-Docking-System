#include <QCoreApplication>
#include <QLayout>

#include "styled_window_p.h"

// This translation unit is deliberately free of OS-specific code. It builds
// the title bar out of plain Qt widgets and routes every event through the
// platform hooks, which are implemented in styled_window_win.cpp,
// styled_window_mac.cpp or styled_window_default.cpp.

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
    platformShutdown();
    delete d;
}

void StyledWindow::init()
{
    d->justMaximized_ = false;
    d->resizeable_ = true;
    d->displayScale_ = devicePixelRatioF();

    platformInit();

    Qt::WindowFlags flags;
    flags |= Qt::Window;
    flags |= Qt::WindowMinMaxButtonsHint;
    flags |= Qt::WindowCloseButtonHint;
    platformApplyWindowFlags(flags);

    setWindowFlags(flags);
    initWindowTitle();
    this->setAttribute(Qt::WA_TranslucentBackground);
}

void StyledWindow::setupMenuBar(QMenuBar* menuBar)
{
    platformSetupMenuBar(menuBar);
}

bool StyledWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (auto* toolBar = qobject_cast<QToolBar*>(watched))
    {
        platformFilterToolBarEvent(toolBar, event);
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
    platformStyleTitleBar();
    d->windowHint_->setMovable(false);
    d->windowHint_->setFloatable(false);
    d->windowHint_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    d->windowHint_->layout()->setContentsMargins(0, 0, 0, 0);
    d->windowHint_->layout()->setSpacing(0);
    d->windowHint_->setWindowFlags(Qt::WindowTitleHint);
    platformAddTitleBarLogo(leftLayout);

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
    platformAddTitleBarDivider(rightLayout);
    d->windowHint_->addWidget(d->rightLayoutWidget_);

    this->setProperty("class", "window-main");
    this->addToolBarBreak();
    platformFinishTitleBar();
}

bool StyledWindow::event(QEvent* event)
{
    auto native = QMainWindow::event(event);
    platformHandleEvent(event);
    return native;
}

QMenuBar* StyledWindow::menuBar()
{
    return platformMenuBar();
}

void StyledWindow::setIcon(QIcon icon)
{
    platformSetIcon(icon);
}

void StyledWindow::setSubToolbar(QToolBar* toolbar)
{
    if (toolbar)
    {
        platformSetSubToolbar(toolbar);
    }
}

}  // namespace ads
