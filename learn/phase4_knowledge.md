# 第四阶段知识文档：体验打磨核心技术

> 本文档覆盖第四阶段所需的全部核心知识点，包括：`paintEvent` 自绘、`QPropertyAnimation` 动画、拖拽排序、Command 设计模式、系统托盘。

---

## 一、QPainter 自绘控件

### 1.1 为什么要自绘？

Qt 提供了 QSS（样式表）来定制控件外观，但 QSS 有局限性：
- 无法实现复杂的图形效果（渐变、阴影、自定义形状）
- 无法精确控制绘制时机和透明度
- 性能不如直接绘制

**自绘的核心：重写 `paintEvent`**

```cpp
class MyWidget : public QWidget {
protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        // 在这里绘制你想要的内容
    }
};
```

### 1.2 QPainter 基础 API

```cpp
QPainter painter(this);

// ── 渲染质量 ──────────────────────────────────────────────
painter.setRenderHint(QPainter::Antialiasing);          // 抗锯齿（圆形/斜线必须开）
painter.setRenderHint(QPainter::SmoothPixmapTransform); // 图片缩放平滑

// ── 画笔（线条）──────────────────────────────────────────
painter.setPen(Qt::NoPen);                    // 无边框
painter.setPen(QPen(QColor("#1890FF"), 2));   // 蓝色 2px 边框

// ── 画刷（填充）──────────────────────────────────────────
painter.setBrush(QColor(50, 50, 50, 220));    // 深色半透明填充
painter.setBrush(Qt::NoBrush);                // 无填充

// ── 绘制形状 ──────────────────────────────────────────────
painter.drawRect(rect());                     // 矩形
painter.drawRoundedRect(rect(), 8, 8);        // 圆角矩形（圆角半径 8px）
painter.drawEllipse(center, rx, ry);          // 椭圆/圆形
painter.drawLine(p1, p2);                     // 直线

// ── 绘制图片 ──────────────────────────────────────────────
painter.drawPixmap(rect, pixmap);             // 绘制图片到指定区域
painter.drawPixmap(x, y, pixmap);             // 绘制图片到指定坐标

// ── 绘制文字 ──────────────────────────────────────────────
painter.setFont(QFont("Arial", 12));
painter.drawText(rect, Qt::AlignCenter, "Hello");

// ── 透明度 ────────────────────────────────────────────────
painter.setOpacity(0.5);  // 0.0（全透明）~ 1.0（不透明）

// ── 填充矩形（快捷方法）──────────────────────────────────
painter.fillRect(rect(), QColor(0, 0, 0, 30));  // 半透明黑色遮罩
```

### 1.3 三态按钮的绘制逻辑

```
状态机：
  is_hovered_ = false, is_pressed_ = false  →  Normal 态
  is_hovered_ = true,  is_pressed_ = false  →  Hover 态
  is_hovered_ = true,  is_pressed_ = true   →  Pressed 态
  isEnabled() == false                       →  Disabled 态
```

```cpp
void IconButton::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 第一步：根据状态绘制背景
    if (!isEnabled()) {
        // 禁用态：无背景变化
    } else if (is_pressed_) {
        p.fillRect(rect(), QColor(0, 0, 0, 40));   // 深遮罩
    } else if (is_hovered_) {
        p.fillRect(rect(), QColor(0, 0, 0, 15));   // 浅遮罩
    }

    // 第二步：绘制图标
    QPixmap pix = isEnabled() ? currentPixmap() : makeGrayPixmap(normal_icon_);
    if (!pix.isNull()) {
        QRect icon_rect = QRect(
            (width()  - icon_size_.width())  / 2,
            (height() - icon_size_.height()) / 2,
            icon_size_.width(), icon_size_.height()
        );
        p.drawPixmap(icon_rect, pix.scaled(icon_size_,
                     Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}
```

### 1.4 鼠标状态追踪

