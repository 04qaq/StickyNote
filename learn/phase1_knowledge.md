# StickyNoteManager 第一阶段 · 代码知识详解与设计思路

---

## 一、Qt 事件系统深度解析

### 1.1 事件传递机制

Qt 的鼠标事件遵循 **"谁接收，谁处理"** 的原则，**不会自动向上冒泡**：

```
鼠标移动到某个控件上
        ↓
该控件的 mouseMoveEvent() 被调用
        ↓
如果该控件调用了 event->accept()（或默认接受），事件停止传递
        ↓
父控件的 mouseMoveEvent() 【不会】被调用
```

这与 Web 的事件冒泡机制完全不同，是 Qt 初学者最容易踩的坑。

本项目中，`MainWindow` 的绝大部分区域被 `TileBar`、`SideBar`、`content_widget_` 等子控件覆盖，如果不做特殊处理，`MainWindow::mouseMoveEvent` 几乎永远不会被调用，导致光标无法更新、缩放无法触发。

### 1.2 事件过滤器机制

事件过滤器是解决"父控件无法接收子控件事件"问题的标准方案：

```
鼠标移动到子控件
        ↓
QApplication 级别的事件过滤器（如果有）
        ↓
子控件父链上所有安装了过滤器的对象  ← MainWindow 的过滤器在这里执行
        ↓
子控件自身的 mouseMoveEvent()
```

安装在 `MainWindow` 上的过滤器会在子控件的 `mouseMoveEvent` **之前**执行，因此可以在不阻止子控件正常工作的前提下更新光标。

**安装方式：**

```cpp
// 为所有子孙控件安装过滤器（包括孙子控件）
for (QWidget* w : findChildren<QWidget*>()) {
    w->installEventFilter(this);
    w->setMouseTracking(true);
}
```

注意必须用 `findChildren<QWidget*>()` 而不是只遍历直接子控件，否则孙子控件（如 TileBar 内部的按钮）不会被覆盖到。

### 1.3 setMouseTracking 的作用

```
setMouseTracking(false)  →  只有按下鼠标键时才触发 mouseMoveEvent（默认）
setMouseTracking(true)   →  鼠标悬停时也触发 mouseMoveEvent
```

本项目中，所有子控件都需要开启 `setMouseTracking(true)`，否则悬停时不会触发 `MouseMove` 事件，光标就无法更新。

### 1.4 事件过滤器的返回值

```cpp
bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (/* 需要处理的事件 */) {
        // 处理逻辑...
    }
    // 必须调用父类实现，让事件继续传递给子控件
    return QWidget::eventFilter(watched, event);
}
```

- 返回 `true`：事件被拦截，子控件不再收到该事件
- 返回 `false`（或调用父类）：事件继续传递给子控件

本项目中应返回父类实现，不拦截事件，让子控件仍能正常响应鼠标操作。

---

## 二、坐标系统详解

### 2.1 Qt 中的三种坐标系

| 坐标系 | 说明 | 获取方式 |
|--------|------|---------|
| 控件本地坐标 | 相对于控件自身左上角，左上角始终是 (0,0) | `event->position().toPoint()` |
| 全局屏幕坐标 | 相对于屏幕左上角 | `event->globalPosition().toPoint()` |
| 父控件坐标 | 相对于父控件左上角 | `mapToParent(localPos)` |

### 2.2 坐标转换

```cpp
// 子控件坐标 → 祖先控件坐标（跨层级）
QPoint posInWindow = childWidget->mapTo(mainWindow, localPos);

// 子控件坐标 → 全局屏幕坐标
QPoint globalPos = childWidget->mapToGlobal(localPos);

// 全局坐标 → 控件本地坐标
QPoint localPos = widget->mapFromGlobal(globalPos);
```

**本项目中的典型用法：**

```cpp
// eventFilter 中：将子控件坐标转换为 MainWindow 坐标，用于边缘检测
QPoint posInWindow = w->mapTo(this, me->position().toPoint());
ResizeEdge edge = window_helper_->hitTest(posInWindow);
```

### 2.3 rect() 与 frameGeometry() 的区别

