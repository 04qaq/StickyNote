#pragma once

#include <QWidget>
#include <QPoint>

class QLabel;
class QPushButton;
class QMouseEvent;
class QResizeEvent;

// ============================================================
// TitleBar —— 自定义标题栏
//   · 应用图标 + 标题文字
//   · 最小化 / 最大化(还原) / 关闭 按钮
//   · 拖拽移动父窗口
//   · 双击最大化 / 还原
// ============================================================
class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget* parent = nullptr);
    ~TitleBar() override = default;

    // 设置标题文字
    void setTitle(const QString& title);

    // 更新最大化按钮图标（由主窗口在 changeEvent 中调用）
    void updateMaxButton(bool isMaximized);

signals:
    void minimizeRequested();
    void maximizeRequested();
    void closeRequested();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void initUI();
    void applyStyle();

    QLabel*      m_titleLabel  = nullptr;
    QPushButton* m_minButton   = nullptr;
    QPushButton* m_maxButton   = nullptr;
    QPushButton* m_closeButton = nullptr;

    QPoint m_dragStartPos;
    bool   m_isDragging = false;
};
