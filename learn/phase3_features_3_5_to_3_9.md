# 功能实现文档：3.5 ~ 3.9 完善记录

> 本文档记录对 **3.5 删除便签 / 3.6 分类管理 / 3.7 搜索 / 3.8 右键菜单 / 3.9 颜色选择器** 的完善改动，以及每处改动背后的 Qt 知识原理。

---

## 一、改动总览

| 功能点 | 改动文件 | 改动内容 |
|--------|---------|---------|
| 3.5 删除便签 | `notelistview.h/.cpp` | 添加删除二次确认、批量删除（Delete 键）|
| 3.5 批量删除 | `notelistview.h/.cpp` | 改为 `ExtendedSelection` 多选模式 |
| 3.5 批量删除 | `notemanager.h/.cpp` | 新增 `removeNotes(QStringList)` 接口 |
| 3.6 分类计数 | `sidebar.cpp` | 分类名称后显示该分类下的便签数量 |
| 3.7 搜索防抖 | `tilebar.h/.cpp` | 改为 `textChanged` 实时触发 + 300ms 防抖 |
| 3.8 右键菜单 | `notelistview.cpp` | 新增置顶、更改分类子菜单、更改颜色子菜单、独立窗口 |
| 3.8 右键菜单 | `notemanager.h/.cpp` | 新增 `togglePin`、`updateNoteCategory`、`updateNoteColor` |
| 3.8 右键菜单 | `mainwindow.h/.cpp` | 接入所有新信号槽 |

---

## 二、3.5 删除便签

### 2.1 删除二次确认

**改动前**：右键点击"删除"后直接删除，没有任何确认。  
**改动后**：弹出 `QMessageBox::question` 确认框，用户点"是"才真正删除。

```cpp
// notelistview.cpp — contextMenuEvent 中
QMessageBox::StandardButton btn = QMessageBox::question(
    this, "确认删除",
    QString("确定要删除便签「%1」吗？").arg(note.title),
    QMessageBox::Yes | QMessageBox::No,
    QMessageBox::No   // 默认选中"否"，防止误操作
);
if (btn == QMessageBox::Yes) {
    emit deleteNoteRequested(index);
}
```

**知识点：`QMessageBox::question`**

`QMessageBox` 有几个静态便捷方法：

| 方法 | 图标 | 用途 |
|------|------|------|
| `question()` | ❓ | 询问用户是/否 |
| `warning()` | ⚠️ | 警告提示 |
| `information()` | ℹ️ | 普通信息 |
| `critical()` | ❌ | 严重错误 |

第 4 个参数是**按钮组合**（用 `|` 组合多个），第 5 个参数是**默认按钮**（回车键触发的那个）。  
返回值是用户点击的按钮枚举值，与之比较即可判断用户的选择。

---

### 2.2 批量删除（Ctrl+Click 多选 + Delete 键）

**改动 1：开启多选模式**

```cpp
// notelistview.cpp — 构造函数中
// 改为扩展多选：Ctrl+Click 多选，Shift+Click 范围选
setSelectionMode(QAbstractItemView::ExtendedSelection);
```

`QAbstractItemView` 提供 5 种选择模式：

| 枚举值 | 说明 |
|--------|------|
| `NoSelection` | 不允许选择 |
| `SingleSelection` | 只能选一项（原来的模式）|
| `ContiguousSelection` | 只能选连续的多项 |
| `ExtendedSelection` | Ctrl 多选、Shift 范围选（最常用）|
| `MultiSelection` | 点击即切换选中，不需要 Ctrl |

**改动 2：重写 `keyPressEvent` 响应 Delete 键**

```cpp
void NoteListView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        QModelIndexList selected = selectedIndexes();
        // ... 弹出确认框 ...
        emit deleteMultipleRequested(selected);
        return;
    }
    QListView::keyPressEvent(event);  // 其他按键交给父类处理
}
```

**知识点：`keyPressEvent` 的重写规范**

- 处理了某个按键后，调用 `return` 阻止事件继续传播
- 没有处理的按键，**必须调用父类的 `keyPressEvent`**，否则其他快捷键（如方向键导航）会失效

**改动 3：`NoteManager` 新增批量删除接口**

