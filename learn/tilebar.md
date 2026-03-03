# TileBar（标题栏）实现指导

## 一、目标效果

替换 `MainWindow::initUI()` 中的标题栏占位 `QWidget`，实现一个完整的自定义标题栏，包含：

- 左侧：应用图标 + 应用名称
- 右侧：最小化 / 最大化(还原) / 关闭 三个窗口控制按钮
- 整体区域支持**双击最大化/还原**
- 与 `MainWindow` 通过信号通信，不直接操作父窗口

---

## 二、类职责划分

| 类 | 文件 | 职责 |
|----|------|------|
| `TileBar` | `src/ui/tilebar.h/.cpp` | 标题栏 UI + 发出窗口控制信号 |
| `MainWindow` | `src/ui/mainwindow.h/.cpp` | 接收信号，执行最小化/最大化/关闭操作 |

> **原则**：`TileBar` 不持有父窗口指针，不直接调用 `parentWidget()->close()` 等，
> 而是通过信号通知 `MainWindow`，保持解耦。

---

## 三、TileBar 类设计

### 3.1 头文件 `src/ui/tilebar.h`

```cpp
#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QMouseEvent;

// ============================================================
// TileBar —— 自定义标题栏
//   · 显示应用图标和标题
//   · 提供最小化 / 最大化(还原) / 关闭 按钮
//   · 双击标题栏区域触发最大化/还原
//   · 通过信号与 MainWindow 通信，不直接操作父窗口
// ============================================================
class TileBar : public QWidget
{
    Q_OBJECT

public:
    explicit TileBar(QWidget* parent = nullptr);

    // 当窗口状态变化时，由 MainWindow 调用，用于切换最大化按钮图标
    void updateMaximizeButton(bool isMaximized);

signals:
    void minimizeRequested();   // 请求最小化
    void maximizeRequested();   // 请求最大化 / 还原
    void closeRequested();      // 请求关闭

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void initUI();
    void applyStyle();

    QLabel*      m_iconLabel    = nullptr;  // 应用图标
    QLabel*      m_titleLabel   = nullptr;  // 应用标题
    QPushButton* m_minBtn       = nullptr;  // 最小化按钮
    QPushButton* m_maxBtn       = nullptr;  // 最大化/还原按钮
    QPushButton* m_closeBtn     = nullptr;  // 关闭按钮
};
```

### 3.2 函数说明

| 函数 | 作用 |
|------|------|
| `TileBar(parent)` | 调用 `initUI()` 和 `applyStyle()`，设置固定高度 |
| `initUI()` | 创建所有子控件，设置布局，连接按钮信号 |
| `applyStyle()` | 通过 `setStyleSheet` 设置标题栏背景色、按钮悬停效果 |
| `updateMaximizeButton(bool)` | 根据当前是否最大化，切换 `m_maxBtn` 的图标或文字 |
| `mouseDoubleClickEvent` | 双击时发出 `maximizeRequested()` 信号 |

---

## 四、完整实现代码

### 4.1 `src/ui/tilebar.h`（完整）

```cpp
#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QMouseEvent;

class TileBar : public QWidget
{
    Q_OBJECT

public:
    explicit TileBar(QWidget* parent = nullptr);
    void updateMaximizeButton(bool isMaximized);

signals:
    void minimizeRequested();
    void maximizeRequested();
    void closeRequested();

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void initUI();
    void applyStyle();

    QLabel*      m_iconLabel  = nullptr;
    QLabel*      m_titleLabel = nullptr;
    QPushButton* m_minBtn     = nullptr;
    QPushButton* m_maxBtn     = nullptr;
    QPushButton* m_closeBtn   = nullptr;
};
```

### 4.2 `src/ui/tilebar.cpp`（完整）

```cpp
#include "tilebar.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMouseEvent>

TileBar::TileBar(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(36);
    setObjectName("TileBar");
    initUI();
    applyStyle();
}

void TileBar::initUI()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 4, 0);
    layout->setSpacing(0);

    // 应用图标（可替换为实际图标）
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(20, 20);
    m_iconLabel->setObjectName("iconLabel");
    layout->addWidget(m_iconLabel);

    layout->addSpacing(8);

    // 应用标题
    m_titleLabel = new QLabel("便签管理器", this);
    m_titleLabel->setObjectName("titleLabel");
    layout->addWidget(m_titleLabel);

    layout->addStretch();  // 弹性空间，将按钮推到右侧

    // 最小化按钮
    m_minBtn = new QPushButton("─", this);
    m_minBtn->setObjectName("minBtn");
    m_minBtn->setFixedSize(46, 36);
    m_minBtn->setToolTip("最小化");
    layout->addWidget(m_minBtn);

    // 最大化/还原按钮
    m_maxBtn = new QPushButton("□", this);
    m_maxBtn->setObjectName("maxBtn");
    m_maxBtn->setFixedSize(46, 36);
    m_maxBtn->setToolTip("最大化");
    layout->addWidget(m_maxBtn);

    // 关闭按钮
    m_closeBtn = new QPushButton("✕", this);
    m_closeBtn->setObjectName("closeBtn");
    m_closeBtn->setFixedSize(46, 36);
    m_closeBtn->setToolTip("关闭");
    layout->addWidget(m_closeBtn);

    // 连接信号
    connect(m_minBtn,   &QPushButton::clicked, this, &TileBar::minimizeRequested);
    connect(m_maxBtn,   &QPushButton::clicked, this, &TileBar::maximizeRequested);
    connect(m_closeBtn, &QPushButton::clicked, this, &TileBar::closeRequested);
}

void TileBar::applyStyle()
{
    setStyleSheet(R"(
        QWidget#TileBar {
            background-color: #1890FF;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
        }
        QLabel#titleLabel {
            color: #ffffff;
            font-size: 14px;
            font-weight: bold;
            background: transparent;
        }
        QPushButton#minBtn, QPushButton#maxBtn {
            color: #ffffff;
            background: transparent;
            border: none;
            font-size: 14px;
        }
        QPushButton#minBtn:hover, QPushButton#maxBtn:hover {
            background-color: rgba(255, 255, 255, 0.2);
        }
        QPushButton#closeBtn {
            color: #ffffff;
            background: transparent;
            border: none;
            font-size: 13px;
            border-top-right-radius: 8px;
        }
        QPushButton#closeBtn:hover {
            background-color: #E81123;
        }
    )");
}

void TileBar::updateMaximizeButton(bool isMaximized)
{
    if (isMaximized) {
        m_maxBtn->setText("❐");       // 还原图标
        m_maxBtn->setToolTip("还原");
    } else {
        m_maxBtn->setText("□");       // 最大化图标
        m_maxBtn->setToolTip("最大化");
    }
}

void TileBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    emit maximizeRequested();
}
```

