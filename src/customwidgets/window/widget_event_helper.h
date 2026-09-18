#ifndef ADS_WIDGET_EVENT_HELPER_H
#define ADS_WIDGET_EVENT_HELPER_H

#include <QObject>

#include "type_versions.h"

class QWidget;

class WidgetEventHelper : public QObject
{
    Q_OBJECT
public:
    WidgetEventHelper(QObject* parent = nullptr);
    ~WidgetEventHelper();

    void SetWidget(QWidget* widget);
    void ReleaseFlag();

    void SetWidgetRectFlag(bool inWidgetRect);
    QWidget* Widget();
    bool IsFirstMove();

    void SetFirstMove(bool firstEnter);

    bool HandleMousePress(Q_RESULT_TYPE result);
    bool HandleMouseRelease(Q_RESULT_TYPE result, bool isNClient = true);
    void HandleMouseMove();

    void SendMouseRelease(bool inWidgetRect);

private:
    void SendMouseEnter();
    void SendMouseLeave();
    void SendMousePress();

private:
    QWidget* widget_{nullptr};

    bool inWidgetRect_{false};
    bool inLastWidgetRect_{false};

    bool left_{false};
    bool pressed_{false};
    bool firstMove_{false};
};

#endif  // ADS_WIDGET_EVENT_HELPER_H
