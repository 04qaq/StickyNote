# StickyNoteManager 第一阶段完整开发文档

---

## 一、项目概述

### 1.1 项目背景

`StickyNoteManager` 是一款基于 **Qt 6 Widgets + CMake + C++17** 开发的桌面端便签管理工具。第一阶段（一期）的目标是搭建整个应用的**窗口骨架**，实现一个美观、可交互的无边框主窗口框架，为后续功能开发奠定基础。

### 1.2 一期交付目标

| 功能点 | 状态 |
|--------|------|
| CMake 工程配置 | ✅ 已完成 |
| 无边框窗口（圆角 + 阴影） | ✅ 已完成 |
| 自定义标题栏（最小化/最大化/关闭） | ✅ 已完成 |
| 标题栏双击最大化/还原 | ✅ 已完成 |
| 窗口拖拽移动 | ✅ 已完成 |
| 窗口四边/四角拖拽缩放 | ✅ 已完成 |
| 左右分栏布局（侧边栏 + 内容区） | ✅ 已完成 |
| 窗口位置与尺寸持久化 | ✅ 已完成 |

### 1.3 技术栈

| 项目 | 版本/说明 |
|------|-----------|
| 操作系统 | Windows（主要）/ macOS / Linux |
| 编译器 | MSVC 2022 |
| 构建系统 | CMake 3.25+ |
| Qt 版本 | Qt 6.10.2 (msvc2022_64) |
| C++ 标准 | C++17 |
| IDE | Visual Studio 2022 |

---

## 二、项目结构

```
StickyNoteManager/
├── CMakeLists.txt                  # 构建配置
├── DEVELOPMENT_GUIDE.md            # 开发指南
├── jianzuwang_mini_proj.md         # 产品需求文档
├── learn/                          # 文档目录
│   ├── phase1_development.md       # 第一阶段完整开发文档（本文件）
│   └── phase1_knowledge.md         # 代码知识详解与设计思路
├── resources/
│   └── StickyNoteManager.qrc       # 资源文件
└── src/
    ├── main.cpp                    # 程序入口
    ├── ui/
    │   ├── mainwindow.h/.cpp       # 主窗口
    │   ├── tilebar.h/.cpp          # 自定义标题栏
    │   └── sidebar.h/.cpp          # 左侧侧边栏
    └── utils/
        ├── windowhelper.h/.cpp     # 窗口缩放辅助类
```

---

## 三、类设计与职责

### 3.1 类关系图

```
MainWindow
├── TileBar          （自定义标题栏，通过信号与 MainWindow 通信）
├── SideBar          （左侧侧边栏）
├── content_widget_  （右侧内容区占位）
└── WindowHelper     （窗口缩放辅助，封装边缘检测与缩放计算）
```

### 3.2 各类职责说明

| 类 | 文件 | 职责 |
|----|------|------|
| `MainWindow` | `src/ui/mainwindow.h/.cpp` | 主窗口容器，负责整体布局、无边框绘制、事件协调 |
| `TileBar` | `src/ui/tilebar.h/.cpp` | 自定义标题栏，显示标题和窗口控制按钮，通过信号与主窗口通信 |
| `SideBar` | `src/ui/sidebar.h/.cpp` | 左侧导航侧边栏（一期占位），固定宽度 200px |
| `WindowHelper` | `src/utils/windowhelper.h/.cpp` | 无边框窗口缩放辅助类，封装边缘检测、光标更新、缩放计算逻辑 |
| `ResizeEdge` | `src/utils/windowhelper.h` | 缩放方向枚举，表示鼠标所在的边缘位置 |

### 3.3 ResizeEdge 枚举

```cpp
enum class ResizeEdge {
    None,
    Left, Right, Top, Bottom,
    TopLeft, TopRight, BottomLeft, BottomRight
};
```

---

## 四、核心常量说明

```cpp
// MainWindow 中定义的核心常量
static constexpr int SHADOW_MARGIN    = 10;  // 阴影区域宽度（像素）
static constexpr int CORNER_RADIUS    = 8;   // 圆角半径（像素）
static constexpr int TITLE_BAR_HEIGHT = 36;  // 标题栏高度（像素）
static constexpr int RESIZE_MARGIN    = 6;   // 缩放检测区域宽度（像素）
```

### 为什么需要 SHADOW_MARGIN？

```
┌──────────────────────────────────────┐  ← QWidget 实际大小（含阴影）
│  SHADOW_MARGIN = 10px（透明阴影区域）  │
│  ┌────────────────────────────────┐  │  ← 视觉上的内容区域
│  │                                │  │
│  │         实际内容区域            │  │
│  │                                │  │
│  └────────────────────────────────┘  │
│  SHADOW_MARGIN = 10px（透明阴影区域）  │
└──────────────────────────────────────┘
```

