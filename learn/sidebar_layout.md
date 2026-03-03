# 一期：左右分栏布局实现指导

## 1. 目标效果

在现有无边框主窗口的基础上，将内容区（当前的 `QLabel placeholder`）替换为左右分栏结构：

```
┌─────────────────────────────────────────────┐
│              TileBar（标题栏）                │  ← 已完成
├──────────┬──────────────────────────────────┤
│          │                                  │
│ Sidebar  │       内容区（右侧占位）           │
│ 侧边栏   │                                  │
│ 200px    │                                  │
│          │                                  │
└──────────┴──────────────────────────────────┘
```

---

## 2. 整体架构

```
MainWindow
└── contentWidget (QWidget)
    └── contentLayout (QVBoxLayout)
        ├── tile_bar_  (TileBar)          ← 已有
        └── bodyWidget (QWidget)          ← 新增：承载左右分栏
            └── bodyLayout (QHBoxLayout)  ← 水平布局
                ├── sidebar_ (Sidebar)    ← 新增：左侧侧边栏
                └── contentArea_ (QWidget) ← 新增：右侧内容区占位
```

---

## 3. Sidebar 类设计

### 3.1 职责

`Sidebar` 是左侧导航栏的**占位实现**，一期只需要：
- 固定宽度 200px
- 显示应用 Logo / 名称
- 预留分类列表区域（二期填充）
- 底部预留设置按钮区域

### 3.2 头文件 `src/ui/sidebar.h`

```cpp
#pragma once

#include <QWidget>

class QVBoxLayout;
class QLabel;
class QPushButton;
class QPaintEvent;

// ============================================================
// Sidebar —— 左侧导航侧边栏（一期占位实现）
//   · 固定宽度 200px
//   · 顶部显示应用名称
//   · 中部预留分类列表区域
//   · 底部预留设置按钮
// ============================================================
class Sidebar : public QWidget
{
    Q_OBJECT

public:
    explicit Sidebar(QWidget* parent = nullptr);
    ~Sidebar() override = default;

private:
    void initUi();
    void applyStyleSheet();

    QLabel*      app_name_label_;   // 顶部应用名称
    QWidget*     category_area_;    // 中部分类列表占位区域
    QPushButton* settings_button_;  // 底部设置按钮
};
```

### 3.3 函数职责说明

| 函数 | 职责 |
|------|------|
| `Sidebar(QWidget*)` | 构造函数，设置固定宽度，调用 `initUi()` 和 `applyStyleSheet()` |
| `initUi()` | 创建并布局三个区域：顶部 Logo、中部占位、底部按钮 |
| `applyStyleSheet()` | 设置侧边栏背景色、字体、按钮悬停样式 |

---

## 4. 完整实现代码

### 4.1 `src/ui/sidebar.h`

```cpp
#pragma once

#include <QWidget>

class QVBoxLayout;
class QLabel;
class QPushButton;

class Sidebar : public QWidget
{
    Q_OBJECT

public:
    explicit Sidebar(QWidget* parent = nullptr);
    ~Sidebar() override = default;

private:
    void initUi();
    void applyStyleSheet();

    QLabel*      app_name_label_;
    QWidget*     category_area_;
    QPushButton* settings_button_;
};
```

### 4.2 `src/ui/sidebar.cpp`

```cpp
#include "sidebar.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

Sidebar::Sidebar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("Sidebar");
    setFixedWidth(200);
    initUi();
    applyStyleSheet();
}

void Sidebar::initUi()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ---- 顶部：应用名称 ----
    app_name_label_ = new QLabel("📌 便签管理", this);
    app_name_label_->setObjectName("appNameLabel");
    app_name_label_->setAlignment(Qt::AlignCenter);
    app_name_label_->setFixedHeight(56);
    layout->addWidget(app_name_label_);

    // ---- 中部：分类列表占位 ----
    category_area_ = new QWidget(this);
    category_area_->setObjectName("categoryArea");
    layout->addWidget(category_area_, 1);   // stretch = 1，占满剩余空间

    // ---- 底部：设置按钮 ----
    settings_button_ = new QPushButton("⚙  设置", this);
    settings_button_->setObjectName("settingsButton");
    settings_button_->setFixedHeight(44);
    layout->addWidget(settings_button_);
}

void Sidebar::applyStyleSheet()
{
    setStyleSheet(R"(
        QWidget#Sidebar {
            background-color: #2C3E50;
            border-bottom-left-radius: 8px;
        }
        QLabel#appNameLabel {
            color: #ECF0F1;
            font-size: 15px;
            font-weight: bold;
            background-color: #243342;
        }
        QWidget#categoryArea {
            background-color: transparent;
        }
        QPushButton#settingsButton {
            color: #BDC3C7;
            background-color: transparent;
            border: none;
            border-top: 1px solid #3D5166;
            font-size: 13px;
            text-align: left;
            padding-left: 20px;
        }
        QPushButton#settingsButton:hover {
            background-color: #3D5166;
            color: #ECF0F1;
        }
    )");
}
```

---

## 5. 修改 MainWindow 集成左右分栏

### 5.1 修改 `mainwindow.h`

在 `private` 区域新增成员变量：

```cpp
// 新增头文件前向声明（在文件顶部）
class Sidebar;

// 新增成员变量（在 private 区域）
Sidebar*  sidebar_       = nullptr;
QWidget*  content_area_  = nullptr;
```

### 5.2 修改 `mainwindow.cpp` 的 `initUI()`

将原来的 `QLabel* placeholder` 替换为左右分栏结构：

