# 标签页实现指导

## 一、目标效果

在现有无边框窗口的标题栏下方，实现一个**自定义标签页栏**，支持：

- 新建标签页 / 关闭标签页
- 点击标签页切换内容区
- 标签页可水平滚动（标签过多时）
- 当前激活标签高亮显示

---

## 二、整体架构

```
MainWindow
├── outerLayout (QVBoxLayout, 留阴影边距)
└── contentWidget
    └── contentLayout (QVBoxLayout)
        ├── titleBar          ← 已有：拖拽区 + 窗口控制按钮
        ├── TabBar            ← 新增：标签页栏
        └── QStackedWidget    ← 新增：内容区（替换原 placeholder）
```

涉及新增的类：

| 类名 | 文件 | 职责 |
|------|------|------|
| `TabBar` | `src/ui/tabbar.h/.cpp` | 标签页栏，管理标签的增删切换 |
| `TabItem` | `tabbar.h` 内部类 | 单个标签的数据结构 |

---

## 三、TabBar 类设计

### 3.1 头文件 `src/ui/tabbar.h`

```cpp
#pragma once

#include <QWidget>
#include <QList>

class QHBoxLayout;
class QPushButton;
class QScrollArea;

// ============================================================
// TabBar —— 自定义标签页栏
//   · 管理多个标签，支持新建/关闭/切换
//   · 超出宽度时可水平滚动
// ============================================================
class TabBar : public QWidget
{
    Q_OBJECT

public:
    explicit TabBar(QWidget* parent = nullptr);

    // 新增一个标签，返回标签索引
    int  addTab(const QString& title);

    // 关闭指定索引的标签
    void removeTab(int index);

    // 获取当前激活的标签索引
    int  currentIndex() const;

    // 设置当前激活的标签索引
    void setCurrentIndex(int index);

    // 获取标签数量
    int  count() const;

signals:
    // 用户切换标签时发出，携带新的索引
    void currentChanged(int index);

    // 用户点击关闭按钮时发出，携带要关闭的索引
    void tabCloseRequested(int index);

    // 用户点击"+"按钮时发出
    void newTabRequested();

private slots:
    void onTabClicked(int index);
    void onCloseClicked(int index);

private:
    void updateTabStyles();   // 刷新所有标签的激活/非激活样式

    // 单个标签的 UI 容器
    struct TabItem {
        QWidget*     widget  = nullptr;  // 标签整体容器
        QPushButton* label   = nullptr;  // 标签标题按钮
        QPushButton* closeBtn = nullptr; // 关闭按钮
    };

    QList<TabItem>  m_tabs;           // 所有标签
    int             m_currentIndex = -1;

    QScrollArea*    m_scrollArea  = nullptr;  // 滚动容器
    QWidget*        m_tabContainer = nullptr; // 标签实际排列的容器
    QHBoxLayout*    m_tabLayout   = nullptr;  // 标签排列布局
    QPushButton*    m_addBtn      = nullptr;  // "+" 新建按钮
};
```

### 3.2 关键函数说明

| 函数 | 作用 |
|------|------|
| `addTab(title)` | 创建一个 `TabItem`，向 `m_tabLayout` 插入标签 widget，连接信号，调用 `setCurrentIndex` 激活新标签 |
| `removeTab(index)` | 从 `m_tabs` 移除，从布局中删除 widget，修正 `m_currentIndex`，发出 `currentChanged` |
| `setCurrentIndex(index)` | 更新 `m_currentIndex`，调用 `updateTabStyles()`，发出 `currentChanged` |
| `updateTabStyles()` | 遍历所有标签，激活的设置高亮样式，其余设置普通样式 |
| `onTabClicked(index)` | 调用 `setCurrentIndex(index)` |
| `onCloseClicked(index)` | 发出 `tabCloseRequested(index)` 信号，由外部（MainWindow）决定是否真正关闭 |

---

## 四、在 MainWindow 中集成

### 4.1 需要修改的地方

**`mainwindow.h`** 新增成员：

```cpp
#include <QStackedWidget>
// 前向声明
class TabBar;

// 在 private 区域新增：
TabBar*         m_tabBar     = nullptr;
QStackedWidget* m_stackedWidget = nullptr;
```

**`mainwindow.cpp`** 的 `initUI()` 中，替换原来的 `placeholder`：

