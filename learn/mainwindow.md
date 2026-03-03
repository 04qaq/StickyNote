# MainWindow 开发指导

## 概述

`MainWindow` 是整个应用的**主窗口容器**，继承自 `QWidget`。它不使用系统原生边框，而是通过 Qt 的绘图 API 自绘圆角背景与投影阴影，并将标题栏交互、侧边栏、边缘缩放等职责分别委托给独立的子类处理。

```
MainWindow (QWidget)
├── TitleBar          ← 标题栏（拖拽 / 最小化 / 最大化 / 关闭）
├── Sidebar           ← 左侧分类导航栏
├── QTextEdit         ← 内容区占位（二期替换为便签列表）
└── WindowHelper      ← 边缘缩放事件过滤器（工具类，非子 Widget）
```

---

## 文件位置

| 文件 | 路径 |
|------|------|
| 头文件 | `src/ui/mainwindow.h` |
| 实现文件 | `src/ui/mainwindow.cpp` |

---

## 关键常量

| 常量 | 值 | 说明 |
|------|----|------|
| `SHADOW_MARGIN` | `10` | 阴影区域宽度（像素），窗口实际内容区比 `geometry()` 小这么多一圈 |
| `CORNER_RADIUS` | `8` | 圆角半径（像素） |

> **为什么需要 `SHADOW_MARGIN`？**  
> Qt 的 `WA_TranslucentBackground` 使窗口背景透明，阴影通过 `paintEvent` 绘制在透明区域上。因此窗口的 `geometry()` 比视觉上的内容区域大一圈，这一圈就是 `SHADOW_MARGIN`。

---

## 构造与析构

### `MainWindow(QWidget* parent = nullptr)`

```cpp
explicit MainWindow(QWidget* parent = nullptr);
```

**执行顺序：**

1. 设置 `Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint`，去掉系统边框，保留任务栏图标。
2. 设置 `WA_TranslucentBackground`，使窗口背景透明，为自绘阴影做准备。
3. 设置最小尺寸（`700+20` × `450+20`）和默认尺寸（`900+20` × `600+20`），`+20` 是两侧各 `SHADOW_MARGIN`。
4. 调用 `initUI()` 构建控件树。
5. 调用 `initConnections()` 连接信号槽。
6. 调用 `applyStyle()` 应用样式表。
7. 创建 `WindowHelper` 并安装到 `this`，启用边缘缩放。
8. 调用 `restoreWindowState()` 从 `QSettings` 恢复上次的位置和尺寸。

### `~MainWindow()`

```cpp
~MainWindow() override;
```

析构函数为空实现。子控件和 `WindowHelper` 均以 `this` 为父对象，Qt 对象树会自动释放。

---

## 私有初始化函数

### `initUI()`

```cpp
void initUI();
```

**职责：** 构建完整的控件树和布局层次。

**布局层次：**

```
MainWindow (this)
└── QVBoxLayout (outerLayout)  ← 四边留 SHADOW_MARGIN 的边距
    └── QWidget (contentWidget)  ← 圆角背景的视觉容器
        └── QVBoxLayout (contentLayout)
            ├── TitleBar (m_titleBar)          ← 固定高度 36px
            └── QWidget (bodyWidget)           ← 主体区域，stretch=1
                └── QHBoxLayout (bodyLayout)
                    ├── Sidebar (m_sidebar)    ← 固定宽度 140px
                    └── QTextEdit (m_textEdit) ← stretch=1，占满剩余空间
```

**关键细节：**
- `outerLayout` 的 `contentsMargins` 设为 `SHADOW_MARGIN`，为阴影留出绘制空间。
- `contentWidget` 和 `bodyWidget` 的 `objectName` 用于 QSS 精确匹配样式。
- `m_textEdit` 设置 `QFrame::NoFrame` 去掉默认边框。

---

### `initConnections()`

```cpp
void initConnections();
```

**职责：** 连接 `TitleBar` 发出的信号到 `MainWindow` 的对应槽。

| 信号 | 槽 | 说明 |
|------|----|------|
| `TitleBar::minimizeRequested` | `MainWindow::showMinimized` | 最小化窗口 |
| `TitleBar::closeRequested` | `MainWindow::close` | 关闭窗口（触发 `closeEvent`） |
| `TitleBar::maximizeRequested` | Lambda | 切换最大化/还原状态 |

---

### `applyStyle()`

```cpp
void applyStyle();
```

**职责：** 通过 `setStyleSheet()` 为内部控件设置 QSS 样式。

| 选择器 | 说明 |
|--------|------|
| `QWidget#contentWidget` | 圆角背景色 `#F5F6FA` |
| `QWidget#bodyWidget` | 主体区域背景色，底部圆角 |
| `QTextEdit#textEdit` | 白色背景、无边框、内边距 12px |

