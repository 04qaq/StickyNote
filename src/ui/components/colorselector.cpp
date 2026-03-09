#include "colorselector.h"
#include <QPainter>
#include <QMouseEvent>

// 色块的圆形直径（像素）
static constexpr int CIRCLE_SIZE   = 24;
// 色块之间以及边缘的间距（像素）
static constexpr int CIRCLE_MARGIN = 8;
// 每行放几个色块
static constexpr int COLS          = 4;

// ── 构造函数 ──────────────────────────────────────────────────
ColorSelector::ColorSelector(QWidget* parent) : QWidget(parent)
{
    currentColor_ = "#ffeaa7"; // 默认选中黄色（统一小写）

    // 根据颜色数量和布局参数，计算 Widget 的固定尺寸
    int rows = (colors_.size() + COLS - 1) / COLS; // 向上取整
    int w = COLS * (CIRCLE_SIZE + CIRCLE_MARGIN) + CIRCLE_MARGIN;
    int h = rows * (CIRCLE_SIZE + CIRCLE_MARGIN) + CIRCLE_MARGIN;
    setFixedSize(w, h);

    // 开启鼠标追踪：不按下鼠标时也能收到 mouseMoveEvent，hover 效果必须开启
    setMouseTracking(true);
}

// ── 设置当前选中颜色（编辑便签时回显） ────────────────────────
void ColorSelector::setCurrentColor(const QString& colorHex)
{
    currentColor_ = colorHex.toLower(); // 统一转小写，方便后续比较
    update(); // 触发重绘，让选中圆圈显示在正确的色块上
}

// ── 获取当前选中颜色 ──────────────────────────────────────────
QString ColorSelector::currentColor() const
{
    return currentColor_;
}

// ── 计算第 index 个色块的矩形区域 ────────────────────────────
// 把线性索引转换成二维坐标，再算出像素位置
QRect ColorSelector::colorRect(int index) const
{
    int col = index % COLS;
    int row = index / COLS;
    int x = CIRCLE_MARGIN + col * (CIRCLE_SIZE + CIRCLE_MARGIN);
    int y = CIRCLE_MARGIN + row * (CIRCLE_SIZE + CIRCLE_MARGIN);
    return QRect(x, y, CIRCLE_SIZE, CIRCLE_SIZE);
}

// ── 根据鼠标坐标找到对应的色块索引 ───────────────────────────
int ColorSelector::hitTest(const QPoint& pos) const
{
    for (int i = 0; i < colors_.size(); ++i) {
        if (colorRect(i).contains(pos)) return i;
    }
    return -1;
}

// ── 绘图事件：每次重绘时自动调用 ─────────────────────────────
void ColorSelector::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    // 开启抗锯齿，让圆形边缘平滑，不出现锯齿
    painter.setRenderHint(QPainter::Antialiasing);

    for (int i = 0; i < colors_.size(); ++i) {
        QRect  rect  = colorRect(i);
        QColor color = colors_[i];

        // 第一层：绘制圆形色块背景
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(rect);

        // 第二层：选中状态 —— 在色块外侧画一圈深色边框
        if (color.name() == currentColor_) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor("#333333"), 2));
            // adjusted(-3,-3,3,3) 让边框比色块大 3px，形成外圈效果
            painter.drawEllipse(rect.adjusted(-3, -3, 3, 3));
        }

        // 第三层：hover 状态 —— 叠加半透明黑色遮罩，让颜色变暗给用户反馈
        if (i == hoveredIndex_) {
            painter.setBrush(QColor(0, 0, 0, 40)); // 黑色，透明度 40/255
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(rect);
        }
    }
}

// ── 鼠标点击事件：选中颜色 ───────────────────────────────────
void ColorSelector::mousePressEvent(QMouseEvent* event)
{
    int idx = hitTest(event->position().toPoint());
    if (idx >= 0) {
        currentColor_ = colors_[idx].name(); // name() 返回 "#rrggbb" 格式（小写）
        update();                             // 触发重绘，显示新的选中圆圈
        emit colorSelected(currentColor_);   // 通知外部颜色已改变
    }
}

// ── 鼠标移动事件：更新 hover 高亮 ────────────────────────────
void ColorSelector::mouseMoveEvent(QMouseEvent* event)
{
    int idx = hitTest(event->position().toPoint());
    if (idx != hoveredIndex_) {
        hoveredIndex_ = idx;
        update(); // 只有 hoveredIndex_ 变化时才重绘，避免不必要的绘制
    }
}
