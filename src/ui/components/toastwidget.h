#pragma once
#include <QWidget>

class QPropertyAnimation;
class QSequentialAnimationGroup;

// ============================================================
// ToastWidget —— 单条 Toast 轻提示控件
//   · 无边框、透明背景的浮层控件
//   · 动画流程：渐入(200ms) → 停留(1500ms) → 渐出(300ms)
//   · 消失后发出 finished 信号，通知 ToastManager 显示下一条
// ============================================================
class ToastWidget : public QWidget {
    Q_OBJECT
    // 声明 toastOpacity 属性，供 QPropertyAnimation 驱动
    // 注意：不能用 "opacity" 因为 QWidget 已有同名属性
    Q_PROPERTY(qreal toastOpacity READ toastOpacity WRITE setToastOpacity)

public:
    explicit ToastWidget(const QString& message, QWidget* parent = nullptr);

    // 开始播放动画（渐入→停留→渐出）
    void popup();

    qreal toastOpacity() const { return opacity_; }
    void  setToastOpacity(qreal val);

signals:
    void finished();  // 动画结束，通知 ToastManager 显示下一条

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void startAnimation();

    QString  message_;
    qreal    opacity_ = 0.0;
};
