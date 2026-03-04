#pragma once

#include <QWidget>
#include <QPoint>
#include <QRect>

// ============================================================
// ResizeEdge —— 缩放方向枚举
// ============================================================
enum class ResizeEdge {
    None,
    Left,
    Right,
    Top,
    Bottom,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

// ============================================================
// WindowHelper —— 无边框窗口缩放辅助类
//   · 检测鼠标位置对应的边缘方向
//   · 更新鼠标光标形状
//   · 执行窗口缩放逻辑
// ============================================================
class WindowHelper {
public:
    explicit WindowHelper(QWidget* window, int shadowMargin = 10, int resizeMargin = 6);

    // 检测鼠标位置对应的边缘方向
    ResizeEdge  hitTest(const QPoint& pos) const;

    // 根据边缘方向更新鼠标光标形状
    // target 为 nullptr 时设置到 window_ 上，否则设置到 target 上
    void        updateCursor(ResizeEdge edge, QWidget* target = nullptr);


    // 开始缩放：记录起始状态
    void        startResize(ResizeEdge edge, const QPoint& globalPos);

    // 执行缩放：根据当前鼠标位置计算并应用新几何
    void        doResize(const QPoint& globalPos);

    // 结束缩放：清除状态，恢复默认光标
    void        stopResize();

    // 是否正在缩放
    bool        isResizing() const { return resizing_; }

private:
    QWidget*    window_;          // 目标窗口
    int         shadowMargin_;    // 阴影边距
    int         resizeMargin_;    // 缩放检测区域宽度

    // 缩放状态
    bool        resizing_               = false;
    ResizeEdge  resizeEdge_             = ResizeEdge::None;
    QPoint      resizeStartGlobalPos_;   // 开始缩放时的全局鼠标位置
    QRect       resizeStartGeometry_;    // 开始缩放时的窗口几何
};