> **注意：** `MainWindow` 本身背景透明，圆角和阴影完全由 `paintEvent` 绘制，不依赖 QSS。

---

## 窗口状态持久化

### `saveWindowState()`

```cpp
void saveWindowState();
```

**职责：** 将当前窗口的几何信息和最大化状态写入 `QSettings`。

```cpp
QSettings settings("StickyNoteManager", "StickyNoteManager");
settings.setValue("mainwindow/geometry", saveGeometry());   // 位置+尺寸的二进制序列化
settings.setValue("mainwindow/maximized", isMaximized());   // bool
```

- `saveGeometry()` 返回 `QByteArray`，包含位置、尺寸、屏幕 DPI 等信息，跨屏幕安全。
- 在 `closeEvent` 中调用，确保每次关闭都保存。

---

### `restoreWindowState()`

```cpp
void restoreWindowState();
```

**职责：** 从 `QSettings` 读取并恢复窗口状态。

**逻辑流程：**

```
有保存的 geometry？
  ├── 是 → restoreGeometry() 恢复位置和尺寸
  └── 否 → 首次启动，将窗口居中到主屏幕

有保存的 maximized=true？
  └── 是 → showMaximized()
```

- 在构造函数末尾调用，确保控件树已完全初始化。

---

## 事件处理函数

### `paintEvent(QPaintEvent* event)`

```cpp
void paintEvent(QPaintEvent* event) override;
```

**职责：** 绘制窗口的圆角背景和多层模拟阴影。

**绘制流程：**

```
最大化状态？
  ├── 是 → 直接 fillRect 填充背景色，return（无需阴影和圆角）
  └── 否 → 正常绘制

1. 计算 shadowRect = rect() 向内收缩 SHADOW_MARGIN
2. 循环 i 从 SHADOW_MARGIN 到 1：
   - alpha = 50 * (1 - i/SHADOW_MARGIN)  ← 越靠外越透明
   - 在 shadowRect 向外扩展 i 像素处绘制圆角矩形轮廓
3. 在 shadowRect 上绘制实心圆角矩形（背景色 #F5F6FA）
```

> **为什么用循环模拟阴影？**  
> Qt 没有内置的 `box-shadow`，通过绘制多层逐渐透明的圆角矩形轮廓来模拟高斯模糊阴影效果。

---

### `resizeEvent(QResizeEvent* event)`

```cpp
void resizeEvent(QResizeEvent* event) override;
```

**职责：** 窗口尺寸变化时触发重绘，确保阴影区域随窗口大小更新。

调用 `QWidget::resizeEvent(event)` 后调用 `update()` 请求重绘。

---

### `closeEvent(QCloseEvent* event)`

```cpp
void closeEvent(QCloseEvent* event) override;
```

**职责：** 窗口关闭前保存状态。

先调用 `saveWindowState()`，再调用 `QWidget::closeEvent(event)` 执行默认关闭逻辑。

---

### `changeEvent(QEvent* event)`

```cpp
void changeEvent(QEvent* event) override;
```

**职责：** 监听窗口状态变化（最大化 ↔ 还原），同步更新 UI。

**触发条件：** `event->type() == QEvent::WindowStateChange`

**处理逻辑：**

| 操作 | 说明 |
|------|------|
| `m_titleBar->updateMaxButton(maximized)` | 通知标题栏切换最大化按钮图标（`□` ↔ `❐`） |
| `outerLayout->setContentsMargins(...)` | 最大化时边距设为 0（无阴影），还原时恢复 `SHADOW_MARGIN` |
| `update()` | 触发重绘，更新阴影/圆角显示 |

---

## 数据成员

| 成员 | 类型 | 说明 |
|------|------|------|
| `m_titleBar` | `TitleBar*` | 自定义标题栏 |
| `m_sidebar` | `Sidebar*` | 左侧分类导航栏 |
| `m_textEdit` | `QTextEdit*` | 内容区占位控件（二期替换） |
| `m_winHelper` | `WindowHelper*` | 边缘缩放辅助，以事件过滤器方式工作 |

---

## 设计要点总结

1. **职责分离**：标题栏交互 → `TitleBar`，边缘缩放 → `WindowHelper`，导航 → `Sidebar`，主窗口只负责组装和协调。

2. **透明背景 + 自绘阴影**：`WA_TranslucentBackground` + `paintEvent` 实现圆角和阴影，不依赖系统窗口装饰。

3. **阴影边距补偿**：`SHADOW_MARGIN` 贯穿布局、绘制、最大化三个场景，修改时需同步考虑三处。

4. **状态持久化**：使用 `QSettings` 的 `saveGeometry/restoreGeometry` 接口，天然支持多屏幕和 DPI 缩放。

5. **最大化适配**：最大化时去掉阴影边距、跳过圆角绘制、更新标题栏按钮图标，三处联动保证视觉一致性。