```cpp
// 1. 引入头文件
#include "tabbar.h"
#include <QStackedWidget>

// 2. 在 contentLayout 中，titleBar 之后添加：

// 标签页栏
m_tabBar = new TabBar(contentWidget);
contentLayout->addWidget(m_tabBar);

// 内容区（多页面容器）
m_stackedWidget = new QStackedWidget(contentWidget);
contentLayout->addWidget(m_stackedWidget, 1);  // stretch=1 占满剩余空间

// 3. 连接信号
connect(m_tabBar, &TabBar::currentChanged,
        m_stackedWidget, &QStackedWidget::setCurrentIndex);

connect(m_tabBar, &TabBar::tabCloseRequested,
        this, &MainWindow::onTabCloseRequested);

connect(m_tabBar, &TabBar::newTabRequested,
        this, &MainWindow::onNewTabRequested);

// 4. 默认打开一个标签
onNewTabRequested();
```

### 4.2 MainWindow 新增槽函数

```cpp
// 新建标签页
void MainWindow::onNewTabRequested()
{
    // 创建该标签对应的内容页
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);
    QLabel* label = new QLabel("新便签", page);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    // 向 stackedWidget 添加页面
    int pageIndex = m_stackedWidget->addWidget(page);

    // 向 tabBar 添加标签（两者索引保持一致）
    m_tabBar->addTab(QString("便签 %1").arg(pageIndex + 1));
}

// 关闭标签页
void MainWindow::onTabCloseRequested(int index)
{
    if (m_tabBar->count() <= 1) return;  // 至少保留一个标签

    // 先移除内容页
    QWidget* page = m_stackedWidget->widget(index);
    m_stackedWidget->removeWidget(page);
    delete page;

    // 再移除标签
    m_tabBar->removeTab(index);
}
```

> **关键约定**：`TabBar` 的标签索引与 `QStackedWidget` 的页面索引**必须始终保持一致**。
> 新增时先 `addWidget` 拿到索引，再 `addTab`；删除时先 `removeWidget`，再 `removeTab`。

---

## 五、TabBar 实现要点

### 5.1 布局结构

```
TabBar (QWidget)
└── outerLayout (QHBoxLayout, spacing=0, margin=0)
    ├── m_scrollArea (QScrollArea, 水平滚动)
    │   └── m_tabContainer (QWidget)
    │       └── m_tabLayout (QHBoxLayout)
    │           ├── TabItem[0].widget
    │           ├── TabItem[1].widget
    │           └── ...
    └── m_addBtn (QPushButton, 固定宽度 32px)
```

### 5.2 单个 TabItem 的布局

```
TabItem.widget (QWidget)
└── itemLayout (QHBoxLayout, 左右 padding 8px)
    ├── label (QPushButton, 显示标题，无边框)
    └── closeBtn (QPushButton, "×", 固定 16×16)
```

### 5.3 滚动区域设置

```cpp
m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
m_scrollArea->setWidgetResizable(true);
m_scrollArea->setFrameShape(QFrame::NoFrame);
// 固定高度，与标签栏高度一致
m_scrollArea->setFixedHeight(TAB_HEIGHT);
```

### 5.4 样式参考

```cpp
// 激活标签
"QWidget#tabItem_active { background: #ffffff; border-bottom: 2px solid #1890FF; }"

// 非激活标签
"QWidget#tabItem_normal { background: #E8EAF0; border-bottom: 2px solid transparent; }"

// 关闭按钮
"QPushButton#closeBtn { border: none; background: transparent; color: #888; }"
"QPushButton#closeBtn:hover { color: #ff4d4f; background: #f0f0f0; border-radius: 8px; }"

// 新建按钮
"QPushButton#addBtn { border: none; background: transparent; font-size: 18px; color: #888; }"
"QPushButton#addBtn:hover { background: #E0E0E0; border-radius: 4px; }"
```

---

## 六、实现步骤（推荐顺序）

1. **创建文件**：新建 `src/ui/tabbar.h` 和 `src/ui/tabbar.cpp`
2. **更新 CMakeLists.txt**：将两个文件加入 `SOURCES` / `HEADERS`
3. **实现 TabBar**：按照第三节完成构造函数、`addTab`、`removeTab`、`setCurrentIndex`、`updateTabStyles`
4. **集成到 MainWindow**：按照第四节修改 `initUI()`，添加槽函数
5. **调试样式**：运行后调整颜色、高度、间距等视觉细节

---

## 七、常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| 标签索引与页面索引不同步 | 删除时顺序错误 | 始终先操作 `QStackedWidget`，再操作 `TabBar` |
| 关闭最后一个标签后崩溃 | `m_stackedWidget` 无页面时 `currentIndex()` 返回 -1 | 在 `onTabCloseRequested` 中判断 `count() <= 1` 时不允许关闭 |
| 标签栏高度撑开整个窗口 | `TabBar` 没有设置固定高度 | 在 `TabBar` 构造函数中调用 `setFixedHeight(TAB_HEIGHT)` |
| 点击标签无响应 | lambda 捕获的 index 是引用而非值 | 使用值捕获：`[this, i]{ onTabClicked(i); }` |
