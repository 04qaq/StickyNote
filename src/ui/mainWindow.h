#pragma once

#include <QWidget>

class TitleBar;
class Sidebar;
class WindowHelper;
class QTextEdit;
class QPaintEvent;
class QResizeEvent;
class QCloseEvent;
class QChangeEvent;

// ============================================================
// MainWindow —— 主窗口容器
//   · 无边框 + 圆角 + 投影
//   · 左右分栏布局（侧边栏 + 内容区）
//   · 窗口位置与尺寸持久化（QSettings）
//   · 委托 TitleBar 处理标题栏交互
//   · 委托 WindowHelper 处理边缘缩放
// ============================================================
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void initUI();
    void initConnections();
    void applyStyle();

    void saveWindowState();// 保存窗口状态
    void restoreWindowState();// 恢复窗口状态

    // ---- 子组件 ----
    TitleBar*     m_titleBar    = nullptr;// 标题栏
    Sidebar*      m_sidebar     = nullptr;// 侧边栏
    QTextEdit*    m_textEdit    = nullptr;// 内容区占位（二期替换为 NoteListView）
    WindowHelper* m_winHelper   = nullptr;// 窗口辅助, 负责边缘缩放

    // ---- 阴影边距 ----
    static constexpr int SHADOW_MARGIN = 10;
    // ---- 圆角半径 ----
    static constexpr int CORNER_RADIUS = 8;
};