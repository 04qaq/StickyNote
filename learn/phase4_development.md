# 第四阶段开发方案：体验打磨

> **阶段目标（Day 8-9）**：三态按钮+禁用态；Toast 排队机制；卡片拖拽排序；撤销/重做操作栈；系统托盘+全局快捷键

---

## 一、阶段概览

第四阶段是在前三阶段功能完备的基础上，专注于**体验打磨**。这一阶段的核心挑战在于：

| 功能模块 | 技术难点 | 预估工时 |
|---------|---------|---------|
| 自定义图标按钮（IconButton） | `paintEvent` 自绘三态 | 1h |
| Toast 轻提示 | 动画队列管理 | 1.5h |
| 卡片拖拽排序 | `QListView` 拖拽事件重写 | 3h |
| 撤销/重做操作栈 | Command 设计模式 | 3h |
| 系统托盘+全局快捷键 | `QSystemTrayIcon` + 系统热键 | 1.5h |

---

## 二、功能 3.10：自定义图标按钮（IconButton）

### 2.1 需求分析

需要一个三态（normal / hover / pressed）+ 禁用态的自定义按钮，用于标题栏和工具栏。

### 2.2 文件规划

```
src/ui/components/
├── iconbutton.h      ← 新建
└── iconbutton.cpp    ← 新建
```

### 2.3 头文件设计

```cpp
// src/ui/components/iconbutton.h
#pragma once
#include <QPushButton>
#include <QPixmap>

// ============================================================
// IconButton —— 三态自绘图标按钮
//   · 三态：normal / hover / pressed
//   · 禁用态：图标灰显（opacity 降低）
//   · 通过 paintEvent 手动绘制，不依赖 QSS
// ============================================================
class IconButton : public QPushButton {
    Q_OBJECT
public:
    explicit IconButton(QWidget* parent = nullptr);

    // 设置三态图标（可只设置 normal，hover/pressed 自动生成）
    void setNormalIcon(const QPixmap& pixmap);
    void setHoverIcon(const QPixmap& pixmap);
    void setPressedIcon(const QPixmap& pixmap);

    // 设置图标尺寸（默认 16x16）
    void setIconSize(const QSize& size);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;   // Qt6 用 QEnterEvent
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    // 根据当前状态返回对应图标
    QPixmap currentPixmap() const;

    // 生成灰显版本（禁用态）
    static QPixmap makeGrayPixmap(const QPixmap& src);

    QPixmap normal_icon_;
    QPixmap hover_icon_;
    QPixmap pressed_icon_;
    QSize   icon_size_  = {16, 16};

    bool is_hovered_ = false;
    bool is_pressed_ = false;
};
```

### 2.4 实现要点

```cpp
// src/ui/components/iconbutton.cpp

void IconButton::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 1. 绘制背景（hover/pressed 时有背景色）
    if (!isEnabled()) {
        // 禁用态：无背景
    } else if (is_pressed_) {
        painter.fillRect(rect(), QColor(0, 0, 0, 30));  // 深色遮罩
    } else if (is_hovered_) {
        painter.fillRect(rect(), QColor(0, 0, 0, 15));  // 浅色遮罩
    }

    // 2. 绘制图标（居中）
    QPixmap pix = isEnabled() ? currentPixmap() : makeGrayPixmap(normal_icon_);
    if (pix.isNull()) return;

    QRect icon_rect(
        (width()  - icon_size_.width())  / 2,
        (height() - icon_size_.height()) / 2,
        icon_size_.width(),
        icon_size_.height()
    );
    painter.drawPixmap(icon_rect, pix.scaled(icon_size_, Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation));
}

// 生成灰显图标：将图标转为灰度并降低透明度
QPixmap IconButton::makeGrayPixmap(const QPixmap& src) {
    QImage img = src.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            QRgb pixel = img.pixel(x, y);
            int gray = qGray(pixel);
            img.setPixel(x, y, qRgba(gray, gray, gray, qAlpha(pixel) / 2));
        }
    }
    return QPixmap::fromImage(img);
}
```