`WA_TranslucentBackground` 使窗口背景透明，阴影通过 `paintEvent` 绘制在透明区域上。因此窗口的 `geometry()` 比视觉上的内容区域大一圈，这一圈就是 `SHADOW_MARGIN`。

---

## 五、布局层次结构

```
MainWindow (QWidget)
└── outerLayout (QVBoxLayout)
    │   contentsMargins = SHADOW_MARGIN（为阴影留出空间）
    └── contentWidget (QWidget, objectName="contentWidget")
        └── contentLayout (QVBoxLayout)
            │   contentsMargins = 0, spacing = 0
            ├── TileBar (固定高度 36px)
            │   └── QHBoxLayout
            │       ├── QLabel "Sticky Notes"
            │       ├── addStretch()
            │       ├── QPushButton "─" (最小化)
            │       ├── QPushButton "□" (最大化)
            │       └── QPushButton "✕" (关闭)
            └── bodyWidget (QWidget, objectName="bodyWidget", stretch=1)
                └── bodyLayout (QHBoxLayout)
                    │   contentsMargins = 0, spacing = 0
                    ├── SideBar (固定宽度 200px)
                    │   └── QVBoxLayout
                    │       ├── QLabel "Sticky Notes" (固定高度 56px)
                    │       ├── categoryWidget (stretch=1, 占位)
                    │       └── QPushButton "⚙ 设置" (固定高度 40px)
                    └── content_widget_ (QWidget, objectName="contentArea", stretch=1)
                        └── QVBoxLayout
                            └── QLabel "内容区域" (占位)
```

---

## 六、功能实现详解

### 6.1 无边框窗口实现

**核心设置：**

```cpp
// 去掉系统边框，保留任务栏图标
setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);
// 背景透明，用于实现圆角和阴影
setAttribute(Qt::WA_TranslucentBackground);
```

**为什么需要 `Qt::WindowMinimizeButtonHint`？**

单独使用 `Qt::FramelessWindowHint` 时，窗口不会出现在任务栏中。加上 `Qt::WindowMinimizeButtonHint` 可以保留任务栏图标，让用户能通过任务栏切换窗口。

### 6.2 圆角背景与阴影绘制

在 `paintEvent` 中手动绘制，分两步：

**第一步：绘制多层模拟阴影**

```cpp
// 循环绘制从外到内逐渐不透明的圆角矩形轮廓
for (int i = SHADOW_MARGIN; i > 0; --i) {
    // alpha 越靠外越小（越透明）
    int alpha = static_cast<int>(50.0 * (1.0 - static_cast<double>(i) / SHADOW_MARGIN));
    QPen shadowPen(QColor(0, 0, 0, alpha));
    shadowPen.setWidth(1);
    painter.setPen(shadowPen);
    painter.setBrush(Qt::NoBrush);
    QRect r = shadowRect.adjusted(-i, -i, i, i);
    QPainterPath path;
    path.addRoundedRect(r, CORNER_RADIUS + i, CORNER_RADIUS + i);
    painter.drawPath(path);
}
```

**第二步：绘制圆角背景**

```cpp
QPainterPath bgPath;
bgPath.addRoundedRect(shadowRect, CORNER_RADIUS, CORNER_RADIUS);
painter.setPen(Qt::NoPen);
painter.setBrush(QColor("#F5F6FA"));
painter.drawPath(bgPath);
```

**最大化时的特殊处理：**

```cpp
if (isMaximized()) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#F5F6FA"));
    return;  // 最大化时不需要阴影和圆角
}
```

### 6.3 窗口拖拽移动

**实现原理：** 记录鼠标按下时的偏移量，移动时用全局坐标减去偏移量得到窗口新位置。

```
窗口新位置 = 鼠标当前全局坐标 - 鼠标按下时的偏移量
偏移量 = 鼠标按下时的全局坐标 - 窗口左上角坐标
```

**关键代码：**

```cpp
// 按下时记录偏移量
m_dragStartPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
m_isDragging = true;

// 移动时计算新位置
move(globalPos - m_dragStartPos);
```

**最大化状态下拖拽还原：**

```cpp
if (isMaximized()) {
    showNormal();
    // 还原后重置偏移量，让鼠标位于标题栏中间
    m_dragStartPos = QPoint(width() / 2, TITLE_BAR_HEIGHT / 2);
}
```

### 6.4 窗口边缘缩放

这是一期最复杂的功能，封装在 `WindowHelper` 类中。

#### 6.4.1 边缘检测（hitTest）