```cpp
// 追踪 hover 状态
void IconButton::enterEvent(QEnterEvent* event) {
    is_hovered_ = true;
    update();  // ← 触发重绘！
    QPushButton::enterEvent(event);
}

void IconButton::leaveEvent(QEvent* event) {
    is_hovered_ = false;
    is_pressed_ = false;
    update();
    QPushButton::leaveEvent(event);
}

void IconButton::mousePressEvent(QMouseEvent* event) {
    is_pressed_ = true;
    update();
    QPushButton::mousePressEvent(event);
}

void IconButton::mouseReleaseEvent(QMouseEvent* event) {
    is_pressed_ = false;
    update();
    QPushButton::mouseReleaseEvent(event);
}
```

> 💡 **关键点**：每次状态改变后必须调用 `update()`，它会触发 Qt 调度一次 `paintEvent`。
> 不要直接调用 `paintEvent`，要通过 `update()` 让 Qt 决定何时重绘。

### 1.5 图像灰度化（禁用态）

```cpp
QPixmap IconButton::makeGrayPixmap(const QPixmap& src) {
    // 1. 转换为 ARGB32 格式（支持透明度）
    QImage img = src.toImage().convertToFormat(QImage::Format_ARGB32);

    // 2. 逐像素处理
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            QRgb pixel = img.pixel(x, y);
            // qGray() 计算灰度值（加权平均：0.299R + 0.587G + 0.114B）
            int gray = qGray(pixel);
            // 保留原始透明度，但降低一半（视觉上更暗淡）
            img.setPixel(x, y, qRgba(gray, gray, gray, qAlpha(pixel) / 2));
        }
    }

    return QPixmap::fromImage(img);
}
```

---

## 二、QPropertyAnimation 动画系统

### 2.1 动画的核心概念

Qt 动画系统基于**属性动画**：通过在一段时间内改变对象的某个属性值，实现动画效果。

```
时间轴：  0ms ──────────────────── 300ms
属性值：  0.0 ──── 插值计算 ──────── 1.0
```

### 2.2 Q_PROPERTY 宏

要让 `QPropertyAnimation` 驱动自定义属性，必须用 `Q_PROPERTY` 声明：

```cpp
class ToastWidget : public QWidget {
    Q_OBJECT
    // 格式：Q_PROPERTY(类型 属性名 READ getter WRITE setter)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    qreal opacity() const { return opacity_; }
    void  setOpacity(qreal val) {
        opacity_ = val;
        update();  // 属性变化时触发重绘
    }

private:
    qreal opacity_ = 0.0;
};
```

### 2.3 QPropertyAnimation 基本用法

```cpp
// 创建动画：对 toast 对象的 "opacity" 属性做动画
auto* anim = new QPropertyAnimation(toast, "opacity", this);
anim->setDuration(200);          // 持续 200ms
anim->setStartValue(0.0);        // 起始值
anim->setEndValue(1.0);          // 结束值
anim->setEasingCurve(QEasingCurve::OutCubic);  // 缓动曲线（先快后慢）
anim->start();
```

### 2.4 串联动画（QSequentialAnimationGroup）

Toast 需要：渐入 → 停留 → 渐出，这是串联动画的典型场景：

```cpp
// 方案一：用 QSequentialAnimationGroup
auto* group = new QSequentialAnimationGroup(this);

// 渐入（200ms）
auto* anim_in = new QPropertyAnimation(this, "opacity");
anim_in->setDuration(200);
anim_in->setStartValue(0.0);
anim_in->setEndValue(1.0);

// 停留（1500ms）：用 QPauseAnimation
auto* pause = new QPauseAnimation(1500);

// 渐出（300ms）
auto* anim_out = new QPropertyAnimation(this, "opacity");
anim_out->setDuration(300);
anim_out->setStartValue(1.0);
anim_out->setEndValue(0.0);

group->addAnimation(anim_in);
group->addAnimation(pause);
group->addAnimation(anim_out);

// 动画结束后删除 Toast
connect(group, &QSequentialAnimationGroup::finished, this, [this]() {
    emit finished();
    deleteLater();
});

group->start(QAbstractAnimation::DeleteWhenStopped);
```