### 2.5 学习重点

- `QPainter` 的 `fillRect` 绘制背景遮罩
- `QImage::convertToFormat` 图像格式转换
- `qGray()` 计算灰度值
- `enterEvent` / `leaveEvent` 追踪鼠标悬浮状态

---

## 三、功能 3.11：Toast 轻提示

### 3.1 需求分析

- 底部居中弹出，1.5 秒后渐隐消失
- **多条排队**：不重叠，依次展示

### 3.2 文件规划

```
src/ui/components/
├── toastwidget.h     ← 单条 Toast 控件
├── toastwidget.cpp
├── toastmanager.h    ← Toast 队列管理器（单例）
└── toastmanager.cpp
```

### 3.3 架构设计

```
ToastManager（单例）
  ├── 维护 QQueue<QString> 消息队列
  ├── 当前无 Toast 显示时，取队首创建 ToastWidget
  └── ToastWidget 消失后，触发 ToastManager 显示下一条

ToastWidget（单个 Toast）
  ├── 无边框、透明背景的 QWidget
  ├── 渐入（200ms）→ 停留（1500ms）→ 渐出（300ms）
  └── 消失后发出 finished 信号通知 ToastManager
```

### 3.4 ToastWidget 头文件

```cpp
// src/ui/components/toastwidget.h
#pragma once
#include <QWidget>
#include <QLabel>
#include <QPropertyAnimation>

class ToastWidget : public QWidget {
    Q_OBJECT
    // 声明 opacity 属性，供 QPropertyAnimation 驱动
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit ToastWidget(const QString& message, QWidget* parent = nullptr);

    // 开始显示动画
    void show();

    qreal opacity() const { return opacity_; }
    void  setOpacity(qreal val);

signals:
    void finished();  // 动画结束，通知 Manager 显示下一条

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void startAnimation();

    QLabel*             label_     = nullptr;
    QPropertyAnimation* anim_in_   = nullptr;  // 渐入
    QPropertyAnimation* anim_out_  = nullptr;  // 渐出
    QTimer*             timer_     = nullptr;   // 停留计时器
    qreal               opacity_   = 0.0;
};
```

### 3.5 ToastManager 头文件

```cpp
// src/ui/components/toastmanager.h
#pragma once
#include <QObject>
#include <QQueue>

class ToastWidget;

class ToastManager : public QObject {
    Q_OBJECT
public:
    // 单例访问
    static ToastManager* instance();

    // 设置父窗口（Toast 需要相对父窗口定位）
    void setParentWidget(QWidget* parent);

    // 显示一条 Toast（线程安全，可从任意地方调用）
    void show(const QString& message);

private:
    explicit ToastManager(QObject* parent = nullptr);
    void showNext();  // 从队列取出下一条显示

    QWidget*         parent_widget_ = nullptr;
    QQueue<QString>  queue_;
    bool             is_showing_ = false;
};
```

### 3.6 实现要点

```cpp
// ToastWidget 的 paintEvent：圆角半透明背景
void ToastWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 整体透明度由 opacity_ 控制
    painter.setOpacity(opacity_);

    // 绘制圆角矩形背景
    painter.setBrush(QColor(50, 50, 50, 220));  // 深色半透明
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 20, 20);
}

// ToastManager::show 的定位逻辑
void ToastManager::showNext() {
    if (queue_.isEmpty() || is_showing_) return;
    is_showing_ = true;

    auto* toast = new ToastWidget(queue_.dequeue(), parent_widget_);

    // 定位到父窗口底部居中
    int x = (parent_widget_->width() - toast->width()) / 2;
    int y = parent_widget_->height() - toast->height() - 30;
    toast->move(x, y);

    connect(toast, &ToastWidget::finished, this, [this]() {
        is_showing_ = false;
        showNext();  // 递归显示下一条
    });

    toast->show();
}
```

### 3.7 学习重点

- `Q_PROPERTY` 宏声明自定义属性，让 `QPropertyAnimation` 能驱动它
- `QPropertyAnimation` 的 `setStartValue` / `setEndValue` / `setDuration`
- `QSequentialAnimationGroup` 串联多个动画
- 单例模式在 Qt 中的实现（`static` 局部变量）