```cpp
rect()            // 控件自身矩形，左上角始终是 (0,0)，不含屏幕位置信息
frameGeometry()   // 包含边框的窗口矩形，相对屏幕，含 x/y 位置信息
geometry()        // 不含边框的窗口矩形，相对屏幕，含 x/y 位置信息
```

在缩放功能中：
- `startResize` 记录 `frameGeometry()`（含屏幕位置）作为起始状态
- `doResize` 中用 `setGeometry(x, y, w, h)` 设置新的位置和尺寸

### 2.4 QRect::adjusted() 详解

```cpp
QRect content = window_->rect().adjusted(
    shadowMargin_,   // left：左边向右移动（向内缩）
    shadowMargin_,   // top：上边向下移动（向内缩）
    -shadowMargin_,  // right：右边向左移动（向内缩）
    -shadowMargin_   // bottom：下边向上移动（向内缩）
);
```

`adjusted(l, t, r, b)` 的四个参数分别调整矩形的四条边：
- 正值 = 向内缩（left/top 向右/下，right/bottom 向左/上）
- 负值 = 向外扩

这是从"含阴影的完整矩形"得到"视觉内容矩形"的标准写法。

---

## 三、QPainter 绘图系统

### 3.1 绘图流程

```cpp
void MainWindow::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);                          // 1. 创建画家，绑定到控件
    painter.setRenderHint(QPainter::Antialiasing);   // 2. 开启抗锯齿（圆角必须）

    // 3. 设置画笔（轮廓）和画刷（填充）
    painter.setPen(QPen(QColor(0, 0, 0, alpha), 1));
    painter.setBrush(Qt::NoBrush);

    // 4. 创建路径并绘制
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    painter.drawPath(path);
}
```

### 3.2 多层阴影的实现原理

Qt 没有内置的 `box-shadow`，通过绘制多层逐渐透明的圆角矩形轮廓来模拟高斯模糊阴影效果：

```
i = SHADOW_MARGIN (最外层，最透明)
i = SHADOW_MARGIN - 1
...
i = 1 (最内层，最不透明)

alpha = 50 × (1 - i / SHADOW_MARGIN)
```

| i 值 | alpha 值 | 效果 |
|------|---------|------|
| 10（最外层） | 0 | 完全透明 |
| 5 | 25 | 半透明 |
| 1（最内层） | 45 | 接近不透明 |

从外到内，阴影逐渐加深，模拟出渐变阴影效果。

### 3.3 QPainterPath 与圆角矩形

```cpp
QPainterPath path;
// addRoundedRect(矩形, x方向圆角半径, y方向圆角半径)
path.addRoundedRect(rect, CORNER_RADIUS, CORNER_RADIUS);
painter.drawPath(path);
```

`QPainterPath` 是一个可以组合多种形状的路径对象，`addRoundedRect` 添加一个圆角矩形路径。使用路径绘制比直接调用 `drawRoundedRect` 更灵活，可以进行路径裁剪、组合等操作。

---

## 四、信号与槽的设计思路

### 4.1 职责分离原则

`TileBar` 不直接操作父窗口，而是通过信号通知 `MainWindow`：

```
用户点击关闭按钮
        ↓
TileBar::close_button_ 发出 clicked() 信号
        ↓
TileBar 转发为 closeRequested() 信号
        ↓
MainWindow::onCloseRequested() 槽函数
        ↓
MainWindow::close() 执行关闭
```

**好处：**
- `TileBar` 不需要知道父窗口是谁，可以被任何窗口复用
- 测试时可以单独测试 `TileBar` 的信号发出逻辑
- 符合"高内聚、低耦合"的设计原则

### 4.2 信号连接方式

```cpp
// 推荐：函数指针语法，编译期类型检查
connect(tile_bar_, &TileBar::minimizeRequested, this, &MainWindow::onMinimizeRequested);

// 信号连接信号（转发，无需中间槽函数）
connect(close_button_, &QPushButton::clicked, this, &TileBar::closeRequested);
```

### 4.3 Q_OBJECT 宏的作用

```cpp
class TileBar : public QWidget
{
    Q_OBJECT  // 必须有，否则信号槽不工作
    // ...
};
```

`Q_OBJECT` 宏触发 MOC（元对象编译器）生成额外代码，实现信号槽机制、`qobject_cast`、`tr()` 国际化等功能。CMake 中 `CMAKE_AUTOMOC ON` 会自动处理 MOC 步骤。