```cpp
// 方案二：用 QTimer 手动控制（更直观）
// 渐入
auto* anim_in = new QPropertyAnimation(this, "opacity", this);
anim_in->setDuration(200);
anim_in->setStartValue(0.0);
anim_in->setEndValue(1.0);
anim_in->start();

// 渐入结束后，等待 1500ms 再渐出
connect(anim_in, &QPropertyAnimation::finished, this, [this]() {
    QTimer::singleShot(1500, this, [this]() {
        auto* anim_out = new QPropertyAnimation(this, "opacity", this);
        anim_out->setDuration(300);
        anim_out->setStartValue(1.0);
        anim_out->setEndValue(0.0);
        anim_out->start();
        connect(anim_out, &QPropertyAnimation::finished, this, [this]() {
            emit finished();
            deleteLater();
        });
    });
});
```

### 2.5 缓动曲线（QEasingCurve）

```cpp
// 常用缓动曲线
QEasingCurve::Linear      // 匀速
QEasingCurve::OutCubic    // 先快后慢（最自然）
QEasingCurve::InOutQuad   // 先慢后快再慢（平滑）
QEasingCurve::OutBounce   // 弹跳效果
```

### 2.6 Toast 的 paintEvent

```cpp
void ToastWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 用 opacity_ 控制整体透明度
    painter.setOpacity(opacity_);

    // 绘制圆角矩形背景
    painter.setBrush(QColor(50, 50, 50, 220));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), height() / 2, height() / 2);  // 胶囊形

    // 绘制文字
    painter.setOpacity(1.0);  // 文字不受背景透明度影响
    painter.setPen(Qt::white);
    painter.setFont(QFont("", 13));
    painter.drawText(rect(), Qt::AlignCenter, label_->text());
}
```

---

## 三、QListView 拖拽排序

### 3.1 Qt 拖拽机制概述

Qt 的拖拽（Drag & Drop）基于 MIME 数据传输：

```
拖拽源（Drag Source）
  └── startDrag() → 创建 QDrag 对象 → 携带 QMimeData

拖拽目标（Drop Target）
  ├── dragEnterEvent()  ← 拖拽进入时
  ├── dragMoveEvent()   ← 拖拽移动时
  ├── dragLeaveEvent()  ← 拖拽离开时
  └── dropEvent()       ← 放下时
```

### 3.2 QListView 内置拖拽

`QListView` 已经内置了拖拽支持，只需配置：

```cpp
// 在 NoteListView 构造函数中
setDragEnabled(true);                                    // 允许拖拽
setAcceptDrops(true);                                    // 允许接受放下
setDropIndicatorShown(true);                             // 显示插入线
setDragDropMode(QAbstractItemView::InternalMove);        // 内部移动模式
setDefaultDropAction(Qt::MoveAction);                    // 默认移动（不复制）
```

### 3.3 Model 需要实现的接口

```cpp
// 1. flags() —— 告诉 Qt 哪些 item 可以拖拽/接受放下
Qt::ItemFlags NoteListModel::flags(const QModelIndex& index) const {
    Qt::ItemFlags f = QAbstractListModel::flags(index);
    if (index.isValid())
        return f | Qt::ItemIsDragEnabled;   // 有效 item：可拖拽
    else
        return f | Qt::ItemIsDropEnabled;   // 根节点：可接受放下
}

// 2. supportedDropActions() —— 支持的放下动作
Qt::DropActions NoteListModel::supportedDropActions() const {
    return Qt::MoveAction;
}

// 3. moveRows() —— 实际执行移动
bool NoteListModel::moveRows(const QModelIndex& srcParent, int srcRow, int count,
                              const QModelIndex& dstParent, int dstChild) {
    // beginMoveRows 的第5个参数是"目标位置"
    // 注意：当向下移动时，目标位置需要 +1（Qt 的规定）
    if (!beginMoveRows(srcParent, srcRow, srcRow + count - 1,
                       dstParent, dstChild)) {
        return false;  // Qt 认为这次移动无效（如移动到自身）
    }

    // 执行数据移动
    NoteData note = notes_.takeAt(srcRow);
    int insert_at = (dstChild > srcRow) ? dstChild - 1 : dstChild;
    notes_.insert(insert_at, note);

    endMoveRows();

    // 更新 sortOrder 并保存
    for (int i = 0; i < notes_.size(); ++i)
        notes_[i].sort_order = i;
    NoteManager::instance()->saveNotes(notes_);

    return true;
}
```

### 3.4 beginMoveRows 的参数规则