---

## 四、功能 3.14：卡片拖拽排序

### 4.1 需求分析

- 鼠标长按卡片后拖拽，改变排列顺序
- 拖拽时卡片半透明跟随，目标位置显示插入线
- 放下后更新 `sortOrder` 并持久化

### 4.2 实现方案

Qt 的 `QListView` 内置了拖拽支持，但需要配合 Model 实现 `moveRows`。

**方案选择：使用 Qt 内置拖拽机制**

```
QListView::setDragEnabled(true)
QListView::setAcceptDrops(true)
QListView::setDropIndicatorShown(true)
QListView::setDragDropMode(QAbstractItemView::InternalMove)
```

同时在 `NoteListModel` 中实现：
- `flags()` 返回 `Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled`
- `supportedDropActions()` 返回 `Qt::MoveAction`
- `moveRows()` 执行实际的数据移动

### 4.3 NoteListModel 需要新增的方法

```cpp
// notelistmodel.h 新增
Qt::ItemFlags flags(const QModelIndex& index) const override;
Qt::DropActions supportedDropActions() const override;
bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
              const QModelIndex& destinationParent, int destinationChild) override;
```

```cpp
// notelistmodel.cpp 实现
Qt::ItemFlags NoteListModel::flags(const QModelIndex& index) const {
    Qt::ItemFlags default_flags = QAbstractListModel::flags(index);
    if (index.isValid())
        return default_flags | Qt::ItemIsDragEnabled;
    else
        return default_flags | Qt::ItemIsDropEnabled;
}

bool NoteListModel::moveRows(const QModelIndex&, int src, int count,
                              const QModelIndex&, int dst) {
    // 1. 边界检查
    if (src < 0 || src + count > notes_.size()) return false;

    // 2. 通知 Qt 数据即将移动（必须调用！）
    beginMoveRows({}, src, src + count - 1, {}, dst > src ? dst + 1 : dst);

    // 3. 执行数据移动
    // 使用 std::rotate 或手动 insert + erase
    NoteData note = notes_.takeAt(src);
    int insert_pos = (dst > src) ? dst - 1 : dst;
    notes_.insert(insert_pos, note);

    endMoveRows();

    // 4. 更新 sortOrder 字段并持久化
    updateSortOrders();
    NoteManager::instance()->saveNotes(notes_);

    return true;
}

void NoteListModel::updateSortOrders() {
    for (int i = 0; i < notes_.size(); ++i) {
        notes_[i].sort_order = i;
    }
}
```

### 4.4 NoteListView 配置

```cpp
// notelistview.cpp 构造函数中添加
setDragEnabled(true);
setAcceptDrops(true);
setDropIndicatorShown(true);
setDragDropMode(QAbstractItemView::InternalMove);
setDefaultDropAction(Qt::MoveAction);
```

### 4.5 插入线视觉效果

Qt 默认的 `dropIndicator` 样式可以通过 QSS 定制：

```css
QListView::item:drop-indicator {
    border-top: 2px solid #1890FF;
}
```

### 4.6 注意事项

> ⚠️ **重要**：`NoteFilterProxyModel` 代理模型会拦截拖拽事件！
>
> 当使用代理模型时，拖拽的行号是代理模型的行号，需要在 `moveRows` 前将其映射回源模型行号。
>
> 解决方案：在 `NoteFilterProxyModel` 中也重写 `dropMimeData`，将代理行号转换后再调用源模型的 `moveRows`。

```cpp
// notefilterproxymodel.cpp
bool NoteFilterProxyModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
                                         int row, int column,
                                         const QModelIndex& parent) {
    // 将代理行号映射到源模型行号
    QModelIndex source_parent = mapToSource(parent);
    int source_row = (row >= 0) ? mapToSource(index(row, column, parent)).row() : row;
    return sourceModel()->dropMimeData(data, action, source_row, column, source_parent);
}
```

### 4.7 学习重点

