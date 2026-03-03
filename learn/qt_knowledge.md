# Qt 知识整理

> 基于 StickyNoteManager 项目代码整理，Qt 6.x / C++17

---

## 目录

1. [QApplication](#1-qapplication)
2. [QWidget](#2-qwidget)
3. [布局管理器](#3-布局管理器)
4. [QLabel](#4-qlabel)
5. [QPushButton](#5-qpushbutton)
6. [信号与槽](#6-信号与槽)
7. [事件系统](#7-事件系统)
8. [QPainter 绘图](#8-qpainter-绘图)
9. [QSettings 持久化](#9-qsettings-持久化)
10. [QScreen 屏幕信息](#10-qscreen-屏幕信息)
11. [样式表 QSS](#11-样式表-qss)
12. [无边框窗口](#12-无边框窗口)
13. [CMake 与 Qt 集成](#13-cmake-与-qt-集成)
14. [常见问题汇总](#14-常见问题汇总)

---

## 1. QApplication

### 头文件
```cpp
#include <QApplication>
```

### 作用
管理 Qt 应用程序的生命周期、事件循环、全局设置。每个 Qt Widgets 程序必须有且只有一个 `QApplication` 实例。

### 常用函数

| 函数 | 原型 | 说明 |
|------|------|------|
| 构造函数 | `QApplication(int &argc, char **argv)` | 必须在所有 Qt 对象之前创建 |
| `exec()` | `int exec()` | 进入事件循环，阻塞直到 `quit()` 被调用 |
| `primaryScreen()` | `static QScreen* primaryScreen()` | 获取主屏幕对象 |

### 典型用法
```cpp
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();  // 进入事件循环
}
```

---

## 2. QWidget

### 头文件
```cpp
#include <QWidget>
```

### 作用
所有 UI 控件的基类，可以作为独立窗口或嵌入其他控件中。

### 常用函数

| 函数 | 原型 | 说明 |
|------|------|------|
| `setWindowFlags()` | `void setWindowFlags(Qt::WindowFlags type)` | 设置窗口标志（无边框、最小化按钮等） |
| `setAttribute()` | `void setAttribute(Qt::WidgetAttribute, bool on = true)` | 设置控件属性 |
| `setObjectName()` | `void setObjectName(const QString &name)` | 设置对象名，用于 QSS 选择器 |
| `setFixedSize()` | `void setFixedSize(int w, int h)` | 固定控件尺寸（宽高不可调整） |
| `setFixedWidth()` | `void setFixedWidth(int w)` | 固定宽度 |
| `setFixedHeight()` | `void setFixedHeight(int h)` | 固定高度 |
| `setMinimumSize()` | `void setMinimumSize(int minw, int minh)` | 设置最小尺寸 |
| `resize()` | `void resize(int w, int h)` | 调整控件大小 |
| `show()` | `void show()` | 显示控件 |
| `showMinimized()` | `void showMinimized()` | 最小化 |
| `showMaximized()` | `void showMaximized()` | 最大化 |
| `showNormal()` | `void showNormal()` | 还原正常大小 |
| `isMaximized()` | `bool isMaximized() const` | 是否处于最大化状态 |
| `close()` | `bool close()` | 关闭控件（触发 closeEvent） |
| `update()` | `void update()` | 请求重绘（异步，合并多次调用） |
| `rect()` | `QRect rect() const` | 返回控件的矩形区域（相对自身） |
| `frameGeometry()` | `QRect frameGeometry() const` | 返回包含边框的窗口矩形（相对屏幕） |
| `move()` | `void move(const QPoint &)` | 移动窗口到指定位置 |
| `layout()` | `QLayout* layout() const` | 获取当前布局 |
| `setStyleSheet()` | `void setStyleSheet(const QString &styleSheet)` | 设置 QSS 样式表 |
| `saveGeometry()` | `QByteArray saveGeometry() const` | 序列化窗口几何信息 |
| `restoreGeometry()` | `bool restoreGeometry(const QByteArray &geometry)` | 恢复窗口几何信息 |

### 重要属性标志

```cpp
// 窗口标志
Qt::FramelessWindowHint        // 去掉系统边框
Qt::WindowMinimizeButtonHint   // 保留任务栏图标（配合无边框使用）

// 控件属性
Qt::WA_TranslucentBackground   // 背景透明（实现圆角/阴影必须）
Qt::WA_StyledBackground        // 让 QWidget 子类响应 QSS background-color
```

### ⚠️ 常见问题

**问题：自定义 QWidget 子类设置了 `background-color` 样式表但不生效**

原因：普通 `QWidget` 默认不绘制背景，样式表的 `background-color` 被忽略。

解决：
```cpp
setAttribute(Qt::WA_StyledBackground, true);  // 必须加这一行
```

---

## 3. 布局管理器

### 头文件
```cpp
#include <QVBoxLayout>   // 垂直布局
#include <QHBoxLayout>   // 水平布局
```

### QVBoxLayout / QHBoxLayout 常用函数

| 函数 | 原型 | 说明 |
|------|------|------|
| 构造函数 | `QVBoxLayout(QWidget *parent)` | 传入父控件，自动设置为该控件的布局 |
| `addWidget()` | `void addWidget(QWidget*, int stretch = 0, ...)` | 添加控件，stretch 为拉伸因子 |
| `addStretch()` | `void addStretch(int stretch = 0)` | 添加弹性空间（推开两侧控件） |
| `setContentsMargins()` | `void setContentsMargins(int l, int t, int r, int b)` | 设置内边距 |
| `setSpacing()` | `void setSpacing(int spacing)` | 设置控件间距 |
| `setContentsMargins(0,0,0,0)` | — | 消除默认内边距（常用） |

### 典型用法
```cpp
// 垂直布局：标题栏 + 内容区
QVBoxLayout* layout = new QVBoxLayout(this);
layout->setContentsMargins(0, 0, 0, 0);
layout->setSpacing(0);
layout->addWidget(titleBar);
layout->addWidget(contentArea, 1);  // stretch=1 占满剩余空间

// 水平布局：侧边栏 + 主内容
QHBoxLayout* hLayout = new QHBoxLayout(bodyWidget);
hLayout->addWidget(sidebar);
hLayout->addWidget(mainContent, 1); // stretch=1 占满剩余宽度
```

### ⚠️ 常见问题

**问题：布局中控件之间有意外间距**

原因：`QVBoxLayout`/`QHBoxLayout` 默认有 `spacing` 和 `contentsMargins`。

解决：
```cpp
layout->setContentsMargins(0, 0, 0, 0);
layout->setSpacing(0);
```

---

## 4. QLabel

### 头文件
```cpp
#include <QLabel>
```

### 常用函数

| 函数 | 原型 | 说明 |
|------|------|------|
| 构造函数 | `QLabel(const QString &text, QWidget *parent)` | 创建带文字的标签 |
| `setAlignment()` | `void setAlignment(Qt::Alignment)` | 设置文字对齐方式 |
| `setFixedHeight()` | `void setFixedHeight(int h)` | 固定高度 |
| `setObjectName()` | `void setObjectName(const QString &)` | 设置对象名（用于 QSS） |

### 对齐方式常量
```cpp
Qt::AlignCenter       // 水平+垂直居中
Qt::AlignLeft         // 左对齐
Qt::AlignRight        // 右对齐
Qt::AlignVCenter      // 垂直居中
```

---

## 5. QPushButton

### 头文件
```cpp
#include <QPushButton>
```

### 常用函数

| 函数 | 原型 | 说明 |
|------|------|------|
| 构造函数 | `QPushButton(const QString &text, QWidget *parent)` | 创建带文字的按钮 |
| `setFixedSize()` | `void setFixedSize(int w, int h)` | 固定按钮尺寸 |
| `setToolTip()` | `void setToolTip(const QString &)` | 设置悬停提示文字 |
| `setText()` | `void setText(const QString &)` | 动态修改按钮文字 |
| `setObjectName()` | `void setObjectName(const QString &)` | 设置对象名（用于 QSS） |

### 常用信号
```cpp
void clicked(bool checked = false);  // 点击时发出
```

---

## 6. 信号与槽

### 基本语法

```cpp
// 连接信号到槽（推荐：函数指针语法，编译期检查）
connect(sender, &SenderClass::signalName,
        receiver, &ReceiverClass::slotName);

// 连接信号到 Lambda
connect(button, &QPushButton::clicked, this, [this]() {
    // 处理逻辑
});

// 信号连接信号（转发）
connect(close_button_, &QPushButton::clicked,
        this, &TileBar::closeRequested);
```

### 自定义信号与槽

```cpp
// .h 文件
class TileBar : public QWidget {
    Q_OBJECT  // 必须加这个宏
signals:
    void closeRequested();      // 只声明，不实现
    void minimizeRequested();
public slots:
    void onMinimizeRequested(); // 槽函数需要实现
};
```

### emit 发出信号

```cpp
void TileBar::mouseDoubleClickEvent(QMouseEvent* event) {
    Q_UNUSED(event)
    emit maximizeRequested();  // 主动发出信号
}
```

### ⚠️ 常见问题

**问题：信号槽连接后没有响应**

检查项：
1. 类定义中是否有 `Q_OBJECT` 宏
2. 是否运行了 MOC（CMake 中 `CMAKE_AUTOMOC ON`）
3. `connect()` 的参数类型是否完全匹配
4. sender 对象是否已被销毁

---

## 7. 事件系统

### 常用事件重写

| 事件函数 | 头文件 | 触发时机 |
|----------|--------|----------|
| `paintEvent(QPaintEvent*)` | `<QPaintEvent>` | 控件需要重绘时 |
| `resizeEvent(QResizeEvent*)` | `<QResizeEvent>` | 控件大小改变时 |
| `closeEvent(QCloseEvent*)` | `<QCloseEvent>` | 窗口关闭时（可拦截） |
| `mousePressEvent(QMouseEvent*)` | `<QMouseEvent>` | 鼠标按下时 |
| `mouseMoveEvent(QMouseEvent*)` | `<QMouseEvent>` | 鼠标移动时 |
| `mouseReleaseEvent(QMouseEvent*)` | `<QMouseEvent>` | 鼠标释放时 |
| `mouseDoubleClickEvent(QMouseEvent*)` | `<QMouseEvent>` | 鼠标双击时 |
| `changeEvent(QEvent*)` | `<QEvent>` | 控件状态改变时（最大化/最小化等） |

### QMouseEvent 常用函数

```cpp
event->button()              // 触发的按键（Qt::LeftButton 等）
event->buttons()             // 当前按下的所有按键（位掩码）
event->position()            // 相对控件的位置（QPointF，Qt6）
event->globalPosition()      // 相对屏幕的位置（QPointF，Qt6）
event->globalPosition().toPoint()  // 转为 QPoint
event->accept()              // 标记事件已处理，不再传播
```

### changeEvent 判断窗口状态

```cpp
void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        // 窗口最大化/还原/最小化时触发
        bool maximized = isMaximized();
    }
    QWidget::changeEvent(event);  // 必须调用父类实现
}
```

### Q_UNUSED 宏

```cpp
// 消除"未使用参数"编译警告
Q_UNUSED(event)
```

### ⚠️ 常见问题

**问题：重写事件后父类行为丢失**

原因：忘记调用父类的事件函数。

解决：在重写函数末尾调用父类实现：
```cpp
void MainWindow::resizeEvent(QResizeEvent* event) {
    // 自定义逻辑...
    QWidget::resizeEvent(event);  // 必须调用
}
```

**问题：`event->pos()` 在 Qt6 中报错**

原因：Qt6 中 `pos()` 已废弃，改用 `position()`。

```cpp
// Qt5
event->pos()           // 返回 QPoint
// Qt6
event->position()      // 返回 QPointF
event->position().toPoint()  // 转为 QPoint
```

---

## 8. QPainter 绘图

### 头文件
```cpp
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
```

### 常用函数

| 函数 | 原型 | 说明 |
|------|------|------|
| 构造函数 | `QPainter(QPaintDevice *device)` | 在 paintEvent 中传入 `this` |
| `setRenderHint()` | `void setRenderHint(RenderHint hint, bool on = true)` | 设置渲染质量 |
| `setPen()` | `void setPen(const QPen &pen)` | 设置画笔（轮廓） |
| `setBrush()` | `void setBrush(const QBrush &brush)` | 设置画刷（填充） |
| `drawPath()` | `void drawPath(const QPainterPath &path)` | 绘制路径 |
| `fillRect()` | `void fillRect(const QRect &, const QColor &)` | 填充矩形 |

### QPainterPath 常用函数

```cpp
QPainterPath path;
path.addRoundedRect(rect, xRadius, yRadius);  // 添加圆角矩形路径
```

### QRect 常用函数

```cpp
rect()                          // 控件自身矩形
rect().adjusted(l, t, r, b)    // 调整矩形四边（正值向内缩，负值向外扩）
```

### 渲染提示

```cpp
painter.setRenderHint(QPainter::Antialiasing);  // 抗锯齿（圆角必须开启）
```

### 典型用法：绘制圆角背景 + 阴影

```cpp
void MainWindow::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect shadowRect = rect().adjusted(SHADOW_MARGIN, SHADOW_MARGIN,
                                       -SHADOW_MARGIN, -SHADOW_MARGIN);
    // 多层阴影
    for (int i = SHADOW_MARGIN; i > 0; --i) {
        int alpha = static_cast<int>(50.0 * (1.0 - (double)i / SHADOW_MARGIN));
        painter.setPen(QPen(QColor(0, 0, 0, alpha), 1));
        painter.setBrush(Qt::NoBrush);
        QPainterPath path;
        path.addRoundedRect(shadowRect.adjusted(-i,-i,i,i),
                            CORNER_RADIUS + i, CORNER_RADIUS + i);
        painter.drawPath(path);
    }
    // 圆角背景
    QPainterPath bgPath;
    bgPath.addRoundedRect(shadowRect, CORNER_RADIUS, CORNER_RADIUS);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#F5F6FA"));
    painter.drawPath(bgPath);
}
```

### ⚠️ 常见问题

**问题：圆角边缘有锯齿**

解决：开启抗锯齿
```cpp
painter.setRenderHint(QPainter::Antialiasing);
```

**问题：最大化时阴影区域仍然显示**

解决：最大化时单独处理，直接填充矩形：
```cpp
if (isMaximized()) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#F5F6FA"));
    return;
}
```

---

## 9. QSettings 持久化

### 头文件
```cpp
#include <QSettings>
```

### 作用
跨会话保存应用程序配置（Windows 下存储在注册表，macOS 存储在 plist，Linux 存储在 ini 文件）。

### 常用函数

| 函数 | 原型 | 说明 |
|------|------|------|
| 构造函数 | `QSettings(const QString &org, const QString &app)` | 指定组织名和应用名 |
| `setValue()` | `void setValue(const QString &key, const QVariant &value)` | 写入键值 |
| `value()` | `QVariant value(const QString &key, const QVariant &defaultValue)` | 读取键值 |
| `contains()` | `bool contains(const QString &key) const` | 判断键是否存在 |

### 典型用法：保存/恢复窗口状态

```cpp
void MainWindow::saveWindowState() {
    QSettings settings("StickyNoteManager", "StickyNoteManager");
    settings.setValue("mainwindow/geometry", saveGeometry());
    settings.setValue("mainwindow/maximized", isMaximized());
}

void MainWindow::restoreWindowState() {
    QSettings settings("StickyNoteManager", "StickyNoteManager");
    if (settings.contains("mainwindow/geometry")) {
        restoreGeometry(settings.value("mainwindow/geometry").toByteArray());
    }
    if (settings.value("mainwindow/maximized", false).toBool()) {
        showMaximized();
    }
}
```

---

## 10. QScreen 屏幕信息

### 头文件
```cpp
#include <QScreen>
```

### 常用函数

| 函数 | 原型 | 说明 |
|------|------|------|
| `availableGeometry()` | `QRect availableGeometry() const` | 可用屏幕区域（排除任务栏） |
| `geometry()` | `QRect geometry() const` | 完整屏幕区域 |

### 典型用法：窗口居中显示

```cpp
QScreen* screen = QApplication::primaryScreen();
if (screen) {
    QRect screenRect = screen->availableGeometry();
    move(screenRect.center() - rect().center());
}
```

---

## 11. 样式表 QSS

### 基本语法

```css
/* 按类型选择 */
QPushButton { color: red; }

/* 按对象名选择（需先 setObjectName）*/
QPushButton#closeButton { background: transparent; }
QWidget#TileBar { background-color: #1890FF; }

/* 伪状态 */
QPushButton#closeButton:hover { background-color: #E81123; }
QPushButton:pressed { background-color: rgba(0,0,0,0.1); }
```

### 常用属性

```css
background-color: #1890FF;
background: transparent;
color: #ffffff;
font-size: 14px;
font-weight: bold;
border: none;
border-radius: 8px;
border-top-left-radius: 8px;     /* 单独设置某个角 */
border-top-right-radius: 8px;
border-bottom-left-radius: 8px;
border-bottom-right-radius: 8px;
border-top: 1px solid #3D5166;   /* 单边边框 */
padding-left: 20px;
text-align: left;
```

### 在代码中设置样式表

```cpp
// 方式1：直接字符串
widget->setStyleSheet("QWidget { background-color: #fff; }");

// 方式2：原始字符串字面量（推荐，支持多行）
widget->setStyleSheet(R"(
    QWidget#TileBar {
        background-color: #1890FF;
        border-top-left-radius: 8px;
    }
    QPushButton#closeButton:hover {
        background-color: #E81123;
    }
)");
```

### ⚠️ 常见问题

**问题1：父控件的样式表影响子控件**

原因：在父控件上调用 `setStyleSheet()` 时，样式表作用域覆盖所有子孙控件。

解决：将样式表设置在具体的子控件上，而不是顶层父控件：
```cpp
// ❌ 错误：设置在 MainWindow(this) 上，会污染所有子控件
setStyleSheet("QWidget { background: #F5F6FA; }");

// ✅ 正确：设置在具体控件上
contentWidget->setStyleSheet("QWidget#contentWidget { background: #F5F6FA; }");
```

**问题2：`QWidget` 子类 `background-color` 不生效**

原因：`QWidget` 默认不绘制背景。

解决：
```cpp
setAttribute(Qt::WA_StyledBackground, true);
```

**问题3：圆角被子控件背景覆盖**

原因：子控件的背景色填满了整个矩形，遮住了父控件的圆角。

解决：子控件设置 `background: transparent` 或对应角也设置圆角。

---

## 12. 无边框窗口

### 实现步骤

```cpp
// 1. 去掉系统边框
setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);

// 2. 背景透明（用于圆角+阴影）
setAttribute(Qt::WA_TranslucentBackground);

// 3. 为阴影留出边距
QVBoxLayout* outerLayout = new QVBoxLayout(this);
outerLayout->setContentsMargins(SHADOW_MARGIN, SHADOW_MARGIN,
                                SHADOW_MARGIN, SHADOW_MARGIN);

// 4. 在 paintEvent 中手动绘制圆角背景和阴影
```

### 拖拽移动实现

```cpp
void MainWindow::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // 只在标题栏区域允许拖拽
        if (event->position().y() <= SHADOW_MARGIN + TITLE_BAR_HEIGHT) {
            m_dragStartPos = event->globalPosition().toPoint()
                             - frameGeometry().topLeft();
            m_isDragging = true;
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        if (isMaximized()) {
            showNormal();
            m_dragStartPos = QPoint(width() / 2, TITLE_BAR_HEIGHT / 2);
        }
        move(event->globalPosition().toPoint() - m_dragStartPos);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}
```

### ⚠️ 常见问题

**问题：无边框窗口无法在任务栏显示**

解决：添加 `Qt::WindowMinimizeButtonHint` 标志：
```cpp
setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);
```

**问题：最大化时阴影边距仍然存在，导致窗口四周有空白**

解决：在 `changeEvent` 中动态调整外层布局的 margin：
```cpp
void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
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

---

## 13. CMake 与 Qt 集成

### 关键配置

```cmake
# 自动处理 MOC（信号槽元对象系统）、RCC（资源文件）、UIC（.ui 文件）
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt6 组件
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)

# 使用 Qt 标准项目设置
qt_standard_project_setup()

# 链接 Qt 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Windows 下隐藏控制台窗口
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# MSVC 中文源码支持
target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
```

### ⚠️ 常见问题

**问题：`Q_OBJECT` 宏报链接错误（未定义的符号 `vtable`）**

原因：MOC 没有处理该文件。

解决：确保 `CMAKE_AUTOMOC ON` 已设置，并重新运行 CMake 配置。

**问题：修改了头文件中的信号/槽声明后编译报错**

解决：删除 `out/build` 目录，重新 CMake 配置，让 MOC 重新生成。

---

## 14. 常见问题汇总

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| `QWidget` 子类背景色不生效 | 未启用样式背景绘制 | `setAttribute(Qt::WA_StyledBackground, true)` |
| 父控件样式污染子控件 | `setStyleSheet` 设置在父控件上 | 将样式设置在具体子控件上 |
| 圆角有锯齿 | 未开启抗锯齿 | `painter.setRenderHint(QPainter::Antialiasing)` |
| 最大化时有空白边距 | 阴影 margin 未清零 | `changeEvent` 中动态设置 `contentsMargins` |
| 信号槽不响应 | 缺少 `Q_OBJECT` 宏 | 类定义中添加 `Q_OBJECT` |
| Qt6 中 `event->pos()` 报错 | Qt6 废弃了 `pos()` | 改用 `event->position().toPoint()` |
| 控件间有意外间距 | 布局默认 spacing/margin | `setSpacing(0)` + `setContentsMargins(0,0,0,0)` |
| 无边框窗口不在任务栏显示 | 缺少窗口标志 | 添加 `Qt::WindowMinimizeButtonHint` |
| 拖拽时最大化还原位置错误 | dragStartPos 未重置 | 还原时重置为窗口中心位置 |
| 窗口关闭时状态未保存 | closeEvent 未重写 | 重写 `closeEvent` 调用 `saveWindowState()` |
