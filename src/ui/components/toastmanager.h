#pragma once
#include <QObject>
#include <QQueue>

class QWidget;
class ToastWidget;

// ============================================================
// ToastManager —— Toast 队列管理器（单例）
//   · 维护消息队列，保证 Toast 不重叠、依次展示
//   · 使用前必须调用 setParentWidget() 设置父窗口
//   · 用法：ToastManager::instance()->show("操作成功");
// ============================================================
class ToastManager : public QObject {
    Q_OBJECT
public:
    static ToastManager* instance();

    // 设置父窗口（Toast 相对父窗口定位）
    void setParentWidget(QWidget* parent);

    // 显示一条 Toast（加入队列，自动排队）
    void show(const QString& message);

private:
    explicit ToastManager(QObject* parent = nullptr);

    // 从队列取出下一条并显示
    void showNext();

    QWidget*        parent_widget_ = nullptr;
    QQueue<QString> queue_;
    bool            is_showing_    = false;
};
