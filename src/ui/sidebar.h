#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

// ============================================================
// Sidebar —— 左侧分类导航栏（一期占位实现）
//   · 固定宽度 140px
//   · 显示分类列表（全部 / 工作 / 生活 / 学习）
//   · 底部"+ 自定义"按钮占位
// ============================================================
class Sidebar : public QWidget
{
    Q_OBJECT

public:
    explicit Sidebar(QWidget* parent = nullptr);
    ~Sidebar() override = default;

signals:
    void categorySelected(const QString& category);

private:
    void initUI();
    void applyStyle();
};