```cpp
// notemanager.h
void removeNotes(const QStringList& ids);

// notemanager.cpp
void NoteManager::removeNotes(const QStringList& ids) {
    for (const QString& id : ids) {
        for (auto it = notes_.begin(); it != notes_.end(); ++it) {
            if (it->id == id) {
                notes_.erase(it);
                break;
            }
        }
    }
    save();
    emit dataChanged();  // 只 emit 一次，避免多次刷新 UI
}
```

> **为什么不循环调用 `removeNote`？**  
> 每次调用 `removeNote` 都会触发 `save()` 和 `emit dataChanged()`，导致 UI 刷新 N 次，性能差。  
> 批量删除应该**一次性修改数据，最后统一通知**。

---

## 三、3.6 分类管理 — 分类计数

### 3.1 改动内容

在 `SideBar::refreshCategories()` 中，为每个分类计算便签数量，并显示在按钮文字中：

```cpp
// sidebar.cpp
const QList<NoteData>& allNotes = NoteManager::instance()->notes();

for (const QString& cat : cats) {
    int count = 0;
    if (cat == "全部") {
        count = allNotes.size();
    } else {
        for (const NoteData& note : allNotes) {
            if (note.category == cat) ++count;
        }
    }
    // 有便签时显示数量，没有时只显示分类名
    QString displayName = count > 0
        ? QString("%1  (%2)").arg(cat).arg(count)
        : cat;

    QPushButton* btn = new QPushButton(displayName, row_widget);
    // ...
    // 注意：lambda 捕获的是原始 cat，不是 displayName
    // 这样 categoryChanged 信号传递的是纯分类名，不含数字
    connect(btn, &QPushButton::clicked, this, [this, cat, btn]() {
        emit categoryChanged(cat);  // 传递原始 cat，不是 displayName
    });
}
```

### 3.2 关键设计：显示名与数据名分离

按钮**显示**的是 `"工作  (3)"`，但点击时 `categoryChanged` 信号传递的是 `"工作"`。  
这是因为 `NoteFilterProxyModel::setCategory()` 接收的是纯分类名，用于与 `note.category` 比较。  
如果传递了 `"工作  (3)"`，过滤就会失效。

> **原则**：UI 显示和数据逻辑要分离，不要把格式化后的字符串当作数据键值使用。

---

## 四、3.7 搜索 — 实时过滤 + 300ms 防抖

### 4.1 改动前的问题

```cpp
// 改动前：只有按回车才触发搜索
connect(search_edit_, &QLineEdit::returnPressed,
    this, [this]() {
        emit searchTextChanged(search_edit_->text());
    });
```

用户每次搜索都要按回车，体验差。

### 4.2 改动后：输入即搜 + 防抖

```cpp
// tilebar.cpp — 构造函数中初始化防抖定时器
search_debounce_timer_ = new QTimer(this);
search_debounce_timer_->setSingleShot(true);  // 单次触发，不循环
search_debounce_timer_->setInterval(300);     // 300ms 延迟

// 每次输入都重置定时器
connect(search_edit_, &QLineEdit::textChanged,
    this, [this](const QString&) {
        search_debounce_timer_->start();  // start() 会重置已有的计时
    });

// 定时器超时后才真正发出搜索信号
connect(search_debounce_timer_, &QTimer::timeout,
    this, [this]() {
        emit searchTextChanged(search_edit_->text());
    });

// 按回车立即搜索（跳过防抖）
connect(search_edit_, &QLineEdit::returnPressed,
    this, [this]() {
        search_debounce_timer_->stop();
        emit searchTextChanged(search_edit_->text());
    });
```

### 4.3 防抖原理图

```
用户输入 "Q"    → 定时器重置，开始计时 300ms
用户输入 "Qt"   → 定时器重置，重新计时 300ms
用户输入 "Qt5"  → 定时器重置，重新计时 300ms
300ms 内无输入  → 定时器超时，emit searchTextChanged("Qt5")
                  → NoteFilterProxyModel::setKeyword("Qt5")
                  → invalidateFilter() → View 刷新
```

### 4.4 知识点：`QTimer::setSingleShot` vs 普通定时器

| 属性 | 普通定时器 | `setSingleShot(true)` |
|------|-----------|----------------------|
| 触发次数 | 每隔 interval 触发一次，循环 | 只触发一次 |
| 用途 | 周期性任务（如动画帧、心跳） | 延迟执行、防抖 |
| 重置方式 | 调用 `start()` 重新开始计时 | 同上 |