---

## 五、QSS 样式表设计

### 5.1 选择器优先级

```css
/* 按类型选择（优先级低）*/
QPushButton { color: red; }

/* 按对象名选择（优先级高，需先 setObjectName）*/
QPushButton#closeButton { background: transparent; }

/* 伪状态（最高）*/
QPushButton#closeButton:hover { background-color: #E81123; }
```

### 5.2 常见陷阱

**陷阱1：`QWidget` 子类背景色不生效**

```cpp
// 必须加这一行，否则 QWidget 子类不绘制背景
setAttribute(Qt::WA_StyledBackground, true);
```

原因：`QWidget` 默认不绘制背景，只有 `QFrame` 等子类才会。设置此属性后，QSS 中的 `background-color` 才会生效。

**陷阱2：父控件样式污染子控件**

```cpp
// ❌ 错误：设置在父控件上，会影响所有子孙控件
this->setStyleSheet("QWidget { background: #F5F6FA; }");

// ✅ 正确：设置在具体控件上，用 objectName 精确匹配
contentWidget->setStyleSheet("QWidget#contentWidget { background: #F5F6FA; }");
```

**陷阱3：圆角被子控件背景覆盖**

子控件的背景色填满了整个矩形，遮住了父控件的圆角。解决方案：给子控件设置对应角的 `border-radius`，或设置 `background: transparent`。

### 5.3 原始字符串字面量用于多行 QSS

```cpp
// 使用 R"(...)" 避免转义，多行 QSS 更易读
setStyleSheet(R"(
    QWidget#contentWidget {
        background-color: #F5F6FA;
        border-bottom-right-radius: 8px;
    }
    QPushButton#closeButton {
        background: transparent;
        color: white;
    }
    QPushButton#closeButton:hover {
        background-color: #E81123;
    }
)");
```

---

## 六、窗口缩放的数学原理

### 6.1 边缘检测区域计算

```
内容矩形左边 x = SHADOW_MARGIN = 10
内容矩形右边 x = width - SHADOW_MARGIN
内容矩形上边 y = SHADOW_MARGIN = 10
内容矩形下边 y = height - SHADOW_MARGIN

左边缘检测区域：x ∈ [10 - 6, 10 + 6] = [4, 16]
右边缘检测区域：x ∈ [width-10-6, width-10+6]
上边缘检测区域：y ∈ [4, 16]
下边缘检测区域：y ∈ [height-16, height-4]
```

角的检测是左右边缘与上下边缘的交集，且角的优先级高于边。

### 6.2 缩放计算示例

以拖拽左边缘为例：

```
初始状态：left=100, right=500, width=400
鼠标向右移动 30px：delta.x = 30

newLeft  = 100 + 30 = 130
newRight = 500（不变）
新宽度   = 500 - 130 = 370

setGeometry(130, top, 370, height)
```

以拖拽左上角为例：

```
初始状态：left=100, top=100, right=500, bottom=400
鼠标向右下移动 (30, 20)：delta = (30, 20)

newLeft   = 100 + 30 = 130
newTop    = 100 + 20 = 120
newRight  = 500（不变）
newBottom = 400（不变）

setGeometry(130, 120, 370, 280)
```

### 6.3 最小尺寸约束的逻辑

**原则：哪条边在动，就约束哪条边；固定不动的那条边保持不变。**

```
拖拽左边缘，宽度不足时：
    固定右边（不动），调整左边 = 右边 - 最小宽度
    newLeft = newRight - minWidth

拖拽右边缘，宽度不足时：
    固定左边（不动），调整右边 = 左边 + 最小宽度
    newRight = newLeft + minWidth
```

如果反过来处理（拖左边时固定左边），会导致窗口在达到最小尺寸后，左边缘"跳跃"到右边，产生视觉抖动。

### 6.4 setGeometry 的参数含义

```cpp
// setGeometry(x, y, width, height)
// x, y：窗口左上角相对屏幕的坐标
// width, height：窗口宽度和高度
window_->setGeometry(newLeft, newTop,
                     newRight  - newLeft,   // 宽度 = 右边 - 左边
                     newBottom - newTop);   // 高度 = 下边 - 上边
```