**删除这段旧代码：**
```cpp
// 内容区占位
QLabel* placeholder = new QLabel("内容区域", contentWidget);
placeholder->setObjectName("placeholder");
placeholder->setAlignment(Qt::AlignCenter);
contentLayout->addWidget(placeholder, 1);
```

**替换为：**
```cpp
// ---- 左右分栏容器 ----
QWidget* bodyWidget = new QWidget(contentWidget);
bodyWidget->setObjectName("bodyWidget");
QHBoxLayout* bodyLayout = new QHBoxLayout(bodyWidget);
bodyLayout->setContentsMargins(0, 0, 0, 0);
bodyLayout->setSpacing(0);

// 左侧侧边栏
sidebar_ = new Sidebar(bodyWidget);
bodyLayout->addWidget(sidebar_);

// 右侧内容区占位
content_area_ = new QWidget(bodyWidget);
content_area_->setObjectName("contentArea");
QLabel* contentPlaceholder = new QLabel("内容区域", content_area_);
contentPlaceholder->setAlignment(Qt::AlignCenter);
contentPlaceholder->setStyleSheet("color: #aaa; font-size: 16px;");
QVBoxLayout* contentAreaLayout = new QVBoxLayout(content_area_);
contentAreaLayout->addWidget(contentPlaceholder);
bodyLayout->addWidget(content_area_, 1);   // stretch = 1，占满剩余宽度

contentLayout->addWidget(bodyWidget, 1);
```

### 5.3 修改 `mainwindow.cpp` 的样式表

在 `setStyleSheet()` 中**删除**旧的 `placeholder` 样式，**新增** `bodyWidget` 和 `contentArea` 样式：

```cpp
setStyleSheet(R"(
    QWidget#contentWidget {
        background-color: #F5F6FA;
        border-radius: 8px;
    }
    QWidget#bodyWidget {
        background: transparent;
    }
    QWidget#contentArea {
        background-color: #F5F6FA;
        border-bottom-right-radius: 8px;
    }
)");
```

### 5.4 在 `mainwindow.h` 中添加 include

```cpp
#include "sidebar.h"
```

---

## 6. CMakeLists.txt 更新

在 `SOURCES` 和 `HEADERS` 中添加新文件：

```cmake
set(SOURCES
    src/main.cpp
    src/ui/mainwindow.cpp
    src/ui/tilebar.cpp
    src/ui/sidebar.cpp      # 新增
)

set(HEADERS
    src/ui/mainwindow.h
    src/ui/tilebar.h
    src/ui/sidebar.h        # 新增
)
```

---

## 7. 实现步骤（推荐顺序）

1. **创建文件**：新建 `src/ui/sidebar.h` 和 `src/ui/sidebar.cpp`，填入第 4 节代码
2. **更新 CMake**：按第 6 节修改 `CMakeLists.txt`
3. **修改 mainwindow.h**：添加前向声明和成员变量
4. **修改 mainwindow.cpp**：按第 5 节替换 `initUI()` 中的占位代码，更新样式表
5. **编译验证**：确认左右分栏正常显示，侧边栏宽度固定 200px

---

## 8. 关键细节说明

### 8.1 为什么需要 bodyWidget 中间层？

`contentLayout` 是 `QVBoxLayout`（垂直），`TileBar` 和内容区上下排列。
内容区内部需要 `QHBoxLayout`（水平）实现左右分栏，因此需要一个 `bodyWidget` 作为水平布局的容器。

```
contentLayout (QVBoxLayout)
├── tile_bar_   ← 垂直方向第一行
└── bodyWidget  ← 垂直方向第二行（stretch=1）
    └── bodyLayout (QHBoxLayout)
        ├── sidebar_      ← 水平方向左列（固定200px）
        └── content_area_ ← 水平方向右列（stretch=1）
```

### 8.2 圆角处理

无边框窗口整体圆角由 `MainWindow::paintEvent` 绘制，子控件只需处理**左下角**和**右下角**：
- `Sidebar` 设置 `border-bottom-left-radius: 8px`
- `contentArea` 设置 `border-bottom-right-radius: 8px`

### 8.3 分隔线

如需在侧边栏和内容区之间添加分隔线，可在 `bodyLayout` 中插入一个 1px 宽的 `QFrame`：

```cpp
QFrame* divider = new QFrame(bodyWidget);
divider->setFrameShape(QFrame::VLine);
divider->setFixedWidth(1);
divider->setStyleSheet("background-color: #D0D3DA;");
bodyLayout->addWidget(divider);
```

### 8.4 最大化时的圆角

`MainWindow::changeEvent` 已处理最大化时去掉阴影边距，子控件的 `border-radius` 在最大化时会显得突兀。可在 `changeEvent` 中动态切换样式，或直接忽略（一期不做要求）。

---

## 9. 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 侧边栏宽度不固定，随窗口拉伸 | 未调用 `setFixedWidth(200)` | 在 `Sidebar` 构造函数中添加 |
| 侧边栏背景色显示为白色 | `objectName` 与样式表不匹配 | 确认 `setObjectName("Sidebar")` 已调用 |
| 右侧内容区没有填满剩余空间 | `addWidget` 未传 `stretch` 参数 | 改为 `bodyLayout->addWidget(content_area_, 1)` |
| 圆角被子控件背景色覆盖 | 子控件背景不透明遮住了父控件圆角 | 给子控件设置对应角的 `border-radius` |
| CMake 重新配置后找不到新文件 | 未更新 `CMakeLists.txt` | 按第 6 节添加新文件到 `SOURCES` 和 `HEADERS` |