```
内容矩形 = rect().adjusted(SHADOW_MARGIN, SHADOW_MARGIN, -SHADOW_MARGIN, -SHADOW_MARGIN)

左边缘区域：x ∈ [contentLeft - RESIZE_MARGIN, contentLeft + RESIZE_MARGIN]
右边缘区域：x ∈ [contentRight - RESIZE_MARGIN, contentRight + RESIZE_MARGIN]
上边缘区域：y ∈ [contentTop - RESIZE_MARGIN, contentTop + RESIZE_MARGIN]
下边缘区域：y ∈ [contentBottom - RESIZE_MARGIN, contentBottom + RESIZE_MARGIN]

角优先于边：先判断四个角，再判断四条边
```

#### 6.4.2 缩放计算（doResize）

**核心思路：** 基于起始状态 + 总偏移量计算，避免累积误差。

```cpp
// ✅ 正确：基于起始状态 + 总偏移
QPoint delta = globalPos - resizeStartGlobalPos_;
QRect  geo   = resizeStartGeometry_;

// ❌ 错误：基于上一帧 + 帧间偏移（会累积误差）
```

#### 6.4.3 最小尺寸约束

```cpp
// 宽度约束：哪条边在动就约束哪条边，固定不动的那条边保持不变
if (newRight - newLeft < minSz.width()) {
    if (拖拽左边) newLeft = newRight - minSz.width();
    else          newRight = newLeft + minSz.width();
}
```

#### 6.4.4 光标更新问题与解决方案

**问题根源：** Qt 鼠标事件不会自动向上冒泡到父控件。窗口的绝大部分区域被子控件覆盖，鼠标在子控件上移动时，`MainWindow::mouseMoveEvent` 根本不会被调用。

**解决方案：事件过滤器（eventFilter）**

```cpp
// 1. 在 initUI() 末尾，为所有子孙控件安装事件过滤器
for (QWidget* w : findChildren<QWidget*>()) {
    w->installEventFilter(this);
    w->setMouseTracking(true);
}

// 2. 在 eventFilter 中拦截 MouseMove 事件
bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseMove && !isMaximized()) {
        QWidget* w = qobject_cast<QWidget*>(watched);
        if (w) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            // 将子控件坐标转换为 MainWindow 坐标
            QPoint posInWindow = w->mapTo(this, me->position().toPoint());
            ResizeEdge edge = window_helper_->hitTest(posInWindow);
            // 将光标设置到鼠标实际所在的子控件上
            window_helper_->updateCursor(edge, w);
        }
    }
    return QWidget::eventFilter(watched, event);
}
```

**为什么要把光标设置到子控件 `w` 上？**

Qt 光标优先级规则：子控件的光标会覆盖父控件的光标。如果只调用 `window_->setCursor()`，鼠标悬停在子控件上时，子控件自身的默认 `ArrowCursor` 会覆盖父窗口设置的缩放光标。解决方案是直接在鼠标所在的子控件 `w` 上调用 `setCursor()`，优先级最高，不会被覆盖。

### 6.5 鼠标事件处理流程

```
鼠标事件触发
    │
    ├─ MouseMove（子控件）→ eventFilter 拦截
    │       → 子控件坐标 mapTo MainWindow
    │       → hitTest 检测边缘
    │       → updateCursor 设置到子控件 w 上
    │
    ├─ MousePress → mousePressEvent
    │       → hitTest 检测边缘
    │       → edge != None → startResize（记录起始状态）
    │       → 在标题栏区域 → 记录 dragStartPos，isDragging=true
    │
    ├─ MouseMove（按下状态）→ mouseMoveEvent
    │       → isResizing → doResize（计算并应用新几何）
    │       → isDragging → move（移动窗口）
    │
    └─ MouseRelease → mouseReleaseEvent
            → isResizing → stopResize（清除状态）
            → isDragging → isDragging=false
```

### 6.6 最大化状态联动

当窗口状态变化时（最大化/还原），需要三处联动：

```cpp
void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange) {
        // 1. 通知标题栏切换按钮图标（□ ↔ ❐）
        tile_bar_->updateMaximizeButton(isMaximized());

        // 2. 最大化时去掉阴影边距，还原时恢复
        QVBoxLayout* outerLayout = qobject_cast<QVBoxLayout*>(layout());
        if (outerLayout) {
            int margin = isMaximized() ? 0 : SHADOW_MARGIN;
            outerLayout->setContentsMargins(margin, margin, margin, margin);
        }

        // 3. 触发重绘（更新阴影/圆角显示）
        update();
    }
    QWidget::changeEvent(event);
}
```

### 6.7 窗口状态持久化

使用 `QSettings` 保存和恢复窗口几何信息：