注意：`setGeometry` 的第三、四个参数是**尺寸**（width/height），不是右下角坐标。

---

## 七、设计模式总结

### 7.1 职责分离（Single Responsibility Principle）

| 类 | 单一职责 |
|----|---------|
| `MainWindow` | 组装和协调，不处理具体的缩放计算 |
| `TileBar` | 只负责标题栏 UI 和发出信号，不操作父窗口 |
| `SideBar` | 只负责侧边栏 UI |
| `WindowHelper` | 只负责缩放逻辑，不关心窗口的其他功能 |

### 7.2 观察者模式（信号与槽）

Qt 的信号与槽机制是观察者模式的实现：

```
TileBar（被观察者）→ 发出信号 → MainWindow（观察者）→ 执行槽函数
```

`TileBar` 不需要持有 `MainWindow` 的指针，两者通过信号槽解耦。

### 7.3 策略模式（WindowHelper）

`WindowHelper` 将缩放策略从 `MainWindow` 中抽离，`MainWindow` 只需调用四个接口：

```cpp
window_helper_->hitTest(pos);           // 检测边缘
window_helper_->startResize(edge, pos); // 开始缩放
window_helper_->doResize(pos);          // 执行缩放
window_helper_->stopResize();           // 结束缩放
```

不需要知道内部实现细节，符合"依赖接口而非实现"的原则。

### 7.4 模板方法模式（Qt 事件重写）

Qt 的事件系统使用模板方法模式：基类 `QWidget` 定义了事件处理的框架，子类重写具体步骤，并在末尾调用父类实现保留默认行为：

```cpp
void MainWindow::resizeEvent(QResizeEvent* event)
{
    // 自定义逻辑
    update();
    // 必须调用父类实现，保留默认行为
    QWidget::resizeEvent(event);
}
```

---

## 八、Qt6 与 Qt5 的关键差异

本项目使用 Qt6，以下是开发中遇到的 Qt6 变化：

| Qt5 写法 | Qt6 写法 | 说明 |
|---------|---------|------|
| `event->pos()` | `event->position().toPoint()` | 鼠标事件本地坐标 |
| `event->globalPos()` | `event->globalPosition().toPoint()` | 鼠标事件全局坐标 |
| `QApplication::desktop()` | `QApplication::primaryScreen()` | 获取屏幕信息 |
| `qt5_add_resources` | `qt_add_resources` | CMake 资源文件 |
| `find_package(Qt5 ...)` | `find_package(Qt6 ...)` | CMake 查找包 |

---

## 九、常见问题速查表

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 光标不随位置变化 | 子控件消费了鼠标事件，父窗口收不到 | 使用事件过滤器拦截子控件事件 |
| 光标设置了但不生效 | 子控件光标覆盖父控件光标 | 将光标设置到鼠标所在的子控件上 |
| 缩放时窗口抖动 | 基于上一帧计算，累积误差 | 基于起始状态+总偏移计算 |
| 最大化时四周有空白 | 阴影 margin 未清零 | `changeEvent` 中动态设置 `contentsMargins` |
| 圆角有锯齿 | 未开启抗锯齿 | `painter.setRenderHint(QPainter::Antialiasing)` |
| 背景色不生效 | `QWidget` 默认不绘制背景 | `setAttribute(Qt::WA_StyledBackground, true)` |
| 样式污染子控件 | `setStyleSheet` 设置在父控件上 | 将样式设置在具体子控件上，用 objectName 精确匹配 |
| 信号槽不响应 | 缺少 `Q_OBJECT` 宏 | 类定义中添加 `Q_OBJECT` |
| 无边框窗口不在任务栏 | 缺少窗口标志 | 添加 `Qt::WindowMinimizeButtonHint` |
| Qt6 中 `event->pos()` 报错 | Qt6 废弃了 `pos()` | 改用 `event->position().toPoint()` |
| 孙子控件未安装过滤器 | 只遍历了直接子控件 | 用 `findChildren<QWidget*>()` 递归安装 |
| 窗口关闭时状态未保存 | `closeEvent` 未重写 | 重写 `closeEvent` 调用 `saveWindowState()` |
| 最大化后拖拽位置偏移 | 还原后窗口尺寸变化，偏移量未更新 | `showNormal()` 后重置 `m_dragStartPos` |
