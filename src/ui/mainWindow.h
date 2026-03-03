#pragma once

#include <QWidget>
#include <QPoint>

class TileBar;
class SideBar;
class QPaintEvent;
class QResizeEvent;
class QCloseEvent;
class QMouseEvent;
class QEvent;

// ============================================================
// MainWindow —— 无边框主窗口
//   · 无系统边框 + 圆角 + 投影
//   · 标题栏区域支持拖拽移动
//   · 窗口位置与尺寸持久化（QSettings）
// ============================================================
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

public slots:
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

private:
    void initUI();
    void connectSignals();

    void saveWindowState();
    void restoreWindowState();

    // ---- 拖拽移动 ----
    QPoint m_dragStartPos;
    bool   m_isDragging = false;

    // ---- 阴影边距 ----
    static constexpr int SHADOW_MARGIN = 10;
    // ---- 圆角半径 ----
    static constexpr int CORNER_RADIUS = 8;
    // ---- 标题栏高度（可拖拽区域）----
    static constexpr int TITLE_BAR_HEIGHT = 36;

    TileBar* tile_bar_ = nullptr;
    SideBar *sidebar_ = nullptr;
    QWidget* content_widget_ = nullptr;
};