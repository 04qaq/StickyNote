#pragma once
#include <QWidget>
#include <QList>
#include <QColor>

class ColorSelector : public QWidget {
    Q_OBJECT
public:
    explicit ColorSelector(QWidget* parent = nullptr);

    // 设置当前选中颜色（编辑便签时回显已保存的颜色）
    void setCurrentColor(const QString& colorHex);

    // 获取当前选中颜色的 Hex 字符串，如 "#ffeaa7"
    QString currentColor() const;

signals:
    // 用户点击某个色块时发出，携带颜色的 Hex 字符串
    void colorSelected(const QString& colorHex);

protected:
    // Qt 绘图事件：每次需要重绘时自动调用，在这里画所有圆形色块
    void paintEvent(QPaintEvent* event) override;

    // 鼠标点击事件：判断点击了哪个色块，更新选中状态并发出信号
    void mousePressEvent(QMouseEvent* event) override;

    // 鼠标移动事件：更新 hoveredIndex_，触发重绘实现 hover 高亮
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    // 预设颜色列表（8 种便签背景色）
    QList<QColor> colors_ = {
        QColor("#FFEAA7"), // 黄色（默认）
        QColor("#A8E6CF"), // 绿色
        QColor("#FFB3BA"), // 粉色
        QColor("#B3D9FF"), // 蓝色
        QColor("#E8D5FF"), // 紫色
        QColor("#FFD4A8"), // 橙色
        QColor("#FFFFFF"), // 白色
        QColor("#F0F0F0"), // 灰色
    };

    QString currentColor_;      // 当前选中颜色的 Hex 字符串
    int     hoveredIndex_ = -1; // 当前鼠标悬停的色块索引，-1 表示无悬停

    // 计算第 index 个色块的矩形区域（供绘制和点击检测共用）
    QRect colorRect(int index) const;

    // 根据鼠标坐标找到对应的色块索引，找不到返回 -1
    int hitTest(const QPoint& pos) const;
};