---

## 五、在 MainWindow 中集成

### 5.1 修改 `mainwindow.h`

```cpp
// 新增前向声明
class TileBar;

// private 区域新增成员
TileBar* m_tileBar = nullptr;

// private slots 新增槽函数
private slots:
    void onMinimizeRequested();
    void onMaximizeRequested();
    void onCloseRequested();
```

### 5.2 修改 `mainwindow.cpp` 的 `initUI()`

将原来的标题栏占位代码：

```cpp
// 删除这段旧代码
QWidget* titleBar = new QWidget(contentWidget);
titleBar->setObjectName("titleBar");
titleBar->setFixedHeight(TITLE_BAR_HEIGHT);
QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
titleLayout->setContentsMargins(12, 0, 12, 0);
QLabel* titleLabel = new QLabel("便签管理器", titleBar);
titleLabel->setObjectName("titleLabel");
titleLayout->addWidget(titleLabel);
titleLayout->addStretch();
contentLayout->addWidget(titleBar);
```

替换为：

```cpp
// 新增头文件引用（文件顶部）
#include "tilebar.h"

// initUI() 中替换标题栏部分
m_tileBar = new TileBar(contentWidget);
contentLayout->addWidget(m_tileBar);

// 连接信号
connect(m_tileBar, &TileBar::minimizeRequested, this, &MainWindow::onMinimizeRequested);
connect(m_tileBar, &TileBar::maximizeRequested, this, &MainWindow::onMaximizeRequested);
connect(m_tileBar, &TileBar::closeRequested,    this, &MainWindow::onCloseRequested);
```

### 5.3 新增槽函数实现

```cpp
void MainWindow::onMinimizeRequested()
{
    showMinimized();
}

void MainWindow::onMaximizeRequested()
{
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
}

void MainWindow::onCloseRequested()
{
    close();  // 会触发 closeEvent，自动保存窗口状态
}
```

### 5.4 在 `changeEvent` 中同步最大化按钮状态

在已有的 `changeEvent` 中，窗口状态变化时通知 `TileBar`：

```cpp
void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange) {
        // 同步最大化按钮图标
        if (m_tileBar) {
            m_tileBar->updateMaximizeButton(isMaximized());
        }

        QVBoxLayout* outerLayout = qobject_cast<QVBoxLayout*>(layout());
        if (outerLayout) {
            int margin = isMaximized() ? 0 : SHADOW_MARGIN;
            outerLayout->setContentsMargins(margin, margin, margin, margin);
        }
        update();
    }
    QWidget::changeEvent(event);
}
```

### 5.5 同时删除 `mainwindow.cpp` 中 `setStyleSheet` 里的旧标题栏样式

原来 `initUI()` 末尾的 `setStyleSheet` 中有 `QWidget#titleBar` 和 `QLabel#titleLabel` 的样式，
这两个已移入 `TileBar::applyStyle()`，**需要从 `MainWindow` 的样式表中删除**，避免冲突。

---

## 六、拖拽移动的注意事项

`MainWindow` 的 `mousePressEvent` 通过判断 Y 坐标来决定是否触发拖拽：

```cpp
int titleBottom = SHADOW_MARGIN + TITLE_BAR_HEIGHT;
if (event->position().y() <= titleBottom) { ... }
```

`TileBar` 内部的按钮点击事件**不会冒泡到 `MainWindow`**，因此拖拽与按钮点击不会冲突。
但双击事件在 `TileBar::mouseDoubleClickEvent` 中处理，`MainWindow` 的 `mousePressEvent`
不会收到双击，两者互不干扰。

---

## 七、实现步骤（推荐顺序）

1. 按照第四节完整填写 `tilebar.h` 和 `tilebar.cpp`
2. 按照第五节修改 `mainwindow.h`（新增前向声明、成员变量、槽函数声明）
3. 修改 `mainwindow.cpp`：替换 `initUI()` 中的标题栏代码，添加槽函数实现，更新 `changeEvent`
4. 删除 `mainwindow.cpp` 中 `setStyleSheet` 里的旧标题栏样式
5. 编译运行，验证三个按钮功能和双击最大化效果
