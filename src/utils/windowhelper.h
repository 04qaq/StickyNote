#pragma once

#include <QObject>
#include <QPoint>

class QWidget;
class QMouseEvent;
class QHoverEvent;
class QEvent;

// ============================================================
// WindowHelper —— 无边框窗口边缘缩放辅助类
//   · 安装到目标窗口，监听鼠标事件
//   · 检测鼠标是否在窗口四边/四角，改变光标形状
//   · 实现拖拽边缘调整窗口大小
// ============================================================
class WindowHelper : public QObject
{
    Q_OBJECT

public:
    // 边缘检测范围（像素）
    static constexpr int EDGE_MARGIN = 6;

    explicit WindowHelper(QWidget* target, QObject* parent = nullptr);
    ~WindowHelper() override = default;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    // 边缘方向枚举
    enum Edge {
        None        = 0x00,
        Left        = 0x01,
        Right       = 0x02,
        Top         = 0x04,
        Bottom      = 0x08,
        TopLeft     = Top | Left,
        TopRight    = Top | Right,
        BottomLeft  = Bottom | Left,
        BottomRight = Bottom | Right,
    };

    // 根据鼠标位置计算所在边缘
    Edge hitTest(const QPoint& pos) const;

    // 根据边缘设置光标形状
    void updateCursor(Edge edge);

    void handleMousePress(QMouseEvent* event);
    void handleMouseMove(QMouseEvent* event);
    void handleMouseRelease(QMouseEvent* event);

    QWidget* m_target    = nullptr;
    Edge     m_resizeDir = None;
    bool     m_resizing  = false;
    QPoint   m_startPos;    // 全局坐标起始点
    QRect    m_startGeom;   // 缩放开始时的窗口几何
};