```
beginMoveRows(srcParent, srcFirst, srcLast, dstParent, dstChild)

srcFirst ~ srcLast：要移动的行范围
dstChild：目标位置（插入到 dstChild 之前）

特殊规则：
  向下移动时（dstChild > srcLast），dstChild 需要是实际目标行 + 1
  例如：把第 2 行移到第 5 行后面，dstChild = 6（不是 5）

Qt 会自动验证参数合法性，非法时 beginMoveRows 返回 false
```

### 3.5 代理模型的坐标映射问题

当使用 `NoteFilterProxyModel` 时，View 看到的行号是**代理行号**，而 Model 存储的是**源行号**。

```
代理模型（过滤后）：  行0, 行1, 行2
源模型（全部数据）：  行0, 行2, 行5  ← 实际行号不连续！
```

解决方案：在 `NoteFilterProxyModel` 中重写 `dropMimeData`：

```cpp
bool NoteFilterProxyModel::dropMimeData(const QMimeData* data,
                                         Qt::DropAction action,
                                         int row, int column,
                                         const QModelIndex& parent) {
    // 将代理行号映射到源模型行号
    int source_row;
    if (row >= 0) {
        QModelIndex proxy_index = index(row, column, parent);
        QModelIndex source_index = mapToSource(proxy_index);
        source_row = source_index.row();
    } else {
        source_row = -1;
    }

    QModelIndex source_parent = mapToSource(parent);
    return sourceModel()->dropMimeData(data, action, source_row, column, source_parent);
}
```

### 3.6 插入线样式定制

```css
/* 在 QSS 中定制插入线颜色 */
QListView::item:drop-indicator {
    border-top: 2px solid #1890FF;
    background: transparent;
}
```

---

## 四、Command 设计模式（撤销/重做）

### 4.1 为什么需要 Command 模式？

不用 Command 模式时，撤销逻辑会散落在各处：

```cpp
// ❌ 不好的做法：直接操作，无法撤销
void NoteController::onNoteDeleted(int row) {
    model_->removeNote(row);  // 删了就没了
}
```

用 Command 模式后，每个操作都是一个**可逆的对象**：

```cpp
// ✅ 好的做法：封装为命令
class DeleteNoteCommand : public UndoCommand {
    NoteData deleted_note_;  // 保存被删除的数据
    int      row_;

    void execute() override { model_->removeNote(row_); }
    void undo()    override { model_->insertNote(row_, deleted_note_); }
};
```

### 4.2 Command 模式的结构

```
┌─────────────────────────────────────────────────────┐
│                   UndoCommand（抽象）                 │
│  + execute() = 0                                    │
│  + undo()    = 0                                    │
│  + description() = 0                               │
└─────────────────────────────────────────────────────┘
         ↑              ↑              ↑
  AddNoteCommand  DeleteNoteCommand  EditNoteCommand
  execute: 插入   execute: 删除      execute: 更新
  undo:    删除   undo:    插入      undo:    恢复旧数据
```

### 4.3 操作栈的工作原理

```
初始状态：
  undo_stack_: []
  redo_stack_: []

执行操作 A：
  undo_stack_: [A]
  redo_stack_: []

执行操作 B：
  undo_stack_: [A, B]
  redo_stack_: []

撤销（Ctrl+Z）：
  undo_stack_: [A]
  redo_stack_: [B]   ← B 被移到 redo 栈

重做（Ctrl+Y）：
  undo_stack_: [A, B]
  redo_stack_: []    ← B 被移回 undo 栈

执行新操作 C（此时 redo 栈有内容）：
  undo_stack_: [A, B, C]
  redo_stack_: []    ← redo 栈被清空！（新操作覆盖了历史）
```

### 4.4 UndoStack 实现

