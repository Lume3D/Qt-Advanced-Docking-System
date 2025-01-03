#ifndef FRAME_LESS_H
#define FRAME_LESS_H

#include <QMainWindow>
#include <QList>

class QLabel;
class QPushButton;
class QWidget;
class QHBoxLayout;
class WidgetEventHelper;

#define DEFAULT_WINDOW_TITLE ""

class StyledWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit StyledWindow(
        QWidget* parent = 0, Qt::WindowFlags f = Qt::WindowFlags(), QString windowTitle = DEFAULT_WINDOW_TITLE);
    ~StyledWindow();

    void init();
    void setWindowTitle(QString title);
    void setupMenuBar(QMenuBar* menuBar);

#ifdef WIN32
public slots:
    void showFullScreen();

protected:
    void initWindowTitle();
    void setResizeable(bool resizeable = true);
    bool isResizeable();
    void setResizeableAreaWidth(int width = 5);
    void setTitleBar(QWidget* titlebar);
    void addIgnoreWidget(QWidget* widget);
    void setContentsMargins(const QMargins& margins);
    void setContentsMargins(int left, int top, int right, int bottom);
    void constructHintButtons();

    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
    void resizeEvent(QResizeEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool event(QEvent* event) override;
    bool isOutOfWidget(QWidget* widget);
    QMenu* createPopupMenu() override;

private slots:
    void onTitleBarDestroyed();

private:
    QWidget* leftLayoutWidget_{ nullptr };
    QWidget* rightLayoutWidget_{ nullptr };
    QScreen* currentScreen_{ nullptr };
    QPushButton* maximize_{ nullptr };
    QPushButton* minimize_{ nullptr };
    QPushButton* close_{ nullptr };
    QMenuBar* menuBar_{ nullptr };
    QToolBar* windowHint_{ nullptr };
    QLabel* titleLabel_{ nullptr };
    QString windowTitle_{ "" };

    QWidget* titleBar_{ nullptr };
    QList<QWidget*> whiteList_;
    QMargins margins_;
    QMargins nativeMargins_;
    QMargins frames_;

    int borderWidth_{ 0 };
    bool justMaximized_{ false };
    bool justMinimized_{ false };
    bool resizeable_{ true };

    WidgetEventHelper* maximizeHelper_{ nullptr };
    WidgetEventHelper* minimizeHelper_{ nullptr };
    WidgetEventHelper* closeHelper_{ nullptr };
    float displayScale_{ 1.f };
#endif
};
#endif // FRAME_LESS_H
