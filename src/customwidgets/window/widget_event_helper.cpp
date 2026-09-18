#include "widget_event_helper.h"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QStyle>
#include <QWidget>

namespace
{
void UpdateHoverState(QWidget* widget, bool hovered)
{
    if (!widget)
    {
        return;
    }
    if (widget->property("hovered").toBool() == hovered
        && widget->testAttribute(Qt::WA_UnderMouse) == hovered)
    {
        return;
    }

    widget->setAttribute(Qt::WA_UnderMouse, hovered);
    widget->setProperty("hovered", hovered);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}
}  // namespace

WidgetEventHelper::WidgetEventHelper(QObject* parent)
    : QObject(parent),
      widget_(nullptr),
      inWidgetRect_(false),
      inLastWidgetRect_(false),
      left_(false),
      pressed_(false),
      firstMove_(false)
{}

WidgetEventHelper::~WidgetEventHelper()
{}

void WidgetEventHelper::SetWidget(QWidget* widget)
{
    widget_ = widget;
    UpdateHoverState(widget_, false);
}

void WidgetEventHelper::ReleaseFlag()
{
    pressed_ = false;
    firstMove_ = false;
}

void WidgetEventHelper::SetWidgetRectFlag(bool inWidgetRect)
{
    inWidgetRect_ = inWidgetRect;
}

QWidget* WidgetEventHelper::Widget()
{
    return widget_;
}

bool WidgetEventHelper::IsFirstMove()
{
    return firstMove_;
}

void WidgetEventHelper::SetFirstMove(bool firstEnter)
{
    firstMove_ = firstEnter;
}

bool WidgetEventHelper::HandleMousePress(Q_RESULT_TYPE result)
{
    *result = 0;
    if (widget_)
    {
        this->firstMove_ = true;
        this->pressed_ = true;
        this->SendMousePress();
        return true;
    }
    return false;
}

bool WidgetEventHelper::HandleMouseRelease(Q_RESULT_TYPE result, bool isNClient)
{
    *result = 0;
    if (widget_)
    {
        if (pressed_)
        {
            pressed_ = false;
            if (isNClient)
            {
                SendMouseRelease(inWidgetRect_);
                left_ = true;
                inWidgetRect_ = false;
                inLastWidgetRect_ = false;
                SendMouseLeave();
                return true;
            }
            else
            {
                if (inWidgetRect_)
                {
                    SendMousePress();
                    SendMouseRelease(true);
                }
            }
        }
    }
    return false;
}

void WidgetEventHelper::HandleMouseMove()
{
    if (widget_)
    {
        if (left_)
        {
            left_ = false;
            return;
        }
        if (inWidgetRect_ != inLastWidgetRect_)
        {
            inLastWidgetRect_ = inWidgetRect_;
            if (inWidgetRect_)
            {
                SendMouseEnter();
            }
            else
            {
                SendMouseLeave();
            }
        }
    }

    inWidgetRect_ = false;
}

void WidgetEventHelper::SendMouseEnter()
{
    UpdateHoverState(widget_, true);
    QEvent event(QEvent::Enter);
    QApplication::sendEvent(widget_, &event);
}

void WidgetEventHelper::SendMouseLeave()
{
    UpdateHoverState(widget_, false);
    QEvent event(QEvent::Leave);
    QApplication::sendEvent(widget_, &event);
}

void WidgetEventHelper::SendMousePress()
{
    QMouseEvent event(QEvent::MouseButtonPress, QPoint(0, 0), Qt::LeftButton,
                      Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(widget_, &event);
}

void WidgetEventHelper::SendMouseRelease(bool inWidgetRect)
{
    if (inWidgetRect)
    {
        QMouseEvent event(QEvent::MouseButtonRelease, QPoint(0, 0),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(widget_, &event);
    }
    else
    {
        QMouseEvent event(QEvent::MouseButtonRelease, QPoint(-1, -1),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(widget_, &event);
    }
}