- `QAbstractItemModel::beginMoveRows` / `endMoveRows` 的正确调用时机
- `Qt::ItemFlags` 的组合使用
- 代理模型（ProxyModel）中坐标映射的重要性
- `QListView` 内置拖拽机制的工作原理

---

## 五、功能 3.15：撤销/重做操作栈

### 5.1 需求分析

- 支持：新建、删除、编辑、置顶、分类变更、排序变更
- 栈深度 20 步
- `Ctrl+Z` 撤销，`Ctrl+Y` 重做
- 工具栏按钮随栈状态变化（可用/禁用）

### 5.2 Command 设计模式

这是第四阶段最重要的设计模式！

```
UndoCommand（抽象基类）
  ├── execute()   ← 执行操作
  ├── undo()      ← 撤销操作
  └── description() ← 操作描述（调试用）

具体命令类：
  ├── AddNoteCommand      ← 新建便签
  ├── DeleteNoteCommand   ← 删除便签
  ├── EditNoteCommand     ← 编辑便签
  ├── PinNoteCommand      ← 置顶/取消置顶
  ├── ChangeCategoryCommand ← 更改分类
  └── MoveNoteCommand     ← 拖拽排序
```

### 5.3 文件规划

```
src/core/
├── undocommand.h         ← 抽象基类
├── undostack.h           ← 操作栈管理器
├── undostack.cpp
└── commands/
    ├── addnotecommand.h/.cpp
    ├── deletenotecommand.h/.cpp
    ├── editnotecommand.h/.cpp
    ├── pinnotecommand.h/.cpp
    ├── changecategorycommand.h/.cpp
    └── movenotecommand.h/.cpp
```

### 5.4 抽象基类设计

```cpp
// src/core/undocommand.h
#pragma once
#include <QString>
#include <memory>

// ============================================================
// UndoCommand —— 撤销/重做命令的抽象基类
//   · 每个用户操作封装为一个 Command 对象
//   · execute() 执行操作，undo() 撤销操作
// ============================================================
class UndoCommand {
public:
    virtual ~UndoCommand() = default;

    virtual void execute() = 0;
    virtual void undo()    = 0;
    virtual QString description() const = 0;
};

using UndoCommandPtr = std::unique_ptr<UndoCommand>;
```

### 5.5 操作栈设计

```cpp
// src/core/undostack.h
#pragma once
#include <QObject>
#include <QStack>
#include "undocommand.h"

// ============================================================
// UndoStack —— 撤销/重做操作栈
//   · 维护两个栈：undo_stack_ 和 redo_stack_
//   · 执行新操作时清空 redo_stack_
//   · 栈深度限制为 MAX_STACK_SIZE
// ============================================================
class UndoStack : public QObject {
    Q_OBJECT
public:
    static constexpr int MAX_STACK_SIZE = 20;

    explicit UndoStack(QObject* parent = nullptr);

    // 执行并入栈（同时清空 redo 栈）
    void push(UndoCommandPtr cmd);

    void undo();
    void redo();

    bool canUndo() const { return !undo_stack_.isEmpty(); }
    bool canRedo() const { return !redo_stack_.isEmpty(); }

signals:
    // 栈状态变化时通知 UI 更新按钮状态
    void stackChanged();

private:
    QStack<UndoCommandPtr> undo_stack_;
    QStack<UndoCommandPtr> redo_stack_;
};
```

### 5.6 具体命令实现示例

```cpp
// src/core/commands/editnotecommand.h
#pragma once
#include "../undocommand.h"
#include "../../common/notedata.h"

class NoteListModel;

class EditNoteCommand : public UndoCommand {
public:
    EditNoteCommand(NoteListModel* model, int row,
                    const NoteData& old_data, const NoteData& new_data);

    void execute() override;
    void undo()    override;
    QString description() const override { return "编辑便签"; }

private:
    NoteListModel* model_;
    int            row_;
    NoteData       old_data_;  // 撤销时恢复
    NoteData       new_data_;  // 执行时应用
};
```

