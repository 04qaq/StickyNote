# TitleBar 自定义标题栏深度解析

在 `StickyNoteManager` 项目中，为了实现更美观、现代化的界面，我们通常会去除操作系统默认的窗口边框（通过 `Qt::FramelessWindowHint`）。然而，这样做会导致窗口失去原生的标题栏，从而失去拖拽移动、最小化、最大化和关闭的功能。

因此，我们需要手动实现一个 `TitleBar` 类来接管这些功能。

## 1. 核心类与架构

`TitleBar` 继承自 **`QWidget`**，本质上它就是一个普通的容器控件，被放置在主窗口的最顶部。

```cpp
class TitleBar : public QWidget
{
    Q_OBJECT
    // ...
signals:
    void minimizeRequested();
    void maximizeRequested();
    void closeRequested();
    // ...
};
```

### 设计理念：职责分离
`TitleBar` 并不直接控制窗口的关闭或最小化（尽管它持有这些按钮）。它的职责是：
1.  **显示**：展示标题和按钮。
2.  **交互**：响应鼠标点击和拖拽。
3.  **通知**：当用户点击按钮时，发送 **信号 (Signal)** 给主窗口。主窗口收到信号后，再执行具体的 `close()` 或 `showMinimized()` 操作。

## 2. 界面布局 (`initUI`)

我们在 `initUI()` 函数中构建界面。

```cpp
void TitleBar::initUI()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 8, 0);
    layout->setSpacing(4);

    // 1. 标题文字
    m_titleLabel = new QLabel("便签管理器", this);
    layout->addWidget(m_titleLabel);

    // 2. 弹簧 (关键)
    layout->addStretch(); 

    // 3. 功能按钮 (最小化、最大化、关闭)
    // ... 创建按钮并添加到 layout ...
}
```

### 关键技术点
*   **`QHBoxLayout` (水平布局)**：让子控件从左到右水平排列。
*   **`setContentsMargins(12, 0, 8, 0)`**：
    *   左边距 12px：防止标题文字紧贴窗口左边缘。
    *   右边距 8px：防止关闭按钮紧贴窗口右边缘。
    *   上下边距 0px：让控件在垂直方向上撑满标题栏高度。
*   **`layout->addStretch()`**：这是一个“占位弹簧”。它会占据所有多余的水平空间，从而将左边的 `m_titleLabel` 推到最左，将右边的按钮组推到最右。

## 3. 窗口拖拽实现 (核心逻辑)

由于没有系统边框，我们需要自己处理鼠标事件来实现窗口移动。

### 原理
移动公式：`窗口新位置 = 鼠标当前绝对位置 - 鼠标按下时的相对偏移量`

### 代码解析

**1. 记录起始点 (`mousePressEvent`)**
当用户在标题栏按下鼠标左键时：
```cpp
void TitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 计算偏移量 = 鼠标全局坐标 - 窗口左上角坐标
        m_dragStartPos = event->globalPosition().toPoint() - window()->frameGeometry().topLeft();
        m_isDragging = true;
        event->accept(); // 标记事件已处理
    }
}
```

**2. 实时移动 (`mouseMoveEvent`)**
当用户按住左键移动鼠标时：
```cpp
void TitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        QWidget* w = window(); // 获取顶层窗口 (MainWindow)
        
        // 特殊处理：如果是最大化状态，拖拽时通常需要先还原窗口
        if (w->isMaximized()) {
            w->showNormal();
            // 还原后，调整偏移量，让鼠标位于窗口标题栏中间，体验更好
            m_dragStartPos = QPoint(w->width() / 2, height() / 2);
        }
        
        // 执行移动：新位置 = 当前鼠标全局位置 - 偏移量
        w->move(event->globalPosition().toPoint() - m_dragStartPos);
        event->accept();
    }
}
```

**3. 结束拖拽 (`mouseReleaseEvent`)**
```cpp
void TitleBar::mouseReleaseEvent(QMouseEvent* event)
{
    m_isDragging = false;
}
```

## 4. 双击最大化 (`mouseDoubleClickEvent`)

现代软件通常支持双击标题栏来切换最大化/还原状态。

```cpp
void TitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit maximizeRequested(); // 发送信号给 MainWindow
        event->accept();
    }
}
```

## 5. 样式定制 (QSS)

我们在 `applyStyle()` 中使用 Qt Style Sheets (QSS) 来定义外观。这类似于 Web 开发中的 CSS。

```cpp
void TitleBar::applyStyle()
{
    setStyleSheet(R"(
        // 设置标题栏背景色和圆角
        TitleBar {
            background-color: #1890FF;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
        }

        // 定义按钮的基础样式
        QPushButton#minButton, QPushButton#maxButton, QPushButton#closeButton {
            color: #ffffff;
            background-color: transparent; // 默认透明背景
            border: none;
            border-radius: 4px;
        }

        // 悬停状态 (Hover) - 增加半透明白色遮罩效果
        QPushButton#minButton:hover, QPushButton#maxButton:hover {
            background-color: rgba(255, 255, 255, 0.2);
        }

        // 关闭按钮的特殊处理 - 悬停变为红色
        QPushButton#closeButton:hover {
            background-color: #E53935;
        }
    )");
}
```

### 为什么使用 `R"()"` ?
这是 C++11 的 **原始字符串字面量 (Raw String Literal)**。它允许我们在字符串中直接换行，而不需要使用 `\n` 或连接符，非常适合编写长段的 CSS 代码。

## 6. 总结

`TitleBar` 的实现展示了 Qt 自定义控件的几个核心方面：
1.  **组合模式**：将 Label 和 Button 组合在一起。
2.  **事件处理**：重写 `mouseEvent` 实现自定义交互（拖拽）。
3.  **信号槽**：通过信号与父窗口解耦。
4.  **QSS**：通过样式表实现现代化外观。
