# StickyNoteManager 第三阶段知识详解

> 本文档面向 C++/Qt 初学者，将三期开发文档中涉及的所有知识点逐一解释清楚。  
> 建议先阅读 `phase3_development.md`，再回来查阅本文档中对应的知识点。

---

## 目录

1. [QSortFilterProxyModel — 代理模型](#一qsortfilterproxymodel--代理模型)
2. [QDialog — 模态对话框](#二qdialog--模态对话框)
3. [QTextEdit 与富文本编辑](#三qtextedit-与富文本编辑)
4. [QTextCursor 与 QTextCharFormat — 富文本格式控制](#四qtextcursor-与-qtextcharformat--富文本格式控制)
5. [QComboBox — 下拉选择框](#五qcombobox--下拉选择框)
6. [QPushButton::setCheckable — 可切换按钮](#六qpushbuttonsetCheckable--可切换按钮)
7. [setMouseTracking — 鼠标追踪](#七setmousetracking--鼠标追踪)
8. [QMenu 与右键上下文菜单](#八qmenu-与右键上下文菜单)
9. [QInputDialog — 快速输入对话框](#九qinputdialog--快速输入对话框)
10. [代理模型索引转换：mapToSource](#十代理模型索引转换maptosource)
11. [QLineEdit::textChanged 信号与实时搜索](#十一qlineedittextchanged-信号与实时搜索)
12. [自定义 Widget 的 paintEvent 与 mousePressEvent](#十二自定义-widget-的-paintevent-与-mousepressevent)
13. [QLayout 布局系统详解](#十三qlayout-布局系统详解)
14. [QString 常用操作](#十四qstring-常用操作)

---

## 一、QSortFilterProxyModel — 代理模型

### 1.1 它是什么？

`QSortFilterProxyModel` 是 Qt 提供的一个**中间层模型**，它不存储数据，而是包装在另一个模型（源模型）外面，对源模型的数据进行**过滤**和**排序**后再提供给 View。

```
View（NoteListView）
    ↑ 看到的是过滤后的数据
NoteFilterProxyModel（代理模型）
    ↑ 从源模型取数据，过滤后转发给 View
NoteListModel（源模型）
    ↑ 存储全部便签数据
NoteManager（数据源）
```

### 1.2 为什么要用代理模型？

**不用代理模型的做法**：在 `NoteListModel` 的 `rowCount()` 和 `data()` 里自己写过滤逻辑。

**问题**：这样会把"数据管理"和"过滤逻辑"混在一起，职责不清晰，而且每次过滤条件变化都要手动刷新整个模型。

**用代理模型的好处**：
- 源模型保持干净，只负责数据
- 代理模型专门负责过滤/排序
- 可以给同一个源模型套多个不同的代理（比如一个按分类过滤，一个按时间排序）

### 1.3 核心方法：`filterAcceptsRow()`

这是你**必须重写**的函数，Qt 会对源模型的每一行调用它：

```cpp
bool NoteFilterProxyModel::filterAcceptsRow(int sourceRow,
                                             const QModelIndex& sourceParent) const {
    // sourceRow：源模型中的行号（0, 1, 2, ...）
    // sourceParent：父节点索引（列表模型中始终是无效索引）

    // 通过源模型的 index() 方法获取该行的索引
    QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);

    // 通过索引获取数据
    QString title = idx.data(NoteTitleRole).toString();

    // 返回 true：该行显示；返回 false：该行隐藏
    return title.contains(keyword_);
}
```

### 1.4 `invalidateFilter()` — 触发重新过滤

当过滤条件改变时（比如用户输入了新的搜索词），需要通知代理模型重新对所有行调用 `filterAcceptsRow()`：

```cpp
void NoteFilterProxyModel::setKeyword(const QString& keyword) {
    keyword_ = keyword;
    invalidateFilter(); // 触发重新过滤，View 自动刷新
}
```

**类比**：就像 Excel 的筛选功能，你改变筛选条件后，表格会自动重新显示符合条件的行。

### 1.5 设置源模型

```cpp
NoteListModel* sourceModel = new NoteListModel(this);
NoteFilterProxyModel* proxy = new NoteFilterProxyModel(this);

proxy->setSourceModel(sourceModel); // 代理包装源模型

// View 绑定代理，不是源模型
noteListView->setModel(proxy);
```

---

## 二、QDialog — 模态对话框

### 2.1 什么是模态对话框？

**模态（Modal）**：打开对话框后，用户**必须先关闭它**，才能操作主窗口。就像你在 Word 里点"另存为"，弹出的保存窗口就是模态的。

**非模态（Modeless）**：对话框和主窗口可以同时操作，比如"查找/替换"窗口。

### 2.2 QDialog 的基本用法

```cpp
// 方式一：exec()——阻塞式，等待用户关闭
NoteEditDialog dlg(note, this);
int result = dlg.exec(); // 阻塞在这里，直到对话框关闭

if (result == QDialog::Accepted) {
    // 用户点了"保存"（调用了 accept()）
    NoteData data = dlg.result();
} else {
    // 用户点了"取消"或关闭了窗口（调用了 reject()）
}

// 方式二：show()——非阻塞式，立即返回
dlg.show(); // 立即返回，对话框在后台运行
```

### 2.3 accept() 和 reject()

在对话框内部，通过调用这两个函数来关闭对话框并设置返回值：

```cpp
void NoteEditDialog::onSaveClicked() {
    // 验证数据...
    accept(); // 关闭对话框，exec() 返回 QDialog::Accepted（值为 1）
}

void NoteEditDialog::onCancelClicked() {
    reject(); // 关闭对话框，exec() 返回 QDialog::Rejected（值为 0）
}
```

### 2.4 为什么用 exec() 而不是 show()？

在便签编辑场景中，我们需要：
1. 打开对话框
2. **等待**用户编辑完成
3. 获取编辑结果
4. 保存到 NoteManager

如果用 `show()`，代码会立即继续执行，此时用户还没编辑完，`dlg.result()` 拿到的是空数据。

`exec()` 会阻塞，确保用户关闭对话框后才继续执行后续代码。

---

## 三、QTextEdit 与富文本编辑

### 3.1 QTextEdit vs QLineEdit

| | `QLineEdit` | `QTextEdit` |
|--|-------------|-------------|
| 行数 | 单行 | 多行 |
| 富文本 | ❌ 不支持 | ✅ 支持 HTML |
| 用途 | 标题、搜索框 | 便签内容 |

### 3.2 富文本读写

```cpp
QTextEdit* editor = new QTextEdit(this);

// 写入富文本（HTML 格式）
editor->setHtml("<b>加粗文字</b> 普通文字 <i>斜体</i>");

// 读取富文本（返回 HTML 字符串）
QString html = editor->toHtml();
// 结果类似：<html><body><p><b>加粗文字</b> 普通文字 <i>斜体</i></p></body></html>

// 写入纯文本（会清除所有格式）
editor->setPlainText("普通文字");

// 读取纯文本（去除所有 HTML 标签）
QString plain = editor->toPlainText();
```

### 3.3 占位符文字

```cpp
editor->setPlaceholderText("请输入便签内容..."); // 未输入时显示的提示文字
```

### 3.4 为什么便签内容要存 HTML？

因为用户可能设置了**加粗、斜体、颜色**等格式，这些格式信息只有 HTML 才能保存。如果存纯文本，格式就丢失了。

---

## 四、QTextCursor 与 QTextCharFormat — 富文本格式控制

### 4.1 QTextCursor 是什么？

`QTextCursor` 代表文本编辑器中的**光标**，它不仅表示光标位置，还可以表示**选中的文字范围**。

```
"Hello World"
      ↑
   光标在这里（位置 5）

"Hello World"
 ↑─────↑
 选中了 "Hello"（从位置 0 到位置 5）
```

### 4.2 获取当前光标

```cpp
QTextCursor cursor = content_edit_->textCursor();
// 如果用户选中了文字，cursor 就包含选中范围
// 如果没有选中，cursor 只是一个位置点
```

### 4.3 QTextCharFormat — 字符格式

`QTextCharFormat` 描述文字的格式（字体、颜色、加粗等）：

```cpp
QTextCharFormat fmt;

// 设置加粗
fmt.setFontWeight(QFont::Bold);    // 加粗
fmt.setFontWeight(QFont::Normal);  // 取消加粗

// 设置斜体
fmt.setFontItalic(true);

// 设置文字颜色
fmt.setForeground(QColor("#FF0000")); // 红色

// 设置字体大小
fmt.setFontPointSize(14);
```

### 4.4 应用格式到选中文字

```cpp
// mergeCharFormat：将格式合并到选中文字（只改变指定的属性，其他属性不变）
content_edit_->textCursor().mergeCharFormat(fmt);

// setCharFormat：完全替换选中文字的格式（会覆盖所有属性）
content_edit_->textCursor().setCharFormat(fmt);
```

**类比**：`mergeCharFormat` 就像 Word 里只点"加粗"按钮，只改变粗细，颜色不变；`setCharFormat` 就像把所有格式都重置。

### 4.5 完整的加粗实现

```cpp
void NoteEditDialog::onBoldClicked() {
    QTextCharFormat fmt;
    // bold_btn_->isChecked() 返回按钮当前是否处于"按下"状态
    fmt.setFontWeight(bold_btn_->isChecked() ? QFont::Bold : QFont::Normal);
    content_edit_->textCursor().mergeCharFormat(fmt);
    content_edit_->setFocus(); // 操作完后把焦点还给编辑器
}
```

---

## 五、QComboBox — 下拉选择框

### 5.1 基本用法

```cpp
QComboBox* combo = new QComboBox(this);

// 添加选项
combo->addItem("未分类");
combo->addItem("工作");
combo->addItem("生活");

// 批量添加
QStringList categories = {"未分类", "工作", "生活"};
combo->addItems(categories);

// 获取当前选中的文字
QString selected = combo->currentText(); // "工作"

// 获取当前选中的索引
int idx = combo->currentIndex(); // 1

// 设置当前选中项（按文字查找）
int pos = combo->findText("生活");
if (pos >= 0) combo->setCurrentIndex(pos);
```

### 5.2 在编辑对话框中的用途

便签编辑时，分类选择器用 `QComboBox` 实现：
- 从 `NoteManager::instance()->categories()` 获取所有分类
- 用 `addItems()` 填充下拉列表
- 编辑已有便签时，用 `findText()` 定位到当前分类

---

## 六、QPushButton::setCheckable — 可切换按钮

### 6.1 什么是 Checkable 按钮？

普通按钮：点击 → 触发事件 → 恢复原状（不保持状态）

Checkable 按钮：点击 → 切换"按下/弹起"状态（保持状态，像开关）

```cpp
QPushButton* boldBtn = new QPushButton("B", this);
boldBtn->setCheckable(true); // 设置为可切换

// 判断当前状态
bool isBold = boldBtn->isChecked(); // true = 按下（加粗），false = 弹起（不加粗）

// 手动设置状态
boldBtn->setChecked(true);  // 设为按下状态
boldBtn->setChecked(false); // 设为弹起状态
```

### 6.2 在富文本工具栏中的用途

加粗按钮和斜体按钮都是 Checkable 的：
- 用户点击"B"按钮 → 按钮变为"按下"状态 → 选中文字变为加粗
- 再次点击"B"按钮 → 按钮变为"弹起"状态 → 选中文字取消加粗

---

## 七、setMouseTracking — 鼠标追踪

### 7.1 默认行为

默认情况下，Qt 的 Widget **只有在鼠标按键按下时**才会收到 `mouseMoveEvent`。

这意味着：如果你想实现"鼠标悬停在颜色块上时高亮"的效果，默认是做不到的——因为用户只是移动鼠标，没有按下按键。

### 7.2 开启鼠标追踪

```cpp
// 在构造函数中调用
setMouseTracking(true);
// 开启后，即使没有按下鼠标键，只要鼠标在 Widget 上移动，就会触发 mouseMoveEvent
```

### 7.3 在 ColorSelector 中的应用

```cpp
ColorSelector::ColorSelector(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true); // 开启追踪，才能实现 hover 高亮效果
}

void ColorSelector::mouseMoveEvent(QMouseEvent* event) {
    int idx = hitTest(event->position().toPoint());
    if (idx != hoveredIndex_) {
        hoveredIndex_ = idx; // 更新悬停索引
        update();            // 触发重绘，显示高亮效果
    }
}
```

---

## 八、QMenu 与右键上下文菜单

### 8.1 什么是上下文菜单？

右键点击某个控件时弹出的菜单，叫做**上下文菜单（Context Menu）**。

### 8.2 实现步骤

**Step 1**：开启右键菜单策略

```cpp
// 在 NoteListView 的构造函数中
setContextMenuPolicy(Qt::CustomContextMenu);
// 告诉 Qt：我要自己处理右键菜单，不用默认的
```

**Step 2**：连接信号

```cpp
connect(note_list_view_, &QListView::customContextMenuRequested,
        this, &MainWindow::onNoteContextMenu);
// customContextMenuRequested 信号在用户右键点击时发出，携带鼠标位置
```

**Step 3**：创建并显示菜单

```cpp
void MainWindow::onNoteContextMenu(const QPoint& pos) {
    // pos 是相对于 View 的坐标
    QModelIndex index = note_list_view_->indexAt(pos);
    if (!index.isValid()) return; // 没有点到任何 item，不显示菜单

    QMenu menu(this);
    QAction* deleteAction = menu.addAction("🗑 删除便签");
    QAction* pinAction    = menu.addAction("📌 置顶");

    // exec() 在鼠标位置显示菜单，阻塞直到用户选择或关闭
    // mapToGlobal 将 View 内的坐标转换为屏幕坐标
    QAction* chosen = menu.exec(note_list_view_->viewport()->mapToGlobal(pos));

    if (chosen == deleteAction) {
        // 处理删除
    } else if (chosen == pinAction) {
        // 处理置顶
    }
    // chosen == nullptr 表示用户关闭了菜单，不做任何操作
}
```

### 8.3 `viewport()` 是什么？

`QListView` 内部有一个 `viewport()` 子控件，实际的列表内容是绘制在 `viewport()` 上的，而不是直接绘制在 `QListView` 上。

所以坐标转换时要用 `viewport()->mapToGlobal(pos)`，而不是 `mapToGlobal(pos)`。

---

## 九、QInputDialog — 快速输入对话框

### 9.1 它是什么？

`QInputDialog` 是 Qt 提供的**内置输入对话框**，不需要自己设计 UI，一行代码就能弹出一个带输入框的对话框。

### 9.2 常用方法

```cpp
#include <QInputDialog>

// 文字输入
bool ok;
QString name = QInputDialog::getText(
    this,           // 父窗口
    "新建分类",      // 对话框标题
    "请输入分类名称：", // 提示文字
    QLineEdit::Normal, // 输入模式（Normal/Password）
    "",             // 默认值
    &ok             // 用户是否点了"确定"
);

if (ok && !name.isEmpty()) {
    // 用户点了确定，且输入了内容
    NoteManager::instance()->addCategory(name);
}

// 整数输入
int value = QInputDialog::getInt(this, "标题", "请输入数字：", 0, 0, 100, 1, &ok);

// 下拉选择
QStringList items = {"选项A", "选项B", "选项C"};
QString item = QInputDialog::getItem(this, "标题", "请选择：", items, 0, false, &ok);
```

### 9.3 为什么用 QInputDialog 而不是自定义对话框？

对于简单的单行输入（如分类名称），`QInputDialog` 已经足够，不需要额外创建一个 `QDialog` 子类，代码更简洁。

---

## 十、代理模型索引转换：mapToSource

### 10.1 为什么需要索引转换？

当 View 绑定的是代理模型时，所有从 View 获取的 `QModelIndex` 都是**代理索引**（在过滤后的列表中的位置）。

**举例**：
- 源模型有 10 条便签（行号 0-9）
- 过滤后只显示 3 条（行号 0-2）
- 用户点击第 2 条（代理索引 row=1）
- 这条便签在源模型中可能是第 5 条（源索引 row=4）

如果直接用代理索引去操作源模型，会操作到错误的数据！

### 10.2 转换方法

```cpp
// 代理索引 → 源模型索引
QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);

// 源模型索引 → 代理索引
QModelIndex proxyIndex = proxyModel->mapFromSource(sourceIndex);
```

### 10.3 什么时候需要转换？

| 操作 | 是否需要转换 |
|------|------------|
| 从 View 获取数据（`index.data(role)`） | ❌ 不需要，代理会自动转发 |
| 获取源模型中的真实行号 | ✅ 需要 `mapToSource` |
| 删除操作（需要源模型行号） | ✅ 需要 `mapToSource` |
| 向源模型写入数据 | ✅ 需要 `mapToSource` |

### 10.4 实际代码示例

```cpp
void MainWindow::onNoteDoubleClicked(const QModelIndex& proxyIndex) {
    // proxyIndex 是代理模型中的索引

    // 方式一：直接从代理索引获取数据（代理会自动转发给源模型）
    NoteData note = proxyIndex.data(NoteDataRole).value<NoteData>(); // ✅ 可以

    // 方式二：转换为源索引后获取数据（更明确）
    QModelIndex sourceIndex = note_proxy_->mapToSource(proxyIndex);
    NoteData note2 = sourceIndex.data(NoteDataRole).value<NoteData>(); // ✅ 也可以

    // 打开编辑对话框
    NoteEditDialog dlg(note, this);
    if (dlg.exec() == QDialog::Accepted) {
        NoteManager::instance()->updateNote(dlg.result());
    }
}
```

---

## 十一、QLineEdit::textChanged 信号与实时搜索

### 11.1 textChanged 信号

`QLineEdit` 每次文字变化时都会发出 `textChanged(const QString& text)` 信号：

```cpp
QLineEdit* searchEdit = new QLineEdit(this);
searchEdit->setPlaceholderText("搜索便签...");

// 每次用户输入/删除字符，都会触发 setKeyword
connect(searchEdit, &QLineEdit::textChanged,
        note_proxy_, &NoteFilterProxyModel::setKeyword);
```

### 11.2 实时搜索的完整流程

```
用户输入 "工作"
    ↓
QLineEdit::textChanged("工作") 信号发出
    ↓
NoteFilterProxyModel::setKeyword("工作") 被调用
    ↓
keyword_ = "工作"
invalidateFilter() 被调用
    ↓
Qt 对源模型每一行调用 filterAcceptsRow()
    ↓
只有标题或内容包含 "工作" 的便签返回 true
    ↓
NoteListView 自动刷新，只显示匹配的便签
```

### 11.3 其他常用信号

```cpp
// 用户按下回车时触发
connect(searchEdit, &QLineEdit::returnPressed, this, &MainWindow::onSearch);

// 文字变化时触发（同 textChanged，但参数不同）
connect(searchEdit, &QLineEdit::textEdited, ...); // 只有用户手动输入才触发，程序设置不触发
```

---

## 十二、自定义 Widget 的 paintEvent 与 mousePressEvent

### 12.1 paintEvent — 自定义绘制

每当 Widget 需要重绘时（窗口显示、大小改变、调用 `update()` 等），Qt 会调用 `paintEvent()`。

```cpp
void ColorSelector::paintEvent(QPaintEvent* event) {
    // 必须在 paintEvent 内部创建 QPainter，并传入 this
    QPainter painter(this);

    // 开启抗锯齿（让圆形边缘更平滑）
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制每个颜色圆形
    for (int i = 0; i < colors_.size(); ++i) {
        QRect rect = colorRect(i);

        painter.setBrush(colors_[i]);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(rect); // 绘制圆形（内切于矩形）
    }
}
```

**重要**：`QPainter` 必须在 `paintEvent` 内部创建，不能在外部创建后传入。

### 12.2 update() — 触发重绘

`update()` 不会立即重绘，而是**标记 Widget 为"需要重绘"**，Qt 会在合适的时机（通常是下一个事件循环）调用 `paintEvent()`。

```cpp
void ColorSelector::mousePressEvent(QMouseEvent* event) {
    int idx = hitTest(event->position().toPoint());
    if (idx >= 0) {
        currentColor_ = colors_[idx].name();
        update(); // 标记需要重绘，Qt 会在合适时机调用 paintEvent()
        emit colorSelected(currentColor_);
    }
}
```

### 12.3 hitTest — 碰撞检测

判断鼠标点击位置是否在某个颜色块上：

```cpp
int ColorSelector::hitTest(const QPoint& pos) const {
    for (int i = 0; i < colors_.size(); ++i) {
        QRect rect = colorRect(i); // 获取第 i 个颜色块的矩形
        if (rect.contains(pos)) { // 判断点是否在矩形内
            return i;
        }
    }
    return -1; // 没有点到任何颜色块
}
```

### 12.4 drawEllipse — 绘制圆形

```cpp
// 绘制内切于矩形的椭圆（当矩形是正方形时，就是圆形）
painter.drawEllipse(QRect(x, y, width, height));

// 或者用圆心+半径的方式
painter.drawEllipse(QPointF(cx, cy), radius, radius);
```

---

## 十三、QLayout 布局系统详解

### 13.1 为什么需要布局？

如果手动设置每个控件的位置（`widget->setGeometry(x, y, w, h)`），当窗口大小改变时，控件不会自动调整位置和大小。

布局管理器（Layout）会**自动**根据窗口大小调整控件的位置和大小。

### 13.2 三种常用布局

#### QVBoxLayout — 垂直布局

控件从上到下排列：

```cpp
QVBoxLayout* layout = new QVBoxLayout(this); // this 是父 Widget
layout->addWidget(titleEdit);    // 第一行
layout->addWidget(contentEdit);  // 第二行
layout->addWidget(saveBtn);      // 第三行
```

#### QHBoxLayout — 水平布局

控件从左到右排列：

```cpp
QHBoxLayout* layout = new QHBoxLayout();
layout->addWidget(cancelBtn);
layout->addWidget(saveBtn);
```

#### 嵌套布局

```cpp
QVBoxLayout* mainLayout = new QVBoxLayout(this);
mainLayout->addWidget(titleEdit);
mainLayout->addWidget(contentEdit);

// 底部按钮行用水平布局
QHBoxLayout* btnLayout = new QHBoxLayout();
btnLayout->addStretch(); // 弹性空间，把按钮推到右边
btnLayout->addWidget(cancelBtn);
btnLayout->addWidget(saveBtn);

mainLayout->addLayout(btnLayout); // 将水平布局嵌入垂直布局
```

### 13.3 addStretch() — 弹性空间

`addStretch()` 会在布局中插入一个**可伸缩的空白区域**，把其他控件推到一边：

```cpp
QHBoxLayout* layout = new QHBoxLayout();
layout->addStretch(); // 左边的弹性空间
layout->addWidget(cancelBtn);
layout->addWidget(saveBtn);
// 结果：[          空白          ] [取消] [保存]
```

### 13.4 setContentsMargins — 内边距

```cpp
layout->setContentsMargins(12, 12, 12, 12); // 左、上、右、下各 12px 的内边距
layout->setSpacing(8); // 控件之间的间距 8px
```

---

## 十四、QString 常用操作

### 14.1 在搜索过滤中常用的方法

```cpp
QString title = "工作计划";
QString keyword = "工作";

// 是否包含子字符串
bool found = title.contains(keyword); // true
bool foundCI = title.contains(keyword, Qt::CaseInsensitive); // 不区分大小写

// 去除首尾空白
QString trimmed = "  hello  ".trimmed(); // "hello"

// 是否为空
bool empty = "".isEmpty();   // true
bool empty2 = "  ".isEmpty(); // false（有空格）
bool empty3 = "  ".trimmed().isEmpty(); // true（去除空格后为空）
```

### 14.2 在颜色处理中常用的方法

```cpp
QColor color("#FFEAA7");
QString hex = color.name(); // "#ffeaa7"（小写）
QString hexUpper = color.name().toUpper(); // "#FFEAA7"（大写）

// 比较颜色
bool same = (color.name() == "#ffeaa7"); // true
```

### 14.3 字符串格式化

```cpp
// 用 arg() 格式化字符串（类似 printf）
QString msg = QString("便签 %1 已保存").arg(noteTitle);
QString msg2 = QString("共 %1 条便签，%2 条置顶").arg(total).arg(pinned);
```

---

## 总结：三期新增知识点一览

| 知识点 | 用在哪里 | 核心要点 |
|--------|---------|---------|
| `QSortFilterProxyModel` | `NoteFilterProxyModel` | 重写 `filterAcceptsRow()`，调用 `invalidateFilter()` 触发刷新 |
| `QDialog` + `exec()` | `NoteEditDialog` | `exec()` 阻塞等待，`accept()`/`reject()` 关闭并返回结果 |
| `QTextEdit` 富文本 | 便签内容编辑 | `setHtml()`/`toHtml()` 读写，`toPlainText()` 用于搜索 |
| `QTextCursor` + `QTextCharFormat` | 加粗/斜体按钮 | `mergeCharFormat()` 应用格式到选中文字 |
| `QComboBox` | 分类选择 | `addItems()`、`currentText()`、`findText()` |
| `setCheckable` | 加粗/斜体按钮 | 按钮保持按下/弹起状态，`isChecked()` 获取状态 |
| `setMouseTracking` | `ColorSelector` hover 效果 | 开启后才能在不按键时收到 `mouseMoveEvent` |
| `QMenu` 右键菜单 | 便签删除 | `setContextMenuPolicy(Qt::CustomContextMenu)` + `customContextMenuRequested` 信号 |
| `QInputDialog` | 新建分类 | 一行代码弹出输入框 |
| `mapToSource()` | 代理模型索引转换 | View 绑定代理时，操作源模型必须先转换索引 |
| `QLineEdit::textChanged` | 实时搜索 | 直接连接到 `setKeyword()`，无需额外代码 |
| `paintEvent` + `update()` | `ColorSelector` 绘制 | `update()` 标记重绘，Qt 自动调用 `paintEvent` |
| `QVBoxLayout` / `QHBoxLayout` | 对话框布局 | 嵌套布局 + `addStretch()` 实现灵活排列 |
