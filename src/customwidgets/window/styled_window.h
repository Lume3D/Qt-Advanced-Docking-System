#ifndef ADS_STYLED_WINDOW_H
#define ADS_STYLED_WINDOW_H

#include <QMainWindow>

#include "ads_globals.h"
#include "type_versions.h"

class QMenu;
class QPushButton;
class QHBoxLayout;
class WidgetEventHelper;

#ifdef Q_OS_WIN
// The structs behind the MSG and HWND typedefs. Forward-declaring them keeps
// <windows.h> out of this header, which non-Windows-aware consumers include.
struct tagMSG;
struct HWND__;
#endif

namespace ads
{
class ADS_EXPORT IStyledWindow
{
public:
    virtual void setupMenuBar(QMenuBar* menu) = 0;
    virtual void setIcon(QIcon menu) = 0;
    virtual QMenuBar* menuBar() = 0;
    virtual void setSubToolbar(QToolBar* toolbar) = 0;

protected:
    ~IStyledWindow() = default;
};

class ADS_EXPORT StyledWindow : public QMainWindow, public IStyledWindow
{
    Q_OBJECT

public:
    explicit StyledWindow(QWidget* parent = nullptr,
                          Qt::WindowFlags f = Qt::WindowFlags(),
                          QString windowTitle = "");
    ~StyledWindow();

    void setupMenuBar(QMenuBar* menuBar) override;
    QMenuBar* menuBar() override;
    void setIcon(QIcon icon) override;
    void setSubToolbar(QToolBar* toolbar) override;

protected:
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void init();
    void initWindowTitle();

    // Platform hooks. Everything OS-specific lives behind these; this class
    // itself only deals in Qt. Exactly one implementation is compiled, chosen
    // by the build: styled_window_win.cpp, styled_window_mac.cpp or
    // styled_window_default.cpp.
    void platformInit();
    void platformShutdown();
    void platformApplyWindowFlags(Qt::WindowFlags& flags);
    void platformStyleTitleBar();
    void platformAddTitleBarLogo(QHBoxLayout* leftLayout);
    void platformAddTitleBarDivider(QHBoxLayout* rightLayout);
    void platformFinishTitleBar();
    void platformFilterToolBarEvent(QToolBar* toolBar, QEvent* event);
    void platformHandleEvent(QEvent* event);
    void platformSetupMenuBar(QMenuBar* menuBar);
    QMenuBar* platformMenuBar();
    void platformSetIcon(const QIcon& icon);
    void platformSetSubToolbar(QToolBar* toolbar);
#ifdef Q_OS_WIN
    void showSystemMenu(QWidget* widget, const QPoint& pos);
public slots:
    void showFullScreen();

protected:
    void initWindowBackground(bool transparent = true);
    void updateWindowFrameAttributes();
    void setWindowCloaked(bool cloaked);
    void setResizeable(bool resizeable = true);
    bool isResizeable();
    void setResizeableAreaWidth(int width = 5);
    void setTitleBar(QWidget* titlebar);
    void addIgnoreWidget(QWidget* widget);
    void setContentsMargins(const QMargins& margins);
    void setContentsMargins(int left, int top, int right, int bottom);
    void initHintButton(QPushButton* button, const char* cssClass,
                        WidgetEventHelper* helper);
    void constructHintButtons();

    bool nativeEvent(const QByteArray& eventType, void* message,
                     Q_RESULT_TYPE result) override;

    // One handler per intercepted message. Each returns what nativeEvent()
    // returns for that message and writes *result exactly as before.
    bool onSysKeyDown(tagMSG* msg, Q_RESULT_TYPE result);
    bool onNcCalcSize(tagMSG* msg, Q_RESULT_TYPE result);
    bool onNcHitTest(tagMSG* msg, Q_RESULT_TYPE result);
    bool onDisplayChange(tagMSG* msg, Q_RESULT_TYPE result);
    bool onDpiChanged(tagMSG* msg, Q_RESULT_TYPE result);
    bool onSize(tagMSG* msg, Q_RESULT_TYPE result);
    bool onGetMinMaxInfo(tagMSG* msg, Q_RESULT_TYPE result);
    bool onLButtonUp(tagMSG* msg, Q_RESULT_TYPE result);
    bool onNcMouseLeave(tagMSG* msg, Q_RESULT_TYPE result);
    bool onEraseBackground(tagMSG* msg, Q_RESULT_TYPE result);
    bool onNcUahDraw(tagMSG* msg, Q_RESULT_TYPE result);
    bool onMouseMove(tagMSG* msg, Q_RESULT_TYPE result);
    bool onNcLButtonDown(tagMSG* msg, Q_RESULT_TYPE result);
    bool onNcLButtonUp(tagMSG* msg, Q_RESULT_TYPE result);
    bool onNcLButtonDblClk(tagMSG* msg, Q_RESULT_TYPE result);
    bool onEnterSizeMove(tagMSG* msg, Q_RESULT_TYPE result);
    bool onExitSizeMove(tagMSG* msg, Q_RESULT_TYPE result);
    bool onStyleChanged(tagMSG* msg, Q_RESULT_TYPE result);
    bool onSetFocus(tagMSG* msg, Q_RESULT_TYPE result);
    bool onActivate(tagMSG* msg, Q_RESULT_TYPE result);
    bool onThemeChanged(tagMSG* msg, Q_RESULT_TYPE result);
    bool onSettingChange(tagMSG* msg, Q_RESULT_TYPE result);
    bool onNcActivate(tagMSG* msg, Q_RESULT_TYPE result);
    bool onWindowPosChanging(tagMSG* msg, Q_RESULT_TYPE result);
    bool onPowerBroadcast(tagMSG* msg, Q_RESULT_TYPE result);

    bool isOutOfWidget(QWidget* widget);
    QMenu* createPopupMenu() override;

    void forceRedraw();
    void redrawWindowNow(HWND__* hwnd, bool eraseBackground = false);
    bool scheduleDarkModeRefresh();
    QPoint systemMenuAnchor() const;
    void updateWindowDpr(float dpr, QRect rect, WId wid);
    void syncWindowHintGeometry();
    bool isTitleBarChrome(const QWidget* widget) const;
    void rememberFocusedWidget(QWidget* widget);
    void restoreClientFocus();
    void queueRestoreClientFocus();

private slots:
    void onTitleBarDestroyed();
#endif  // Q_OS_WIN
private:
    struct StyledWindowPrivate;
    StyledWindowPrivate* d;
};

}  // namespace ads
#endif  // ADS_STYLED_WINDOW_H
