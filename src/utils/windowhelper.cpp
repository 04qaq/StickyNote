#include "windowhelper.h"
#include <QCursor>

WindowHelper::WindowHelper(QWidget* window, int shadowMargin, int resizeMargin)
    : window_(window)
    , shadowMargin_(shadowMargin)
    , resizeMargin_(resizeMargin)
{
}

// ----------------------------------------------------------------
// hitTest —— 检测鼠标位置对应的边缘方向
//   pos：相对窗口自身的鼠标坐标（来自 event->position().toPoint()）
// ----------------------------------------------------------------
ResizeEdge WindowHelper::hitTest(const QPoint& pos) const
{
    // 实际内容矩形（去掉阴影区域）
    QRect content = window_->rect().adjusted(
        shadowMargin_, shadowMargin_,
        -shadowMargin_, -shadowMargin_
    );

    int x = pos.x();
    int y = pos.y();

    // 判断是否在各边缘区域内
    bool onLeft   = (x >= content.left()   - resizeMargin_) &&
                    (x <= content.left()   + resizeMargin_);
    bool onRight  = (x >= content.right()  - resizeMargin_) &&
                    (x <= content.right()  + resizeMargin_);
    bool onTop    = (y >= content.top()    - resizeMargin_) &&
                    (y <= content.top()    + resizeMargin_);
    bool onBottom = (y >= content.bottom() - resizeMargin_) &&
                    (y <= content.bottom() + resizeMargin_);

    // 必须在内容矩形的扩展范围内（防止检测到窗口外部）
    bool inXRange = (x >= content.left()   - resizeMargin_) &&
                    (x <= content.right()  + resizeMargin_);
    bool inYRange = (y >= content.top()    - resizeMargin_) &&
                    (y <= content.bottom() + resizeMargin_);

    if (!inXRange || !inYRange) return ResizeEdge::None;

    // 角优先于边
    if (onLeft  && onTop)    return ResizeEdge::TopLeft;
    if (onRight && onTop)    return ResizeEdge::TopRight;
    if (onLeft  && onBottom) return ResizeEdge::BottomLeft;
    if (onRight && onBottom) return ResizeEdge::BottomRight;
    if (onLeft)              return ResizeEdge::Left;
    if (onRight)             return ResizeEdge::Right;
    if (onTop)               return ResizeEdge::Top;
    if (onBottom)            return ResizeEdge::Bottom;

    return ResizeEdge::None;
}

// ----------------------------------------------------------------
// updateCursor —— 根据边缘方向设置鼠标光标形状
// ----------------------------------------------------------------
void WindowHelper::updateCursor(ResizeEdge edge, QWidget* target)
{
    QWidget* w = target ? target : window_;
    switch (edge) {
    case ResizeEdge::Left:
    case ResizeEdge::Right:
        w->setCursor(Qt::SizeHorCursor);
        break;
    case ResizeEdge::Top:
    case ResizeEdge::Bottom:
        w->setCursor(Qt::SizeVerCursor);
        break;
    case ResizeEdge::TopLeft:
    case ResizeEdge::BottomRight:
        w->setCursor(Qt::SizeFDiagCursor);
        break;
    case ResizeEdge::TopRight:
    case ResizeEdge::BottomLeft:
        w->setCursor(Qt::SizeBDiagCursor);
        break;
    default:
        w->setCursor(Qt::ArrowCursor);
        break;
    }
}


// ----------------------------------------------------------------
// startResize —— 记录缩放起始状态
// ----------------------------------------------------------------
void WindowHelper::startResize(ResizeEdge edge, const QPoint& globalPos)
{
    resizing_             = true;
    resizeEdge_           = edge;
    resizeStartGlobalPos_ = globalPos;
    // 记录窗口当前的屏幕几何（含阴影）
    resizeStartGeometry_  = window_->frameGeometry();
}

// ----------------------------------------------------------------
// doResize —— 根据鼠标偏移量计算并应用新的窗口几何
// ----------------------------------------------------------------
void WindowHelper::doResize(const QPoint& globalPos)
{
    if (!resizing_) return;

    // 鼠标总偏移量（基于起始位置，避免累积误差）
    QPoint delta = globalPos - resizeStartGlobalPos_;
    QRect  geo   = resizeStartGeometry_;
    QSize  minSz = window_->minimumSize();

    int newLeft   = geo.left();
    int newTop    = geo.top();
    int newRight  = geo.right();
    int newBottom = geo.bottom();

    // 根据缩放方向调整对应边
    switch (resizeEdge_) {
    case ResizeEdge::Left:
        newLeft = geo.left() + delta.x();
        break;
    case ResizeEdge::Right:
        newRight = geo.right() + delta.x();
        break;
    case ResizeEdge::Top:
        newTop = geo.top() + delta.y();
        break;
    case ResizeEdge::Bottom:
        newBottom = geo.bottom() + delta.y();
        break;
    case ResizeEdge::TopLeft:
        newLeft = geo.left() + delta.x();
        newTop  = geo.top()  + delta.y();
        break;
    case ResizeEdge::TopRight:
        newRight = geo.right() + delta.x();
        newTop   = geo.top()   + delta.y();
        break;
    case ResizeEdge::BottomLeft:
        newLeft   = geo.left()   + delta.x();
        newBottom = geo.bottom() + delta.y();
        break;
    case ResizeEdge::BottomRight:
        newRight  = geo.right()  + delta.x();
        newBottom = geo.bottom() + delta.y();
        break;
    default:
        return;
    }

    // 应用最小尺寸约束（防止窗口被拖到比最小尺寸还小）
    if (newRight - newLeft < minSz.width()) {
        if (resizeEdge_ == ResizeEdge::Left   ||
            resizeEdge_ == ResizeEdge::TopLeft ||
            resizeEdge_ == ResizeEdge::BottomLeft) {
            newLeft = newRight - minSz.width();
        } else {
            newRight = newLeft + minSz.width();
        }
    }
    if (newBottom - newTop < minSz.height()) {
        if (resizeEdge_ == ResizeEdge::Top    ||
            resizeEdge_ == ResizeEdge::TopLeft ||
            resizeEdge_ == ResizeEdge::TopRight) {
            newTop = newBottom - minSz.height();
        } else {
            newBottom = newTop + minSz.height();
        }
    }

    // 应用新几何
    window_->setGeometry(newLeft, newTop,
                         newRight  - newLeft,
                         newBottom - newTop);
}

// ----------------------------------------------------------------
// stopResize —— 清除缩放状态，恢复默认光标
// ----------------------------------------------------------------
void WindowHelper::stopResize()
{
    resizing_   = false;
    resizeEdge_ = ResizeEdge::None;
    window_->setCursor(Qt::ArrowCursor);
}