**`start()` 的重置特性**：如果定时器已经在运行，再次调用 `start()` 会**重置计时**（从头开始），这正是防抖的核心机制。

---

## 五、3.8 右键菜单 — 完整实现

### 5.1 菜单结构

```
📝  编辑
📌  置顶 / 取消置顶
─────────────────
🏷️  更改分类  ▶  工作 ✓
                  生活
                  学习
                  自定义分类...
─────────────────
🎨  更改颜色  ▶  🟡 黄色 ✓
                  🩵 青色
                  ⚪ 灰色
                  🔴 红色
                  🟣 紫色
─────────────────
🪟  独立窗口打开
─────────────────
🗑️  删除
```

### 5.2 子菜单的创建方式

```cpp
// 创建子菜单：addMenu 返回一个 QMenu*
QMenu* categoryMenu = menu.addMenu("🏷️  更改分类");

// 向子菜单添加选项
for (const QString& cat : cats) {
    QAction* catAction = categoryMenu->addAction(cat);
    catAction->setCheckable(true);   // 允许打勾
    catAction->setChecked(cat == note.category);  // 当前分类打勾
}
```

**知识点：`QAction::setCheckable`**

- `setCheckable(true)`：让菜单项变成可勾选状态（前面显示 ✓）
- `setChecked(bool)`：设置当前是否勾选
- 常用于"当前选中项"的视觉反馈

### 5.3 判断用户点击了哪个子菜单项

```cpp
QAction* result = menu.exec(event->globalPos());
if (!result) return;  // 用户按 Esc 取消

// 判断是否点击了分类子菜单中的某项
if (categoryMenu->actions().contains(result)) {
    emit changeCategoryRequested(index, result->text());
}
// 判断是否点击了颜色子菜单中的某项
else if (colorMenu->actions().contains(result)) {
    // 从颜色选项列表中找到对应的 hex 值
    for (const auto& [name, hex] : colorOptions) {
        if (name == result->text()) {
            emit changeColorRequested(index, hex);
            break;
        }
    }
}
```

> **为什么颜色要单独查找 hex？**  
> 菜单项显示的是 `"🟡  黄色"`，但我们需要传递 `"#FFEAA7"`。  
> 所以用 `result->text()` 匹配显示名，再从 `colorOptions` 列表中取出对应的 hex 值。

### 5.4 置顶功能

**`NoteManager` 新增接口：**

```cpp
void NoteManager::togglePin(const QString& id) {
    for (auto& n : notes_) {
        if (n.id == id) {
            n.pinned = !n.pinned;           // 取反
            n.modifiedAt = QDateTime::currentDateTime();
            break;
        }
    }
    save();
    emit dataChanged();
}
```

**菜单文字动态变化：**

```cpp
// 根据当前置顶状态显示不同文字
QAction* pinAction = menu.addAction(note.pinned ? "📌  取消置顶" : "📌  置顶");
```

这是一个常见的 UI 技巧：**根据当前状态动态生成菜单文字**，让用户清楚当前操作的效果。

### 5.5 独立窗口打开

```cpp
void MainWindow::onNotePreviewRequested(const QModelIndex& proxyIndex) {
    QModelIndex sourceIndex = note_proxy_model_->mapToSource(proxyIndex);
    NoteData note = note_model_->data(sourceIndex, NoteDataRole).value<NoteData>();

    // parent = nullptr：不依附于主窗口，成为独立顶层窗口
    NotePreviewDialog* dialog = new NotePreviewDialog(note, nullptr);
    dialog->setAttribute(Qt::WA_DeleteOnClose);  // 关闭时自动 delete，防止内存泄漏
    dialog->setWindowFlag(Qt::Window);           // 确保作为独立窗口显示
    dialog->show();  // 非阻塞，不用 exec()
}
```

**知识点：`exec()` vs `show()` 的区别**

| 方法 | 阻塞 | 用途 |
|------|------|------|
| `exec()` | ✅ 阻塞，等待关闭 | 模态对话框（必须先处理再继续）|
| `show()` | ❌ 非阻塞，立即返回 | 独立窗口（可以同时打开多个）|

**知识点：`Qt::WA_DeleteOnClose`**

