#include "toastmanager.h"
#include "toastwidget.h"
#include <QWidget>

ToastManager* ToastManager::instance() {
    // C++11 保证局部静态变量线程安全初始化
    static ToastManager inst;
    return &inst;
}

ToastManager::ToastManager(QObject* parent)
    : QObject(parent)
{
}

void ToastManager::setParentWidget(QWidget* parent) {
    parent_widget_ = parent;
}

void ToastManager::show(const QString& message) {
    queue_.enqueue(message);
    showNext();
}

void ToastManager::showNext() {
    // 如果正在显示或队列为空，直接返回
    if (is_showing_ || queue_.isEmpty()) return;
    if (!parent_widget_) return;

    is_showing_ = true;

    // 创建 Toast，父窗口设为 nullptr（独立浮层），但定位相对 parent_widget_
    auto* toast = new ToastWidget(queue_.dequeue(), nullptr);

    // ── 定位到父窗口底部居中 ──────────────────────────────────
    // 将父窗口的底部中心点转换为全局坐标
    QPoint parent_global = parent_widget_->mapToGlobal(QPoint(0, 0));
    int x = parent_global.x() + (parent_widget_->width()  - toast->width())  / 2;
    int y = parent_global.y() +  parent_widget_->height() - toast->height() - 40;
    toast->move(x, y);

    // Toast 消失后，标记为空闲，并尝试显示下一条
    connect(toast, &ToastWidget::finished, this, [this]() {
        is_showing_ = false;
        showNext();
    });

    toast->popup();
}