```cpp
void UndoStack::push(UndoCommandPtr cmd) {
    // 1. 执行命令
    cmd->execute();

    // 2. 入栈
    undo_stack_.push(std::move(cmd));

    // 3. 清空 redo 栈（新操作覆盖历史）
    redo_stack_.clear();

    // 4. 限制栈深度
    while (undo_stack_.size() > MAX_STACK_SIZE) {
        undo_stack_.removeFirst();  // 移除最旧的操作
    }

    emit stackChanged();
}

void UndoStack::undo() {
    if (undo_stack_.isEmpty()) return;

    UndoCommandPtr cmd = std::move(undo_stack_.top());
    undo_stack_.pop();

    cmd->undo();

    redo_stack_.push(std::move(cmd));

    emit stackChanged();
}

void UndoStack::redo() {
    if (redo_stack_.isEmpty()) return;

    UndoCommandPtr cmd = std::move(redo_stack_.top());
    redo_stack_.pop();

    cmd->execute();

    undo_stack_.push(std::move(cmd));

    emit stackChanged();
}
```

### 4.5 std::unique_ptr 与所有权

Command 对象用 `std::unique_ptr` 管理，确保内存安全：

```cpp
// 创建命令（unique_ptr 拥有所有权）
auto cmd = std::make_unique<EditNoteCommand>(model_, row, old_data, new_data);

// 转移所有权到栈（move 后 cmd 变为 nullptr）
undo_stack_->push(std::move(cmd));

// QStack 存储 unique_ptr
QStack<std::unique_ptr<UndoCommand>> undo_stack_;
// 注意：QStack 不支持 unique_ptr 的拷贝，只能 move
// 使用 std::stack 或 QVector 更方便
```

> 💡 **建议**：用 `QVector<UndoCommandPtr>` 代替 `QStack<UndoCommandPtr>`，
> 因为 `QStack` 继承自 `QVector`，但 `unique_ptr` 不可拷贝，
> 直接用 `QVector` 的 `append(std::move(cmd))` 更清晰。

### 4.6 各命令的 execute/undo 对照表

| 命令 | execute | undo |
|------|---------|------|
| `AddNoteCommand` | `model->insertNote(row, data)` | `model->removeNote(row)` |
| `DeleteNoteCommand` | `model->removeNote(row)` | `model->insertNote(row, saved_data)` |
| `EditNoteCommand` | `model->updateNote(row, new_data)` | `model->updateNote(row, old_data)` |
| `PinNoteCommand` | `model->setPinned(row, true/false)` | `model->setPinned(row, !pinned)` |
| `ChangeCategoryCommand` | `model->setCategory(row, new_cat)` | `model->setCategory(row, old_cat)` |
| `MoveNoteCommand` | `model->moveRows(..., new_pos)` | `model->moveRows(..., old_pos)` |

---

## 五、系统托盘（QSystemTrayIcon）

### 5.1 基本用法

```cpp
#include <QSystemTrayIcon>
#include <QMenu>

// 创建托盘图标
QSystemTrayIcon* tray = new QSystemTrayIcon(QIcon(":/icons/app.png"), this);

// 创建右键菜单
QMenu* menu = new QMenu(this);
menu->addAction("显示主窗口", this, &MainWindow::show);
menu->addAction("退出", qApp, &QApplication::quit);
tray->setContextMenu(menu);

// 显示托盘图标
tray->show();

// 显示气泡提示
tray->showMessage("标题", "内容", QSystemTrayIcon::Information, 2000);
```

### 5.2 响应托盘点击

```cpp
connect(tray, &QSystemTrayIcon::activated,
        this, [this](QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
    case QSystemTrayIcon::Trigger:       // 单击
        break;
    case QSystemTrayIcon::DoubleClick:   // 双击
        show();
        raise();           // 提升到最前
        activateWindow();  // 激活窗口（获取焦点）
        break;
    case QSystemTrayIcon::Context:       // 右键（自动显示菜单）
        break;
    default:
        break;
    }
});
```

### 5.3 关闭时隐藏到托盘

```cpp
void MainWindow::closeEvent(QCloseEvent* event) {
    if (tray_icon_ && tray_icon_->isVisible()) {
        // 隐藏窗口，不退出程序
        hide();
        event->ignore();  // ← 关键：阻止默认的关闭行为

        tray_icon_->showMessage(
            "便签管理器",
            "程序已最小化到系统托盘，双击图标可恢复窗口",
            QSystemTrayIcon::Information,
            2000
        );
    } else {
        // 托盘不可用时，正常退出
        saveWindowState();
        event->accept();
    }
}
```

### 5.4 全局快捷键（Windows）