```cpp
dialog->setAttribute(Qt::WA_DeleteOnClose);
```

设置此属性后，窗口关闭时会自动调用 `delete this`，释放内存。  
如果不设置，`new` 出来的对象会一直存在内存中，造成**内存泄漏**。

**知识点：`parent = nullptr` 的含义**

- `parent = this`（主窗口）：对话框是主窗口的子窗口，主窗口最小化时它也消失
- `parent = nullptr`：对话框是独立的顶层窗口，与主窗口互不影响

---

## 六、信号槽扩展 — MainWindow 接入新功能

### 6.1 新增的信号（NoteListView → MainWindow）

```cpp
// notelistview.h 中新增的信号
signals:
    void pinToggleRequested(const QModelIndex& index);
    void changeCategoryRequested(const QModelIndex& index, const QString& category);
    void changeColorRequested(const QModelIndex& index, const QString& color);
    void openPreviewRequested(const QModelIndex& index);
    void deleteMultipleRequested(const QModelIndexList& indexes);
```

### 6.2 MainWindow 中的槽函数模式

所有槽函数都遵循同一个模式：

```
1. 将代理索引转换为源模型索引（mapToSource）
2. 从源模型获取 NoteData
3. 调用 NoteManager 的对应接口
4. 刷新 Model（note_model_->refresh()）
```

```cpp
void MainWindow::onNotePinToggled(const QModelIndex& proxyIndex) {
    QModelIndex sourceIndex = note_proxy_model_->mapToSource(proxyIndex);  // 步骤 1
    NoteData note = note_model_->data(sourceIndex, NoteDataRole).value<NoteData>();  // 步骤 2
    NoteManager::instance()->togglePin(note.id);  // 步骤 3
    note_model_->refresh();  // 步骤 4
}
```

> **为什么需要 `mapToSource`？**  
> `NoteListView` 绑定的是 `NoteFilterProxyModel`（代理模型），所以右键菜单拿到的 `QModelIndex` 是**代理索引**，它的行号是过滤后的行号，不是源模型中的真实行号。  
> 必须用 `mapToSource()` 转换，才能正确访问源模型数据。

---

## 七、数据流总览（更新后）

```
用户右键菜单操作
        │
        ▼
NoteListView::contextMenuEvent()
        │ emit xxxRequested(index)
        ▼
MainWindow::onXxx(proxyIndex)
        │ mapToSource → 获取 NoteData
        ▼
NoteManager::togglePin / updateNoteCategory / updateNoteColor / removeNotes
        │ save() + emit dataChanged()
        ▼
NoteListModel::refresh()
        │ beginResetModel / endResetModel
        ▼
NoteFilterProxyModel（自动重新过滤）
        │
        ▼
NoteListView 重绘 + SideBar::refreshCategories()（分类计数更新）
```

---

## 八、常见问题

### Q1：为什么 `categoryMenu->actions().contains(result)` 能判断子菜单的点击？

`QMenu::exec()` 返回的是用户最终点击的那个 `QAction*`，无论它在哪个层级的子菜单中。  
`categoryMenu->actions()` 返回该子菜单中所有 `QAction` 的列表。  
用 `contains()` 检查返回的 action 是否在子菜单中，就能判断用户点击的是哪个子菜单项。

### Q2：防抖定时器为什么用 `start()` 而不是 `restart()`？

`QTimer` 没有 `restart()` 方法。`start()` 本身就有重置效果：  
如果定时器已经在运行，再次调用 `start()` 会**停止当前计时并重新开始**，这正是防抖需要的行为。

### Q3：批量删除时，为什么要先收集所有 id，再统一删除？

因为删除操作会修改 `notes_` 列表，如果边遍历边删除，迭代器会失效，导致崩溃或跳过元素。  
正确做法是：先收集所有要删除的 id，再统一删除。

### Q4：`QList<QPair<QString, QString>>` 中的结构化绑定是什么语法？

```cpp
for (const auto& [name, hex] : colorOptions) { ... }
```

这是 **C++17 的结构化绑定（Structured Bindings）**，可以直接将 `QPair` 的 `first` 和 `second` 解构为有意义的变量名，比 `item.first` / `item.second` 更易读。  
需要确保编译器开启了 C++17 支持（CMakeLists.txt 中 `set(CMAKE_CXX_STANDARD 17)`）。
