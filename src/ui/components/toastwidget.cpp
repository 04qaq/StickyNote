#include "toastwidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QPauseAnimation>
#include <QFontMetrics>

ToastWidget::ToastWidget(const QString& message, QWidget* parent)
    : QWidget(parent)
    , message_(message)
{
    // 无边框、透明背景、不抢焦点
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);  // 显示时不抢焦点

    // 根据文字内容计算控件尺寸
    QFont font;
    font.setPointSize(11);
    QFontMetrics fm(font);
    int text_width  = fm.horizontalAdvance(message_);
    int text_height = fm.height();

    // 内边距：水平 24px，垂直 12px
    int w = qMax(text_width + 48, 120);
    int h = text_height + 24;
    setFixedSize(w, h);
}

void ToastWidget::setToastOpacity(qreal val) {
    opacity_ = val;
    update();  // 触发重绘
}

void ToastWidget::popup() {
    // 先显示控件（此时 opacity_=0，不可见）
    QWidget::show();
    startAnimation();
}

void ToastWidget::startAnimation() {
    // ── 渐入动画（200ms）──────────────────────────────────────
    auto* anim_in = new QPropertyAnimation(this, "toastOpacity");
    anim_in->setDuration(200);
    anim_in->setStartValue(0.0);
    anim_in->setEndValue(1.0);
    anim_in->setEasingCurve(QEasingCurve::OutCubic);

    // ── 停留（1500ms）────────────────────────────────────────
    auto* pause = new QPauseAnimation(1500);

    // ── 渐出动画（300ms）──────────────────────────────────────
    auto* anim_out = new QPropertyAnimation(this, "toastOpacity");
    anim_out->setDuration(300);
    anim_out->setStartValue(1.0);
    anim_out->setEndValue(0.0);
    anim_out->setEasingCurve(QEasingCurve::InCubic);

    // ── 串联：渐入 → 停留 → 渐出 ─────────────────────────────
    auto* group = new QSequentialAnimationGroup(this);
    group->addAnimation(anim_in);
    group->addAnimation(pause);
    group->addAnimation(anim_out);

    // 动画结束后：发出 finished 信号，然后销毁自身
    connect(group, &QSequentialAnimationGroup::finished, this, [this]() {
        emit finished();
        deleteLater();
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void ToastWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 整体透明度由 opacity_ 控制
    painter.setOpacity(opacity_);

    // ── 绘制圆角矩形背景（胶囊形）────────────────────────────
    painter.setBrush(QColor(50, 50, 50, 220));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), height() / 2.0, height() / 2.0);

    // ── 绘制文字 ──────────────────────────────────────────────
    painter.setPen(Qt::white);
    QFont font;
    font.setPointSize(11);
    painter.setFont(font);
    painter.drawText(rect(), Qt::AlignCenter, message_);
}