```cpp
// src/core/commands/editnotecommand.cpp
void EditNoteCommand::execute() {
    model_->updateNote(row_, new_data_);
}

void EditNoteCommand::undo() {
    model_->updateNote(row_, old_data_);
}
```

### 5.7 与 NoteController 集成

```cpp
// notecontroller.h 新增
#include "../core/undostack.h"

class NoteController : public QObject {
    // ...
private:
    UndoStack* undo_stack_ = nullptr;  // 新增
};

// notecontroller.cpp 修改 onNoteEdited
void NoteController::onNoteEdited(const QModelIndex& proxyIndex) {
    // ... 获取 old_data 和 new_data ...

    // 不直接修改，而是创建命令并推入栈
    auto cmd = std::make_unique<EditNoteCommand>(model_, source_row, old_data, new_data);
    undo_stack_->push(std::move(cmd));
    // push 内部会调用 cmd->execute()
}
```

### 5.8 工具栏按钮状态联动

```cpp
// mainwindow.cpp 中
connect(undo_stack_, &UndoStack::stackChanged, this, [this]() {
    undo_btn_->setEnabled(undo_stack_->canUndo());
    redo_btn_->setEnabled(undo_stack_->canRedo());
});

// 快捷键
new QShortcut(QKeySequence::Undo, this, [this]() { undo_stack_->undo(); });
new QShortcut(QKeySequence::Redo, this, [this]() { undo_stack_->redo(); });
```

### 5.9 学习重点

- **Command 设计模式**：将操作封装为对象，实现撤销/重做
- `std::unique_ptr` 管理命令对象的生命周期
- `QStack` 的使用
- 信号/槽驱动 UI 状态更新（按钮禁用/启用）

---

## 六、功能 3.13：系统托盘与全局快捷键

### 6.1 需求分析

- 关闭按钮 → 隐藏到托盘（不退出）
- 双击托盘图标 → 恢复窗口
- 托盘右键菜单：显示主窗口 / 快速新建 / 退出
- `Ctrl+Alt+N` 全局快捷键唤起新建窗口

### 6.2 系统托盘实现

```cpp
// mainwindow.h 新增
#include <QSystemTrayIcon>
#include <QMenu>

class MainWindow : public QWidget {
    // ...
private:
    void initTray();

    QSystemTrayIcon* tray_icon_  = nullptr;
    QMenu*           tray_menu_  = nullptr;
};
```

```cpp
// mainwindow.cpp
void MainWindow::initTray() {
    tray_icon_ = new QSystemTrayIcon(QIcon(":/icons/app.png"), this);

    tray_menu_ = new QMenu(this);
    tray_menu_->addAction("显示主窗口", this, &MainWindow::show);
    tray_menu_->addAction("快速新建",   this, [this]() {
        show();
        note_controller_->onNewNoteRequested();
    });
    tray_menu_->addSeparator();
    tray_menu_->addAction("退出", qApp, &QApplication::quit);

    tray_icon_->setContextMenu(tray_menu_);
    tray_icon_->show();

    // 双击恢复窗口
    connect(tray_icon_, &QSystemTrayIcon::activated,
            this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            show();
            raise();
            activateWindow();
        }
    });
}

// 重写 closeEvent：关闭时隐藏到托盘
void MainWindow::closeEvent(QCloseEvent* event) {
    if (tray_icon_ && tray_icon_->isVisible()) {
        hide();
        event->ignore();  // 阻止真正关闭
        tray_icon_->showMessage("便签管理器", "程序已最小化到托盘",
                                 QSystemTrayIcon::Information, 2000);
    } else {
        saveWindowState();
        event->accept();
    }
}
```

### 6.3 全局快捷键

> ⚠️ Qt 本身没有跨平台的全局快捷键 API，需要平台特定实现。

**Windows 平台方案：使用 `RegisterHotKey` Win32 API**