Qt 没有跨平台的全局快捷键 API，Windows 平台使用 Win32 API：

```cpp
// 注册热键
// 参数：窗口句柄（nullptr=全局）, 热键ID, 修饰键, 虚拟键码
::RegisterHotKey(nullptr, HOT_KEY_ID, MOD_CONTROL | MOD_ALT, 'N');

// 注销热键（程序退出时必须注销！）
::UnregisterHotKey(nullptr, HOT_KEY_ID);
```

**接收热键消息**：需要通过 `QAbstractNativeEventFilter` 拦截 Windows 消息：

```cpp
class GlobalHotKey : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT
public:
    GlobalHotKey(QObject* parent = nullptr) : QObject(parent) {
        // 安装全局事件过滤器
        qApp->installNativeEventFilter(this);
    }

    ~GlobalHotKey() {
        qApp->removeNativeEventFilter(this);
        ::UnregisterHotKey(nullptr, 1);
    }

    bool nativeEventFilter(const QByteArray& eventType,
                           void* message, qintptr* result) override {
#ifdef Q_OS_WIN
        if (eventType == "windows_generic_MSG") {
            MSG* msg = static_cast<MSG*>(message);
            if (msg->message == WM_HOTKEY) {
                emit hotKeyPressed(static_cast<int>(msg->wParam));
                return true;  // 事件已处理
            }
        }
#endif
        return false;  // 继续传递
    }

signals:
    void hotKeyPressed(int id);
};
```

---

## 六、常见问题与解决方案

### Q1：`update()` 和 `repaint()` 的区别？

| | `update()` | `repaint()` |
|--|--|--|
| 执行时机 | 异步（下一个事件循环） | 同步（立即执行） |
| 性能 | 好（多次调用会合并） | 差（每次都立即绘制） |
| 使用场景 | 绝大多数情况 | 需要立即看到结果时 |

**结论：几乎总是用 `update()`。**

### Q2：为什么 `QPropertyAnimation` 不生效？

常见原因：
1. 属性没有用 `Q_PROPERTY` 声明
2. 类没有继承 `QObject` 或没有 `Q_OBJECT` 宏
3. `setter` 函数没有调用 `update()`
4. 动画对象被提前销毁（没有设置父对象）

### Q3：拖拽排序后数据没有更新？

检查：
1. `moveRows` 是否正确调用了 `beginMoveRows` / `endMoveRows`
2. `sortOrder` 字段是否在 `moveRows` 后更新
3. 是否调用了 `NoteManager::saveNotes()` 持久化

### Q4：代理模型下拖拽行号错误？

原因：View 传给 Model 的行号是代理行号，需要映射到源行号。
解决：在 `NoteFilterProxyModel` 中重写 `dropMimeData`，用 `mapToSource` 转换。

### Q5：撤销后 UI 没有更新？

检查：
1. `undo()` 中调用的 Model 方法是否发出了 `dataChanged` 信号
2. `NoteListModel` 的修改方法是否正确发出信号

---

## 七、知识点速查表

| 知识点 | 关键 API | 所在功能 |
|--------|---------|---------|
| 自绘控件 | `paintEvent`, `QPainter`, `update()` | IconButton, Toast |
| 鼠标状态追踪 | `enterEvent`, `leaveEvent`, `mousePressEvent` | IconButton |
| 图像处理 | `QImage::convertToFormat`, `qGray()` | IconButton 禁用态 |
| 属性动画 | `Q_PROPERTY`, `QPropertyAnimation` | Toast |
| 串联动画 | `QSequentialAnimationGroup`, `QPauseAnimation` | Toast |
| 拖拽排序 | `setDragEnabled`, `moveRows`, `beginMoveRows` | 卡片排序 |
| 坐标映射 | `mapToSource`, `mapFromSource` | 代理模型拖拽 |
| Command 模式 | `execute()`, `undo()`, `UndoStack` | 撤销/重做 |
| 智能指针 | `std::unique_ptr`, `std::move` | 命令对象管理 |
| 系统托盘 | `QSystemTrayIcon`, `closeEvent` | 托盘功能 |
| 全局热键 | `RegisterHotKey`, `nativeEventFilter` | 全局快捷键 |
