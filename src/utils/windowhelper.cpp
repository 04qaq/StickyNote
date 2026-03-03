#include "windowhelper.h"

#include <QWidget>
#include <QMouseEvent>
#include <QCursor>
#include <QApplication>

WindowHelper::WindowHelper(QWidget* target, QObject* parent)
    : QObject(parent)
    , m_target(target)
{
    Q_ASSERT(target);
    // 开启鼠标追踪，以便在不按下按钮时也能收到 mouseMoveEvent
    target->setMouseTracking(true);
    // 安装事件过滤器
    target->installEventFilter(this);
}

bool WindowHelper::eventFilter(QObject* obj, QEvent* event)
{
    if (obj != m_target)
        return QObject::eventFilter(obj, event);

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        handleMousePress(static_cast<QMouseEvent*>(event));
        break;
    case QEvent::MouseMove:
        handleMouseMove(static_cast<QMouseEvent*>(event));
        break;
    case QEvent::MouseButtonRelease:
        handleMouseRelease(static_cast<QMouseEvent*>(event));
        break;
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}

// ----------------------------------------------------------------
// 边缘检测
// ----------------------------------------------------------------
WindowHelper::Edge WindowHelper::hitTest(const QPoint& pos) const
{
    const QRect rect = m_target->rect();
    int edge = None;

    if (pos.x() <= EDGE_MARGIN)                          edge |= Left;
    if (pos.x() >= rect.width() - EDGE_MARGIN)           edge |= Right;
    if (pos.y() <= EDGE_MARGIN)                          edge |= Top;
    if (pos.y() >= rect.height() - EDGE_MARGIN)          edge |= Bottom;

    return static_cast<Edge>(edge);
}

void WindowHelper::updateCursor(Edge edge)
{
    switch (edge) {
    case Left:
    case Right:
        m_target->setCursor(Qt::SizeHorCursor);
        break;
    case Top:
    case Bottom:
        m_target->setCursor(Qt::SizeVerCursor);
        break;
    case TopLeft:
    case BottomRight:
        m_target->setCursor(Qt::SizeFDiagCursor);
        break;
    case TopRight:
    case BottomLeft:
        m_target->setCursor(Qt::SizeBDiagCursor);
        break;
    default:
        m_target->setCursor(Qt::ArrowCursor);
        break;
    }
}

// ----------------------------------------------------------------
// 鼠标事件处理
// ----------------------------------------------------------------
void WindowHelper::handleMousePress(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return;

    Edge edge = hitTest(event->pos());
    if (edge != None) {
        m_resizing  = true;
        m_resizeDir = edge;
        m_startPos  = event->globalPosition().toPoint();
        m_startGeom = m_target->geometry();
        event->accept();
    }
}

void WindowHelper::handleMouseMove(QMouseEvent* event)
{
    if (!m_resizing) {
        // 未缩放时更新光标
        updateCursor(hitTest(event->pos()));
        return;
    }

    if (!(event->buttons() & Qt::LeftButton)) {
        m_resizing = false;
        return;
    }

    QPoint delta = event->globalPosition().toPoint() - m_startPos;
    QRect  geom  = m_startGeom;
    QSize  minSz = m_target->minimumSize();

    if (m_resizeDir & Left) {
        int newLeft = geom.left() + delta.x();
        int newWidth = geom.right() - newLeft + 1;
        if (newWidth >= minSz.width()) {
            geom.setLeft(newLeft);
        }
    }
    if (m_resizeDir & Right) {
        int newWidth = geom.width() + delta.x();
        if (newWidth >= minSz.width()) {
            geom.setWidth(newWidth);
        }
    }
    if (m_resizeDir & Top) {
        int newTop = geom.top() + delta.y();
        int newHeight = geom.bottom() - newTop + 1;
        if (newHeight >= minSz.height()) {
            geom.setTop(newTop);
        }
    }
    if (m_resizeDir & Bottom) {
        int newHeight = geom.height() + delta.y();
        if (newHeight >= minSz.height()) {
            geom.setHeight(newHeight);
        }
    }

    m_target->setGeometry(geom);
    event->accept();
}

void WindowHelper::handleMouseRelease(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_resizing) {
        m_resizing  = false;
        m_resizeDir = None;
        updateCursor(None);
        event->accept();
    }
}
