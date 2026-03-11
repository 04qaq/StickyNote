#pragma once

#include <QWidget>
#include <QPoint>
#include <QVBoxLayout>
#include <QModelIndex>
#include <QSystemTrayIcon>
#include "../utils/windowhelper.h"



class TileBar;
class SideBar;
class NoteListView;
class NoteListModel;
class NoteCardDelegate;
class NoteFilterProxyModel;
class NoteController;

class QMenu;
class QPushButton;
class QPaintEvent;
class QResizeEvent;
class QCloseEvent;
class QMouseEvent;
class QEvent;

// ==========================================================================================
//第一阶段中拦截子组件的鼠标移动事件，可能会导致子组件的鼠标移动事件无法触发，出现问题先看这里
// ===========================================================================================



// ============================================================
// MainWindow —— 无边框主窗口
//   · 无系统边框 + 圆角 + 投影
//   · 标题栏区域支持拖拽移动
//   · 窗口位置与尺寸持久化（QSettings）
//   · 系统托盘：关闭时隐藏到托盘，双击恢复
//   · 撤销/重做：Ctrl+Z / Ctrl+Y 快捷键
// ============================================================
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

public slots:
    // 窗口控制（属于 UI 层，保留在 MainWindow）
    void onMinimizeRequested();
    void onMaximizeRequested();
    void onCloseRequested();



protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;


private:

    void initUI();
    void connectSignals();
    void initTray();          // 初始化系统托盘
    void initShortcuts();     // 初始化快捷键

    void saveWindowState();
    void restoreWindowState();

    // ---- 拖拽移动 ----
    QPoint m_dragStartPos;
    bool   m_isDragging = false;

    // ---- 边缘缩放 ----
    WindowHelper* window_helper_ = nullptr;
    static constexpr int RESIZE_MARGIN = 6;  // 缩放检测区域宽度


    // ---- 阴影边距 ----
    static constexpr int SHADOW_MARGIN = 10;
    // ---- 圆角半径 ----
    static constexpr int CORNER_RADIUS = 8;
    // ---- 标题栏高度（可拖拽区域）----
    static constexpr int TITLE_BAR_HEIGHT = 36;

    TileBar*          tile_bar_       = nullptr;
    SideBar*          sidebar_        = nullptr;
    QWidget*          content_widget_ = nullptr;
    QVBoxLayout*      content_layout_ = nullptr;
    NoteListView*     note_list_view_ = nullptr;
    NoteListModel*    note_model_     = nullptr;
    NoteCardDelegate* note_delegate_  = nullptr;
    NoteFilterProxyModel* note_proxy_model_ = nullptr;

    // Controller 层：负责协调 View 信号与 Model 数据操作
    NoteController*   note_controller_ = nullptr;

    // ---- 系统托盘 ----
    QSystemTrayIcon*  tray_icon_  = nullptr;
    QMenu*            tray_menu_  = nullptr;
};