```cpp
// src/utils/globalhotkey.h
#pragma once
#include <QObject>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

class GlobalHotKey : public QObject {
    Q_OBJECT
public:
    explicit GlobalHotKey(QObject* parent = nullptr);
    ~GlobalHotKey() override;

    // 注册热键（Windows: MOD_CONTROL | MOD_ALT, 'N'）
    bool registerHotKey(int id, Qt::KeyboardModifiers modifiers, Qt::Key key);
    void unregisterHotKey(int id);

signals:
    void hotKeyPressed(int id);

protected:
    // 拦截 Windows 消息
    bool nativeEventFilter(const QByteArray& eventType,
                           void* message, qintptr* result);
};
```

```cpp
// Windows 实现
bool GlobalHotKey::registerHotKey(int id, Qt::KeyboardModifiers mods, Qt::Key key) {
    UINT win_mods = 0;
    if (mods & Qt::ControlModifier) win_mods |= MOD_CONTROL;
    if (mods & Qt::AltModifier)     win_mods |= MOD_ALT;
    if (mods & Qt::ShiftModifier)   win_mods |= MOD_SHIFT;

    return ::RegisterHotKey(nullptr, id, win_mods, key);
}

// 在 MainWindow 中注册
hotkey_->registerHotKey(1, Qt::ControlModifier | Qt::AltModifier, Qt::Key_N);
connect(hotkey_, &GlobalHotKey::hotKeyPressed, this, [this](int id) {
    if (id == 1) {
        show();
        raise();
        note_controller_->onNewNoteRequested();
    }
});
```

### 6.4 学习重点

- `QSystemTrayIcon` 的使用
- `closeEvent` 中 `event->ignore()` 阻止关闭
- Windows `RegisterHotKey` / `UnregisterHotKey` API
- `QAbstractNativeEventFilter` 拦截系统消息

---

## 七、开发顺序建议

```
Day 8：
  上午：IconButton（1h）→ Toast（1.5h）
  下午：系统托盘（1.5h）→ 全局快捷键（1h）

Day 9：
  上午：卡片拖拽排序（3h）
  下午：撤销/重做操作栈（3h）
  晚上：联调测试 + 修复 Bug（1h）
```

---

## 八、CMakeLists.txt 需要新增的文件

```cmake
# 在 target_sources 中新增：
src/ui/components/iconbutton.h
src/ui/components/iconbutton.cpp
src/ui/components/toastwidget.h
src/ui/components/toastwidget.cpp
src/ui/components/toastmanager.h
src/ui/components/toastmanager.cpp
src/core/undocommand.h
src/core/undostack.h
src/core/undostack.cpp
src/core/commands/addnotecommand.h
src/core/commands/addnotecommand.cpp
src/core/commands/deletenotecommand.h
src/core/commands/deletenotecommand.cpp
src/core/commands/editnotecommand.h
src/core/commands/editnotecommand.cpp
src/core/commands/pinnotecommand.h
src/core/commands/pinnotecommand.cpp
src/core/commands/changecategorycommand.h
src/core/commands/changecategorycommand.cpp
src/core/commands/movenotecommand.h
src/core/commands/movenotecommand.cpp
src/utils/globalhotkey.h
src/utils/globalhotkey.cpp
```

---

## 九、验收 Checklist

- [ ] IconButton 三态视觉反馈正常（normal/hover/pressed）
- [ ] IconButton 禁用态图标灰显
- [ ] Toast 底部弹出，1.5 秒后渐隐消失
- [ ] 多条 Toast 排队展示，不重叠
- [ ] 卡片可拖拽排序，拖拽时有半透明跟随效果
- [ ] 目标位置显示插入线
- [ ] 拖拽完成后 `sortOrder` 更新并持久化
- [ ] `Ctrl+Z` 撤销、`Ctrl+Y` 重做
- [ ] 支持撤销：新建、删除、编辑、置顶、分类变更、排序变更
- [ ] 操作栈深度 20 步
- [ ] 工具栏撤销/重做按钮状态随栈变化
- [ ] 关闭按钮隐藏到托盘
- [ ] 双击托盘图标恢复窗口
- [ ] 托盘右键菜单功能正常
- [ ] `Ctrl+Alt+N` 全局快捷键唤起新建窗口