```cpp
// 保存（在 closeEvent 中调用）
void MainWindow::saveWindowState()
{
    QSettings settings("StickyNoteManager", "StickyNoteManager");
    settings.setValue("mainwindow/geometry", saveGeometry());
    settings.setValue("mainwindow/maximized", isMaximized());
}

// 恢复（在构造函数末尾调用）
void MainWindow::restoreWindowState()
{
    QSettings settings("StickyNoteManager", "StickyNoteManager");
    if (settings.contains("mainwindow/geometry")) {
        restoreGeometry(settings.value("mainwindow/geometry").toByteArray());
    } else {
        // 首次启动：居中显示
        QScreen* screen = QApplication::primaryScreen();
        if (screen) {
            QRect screenRect = screen->availableGeometry();
            move(screenRect.center() - rect().center());
        }
    }
    if (settings.value("mainwindow/maximized", false).toBool()) {
        showMaximized();
    }
}
```

---

## 七、样式设计

### 7.1 颜色规范

| 用途 | 色值 |
|------|------|
| 主色调（标题栏背景） | `#1890FF` |
| 窗口背景色 | `#F5F6FA` |
| 侧边栏背景 | `#2C3E50` |
| 侧边栏顶部（应用名称区） | `#243342` |
| 侧边栏文字 | `#ECF0F1` |
| 侧边栏次要文字 | `#BDC3C7` |
| 侧边栏分隔线 | `#3D5166` |
| 关闭按钮悬停 | `#E81123` |

### 7.2 尺寸规范

| 项目 | 值 |
|------|-----|
| 窗口默认尺寸 | 900+20 × 600+20（含阴影） |
| 窗口最小尺寸 | 400+20 × 300+20（含阴影） |
| 阴影边距 | 10px |
| 圆角半径 | 8px |
| 标题栏高度 | 36px |
| 侧边栏宽度 | 200px |
| 窗口控制按钮尺寸 | 46×36px |
| 缩放检测区域宽度 | 6px |

### 7.3 圆角处理策略

无边框窗口整体圆角由 `MainWindow::paintEvent` 绘制，子控件只需处理各自负责的角：

| 控件 | 圆角处理 |
|------|---------|
| `TileBar` | `border-top-left-radius: 8px; border-top-right-radius: 8px` |
| `SideBar` | `border-bottom-left-radius: 8px` |
| `contentArea` | `border-bottom-right-radius: 8px` |

---

## 八、CMake 构建配置

```cmake
cmake_minimum_required(VERSION 3.25)
project(StickyNoteManager VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)   # 自动处理 MOC（信号槽元对象系统）
set(CMAKE_AUTORCC ON)   # 自动处理资源文件
set(CMAKE_AUTOUIC ON)   # 自动处理 .ui 文件

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

# Windows 下隐藏控制台窗口
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)

# MSVC 中文源码支持
target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus /permissive- /utf-8)
```

---

## 九、代码规范

| 项目 | 规范 | 示例 |
|------|------|------|
| 类名 | 大驼峰 | `MainWindow`, `WindowHelper` |
| 成员变量 | 下划线后缀 | `tile_bar_`, `window_helper_` |
| 信号/槽 | 小驼峰 | `onMinimizeRequested()`, `closeRequested()` |
| 文件名 | 全小写 | `mainwindow.h`, `windowhelper.cpp` |
| 头文件保护 | `#pragma once` | — |
| 缩进 | 4 空格 | — |
| 字符串字面量 | 原始字符串 `R"(...)"` | 用于多行 QSS |
| 注释 | 中文注释 | 关键逻辑必须注释 |

---

## 十、已知问题与注意事项

| 问题 | 说明 | 状态 |
|------|------|------|
| `QWidget` 子类背景色不生效 | 需要 `setAttribute(Qt::WA_StyledBackground, true)` | ✅ 已处理 |
| 父控件样式污染子控件 | `setStyleSheet` 设置在父控件上会影响所有子孙 | ✅ 已处理 |
| 最大化时四周有空白 | 需在 `changeEvent` 中动态调整 margin | ✅ 已处理 |
| 子控件光标覆盖父窗口光标 | 需将光标设置到鼠标所在的子控件上 | ✅ 已处理 |
| 孙子控件未安装事件过滤器 | 用 `findChildren` 递归安装 | ✅ 已处理 |
| 缩放时窗口抖动 | 基于起始状态+总偏移计算，避免累积误差 | ✅ 已处理 |

> ⚠️ **重要提示**：第一阶段中为所有子控件安装了事件过滤器，拦截了 `MouseMove` 事件。如果二期开发中子控件的鼠标移动事件出现异常，请首先检查 `MainWindow::eventFilter` 中是否误拦截了不该拦截的事件。
