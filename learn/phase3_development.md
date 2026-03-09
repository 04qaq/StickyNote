# StickyNoteManager 第三阶段完整开发文档

---

## 一、阶段概述

### 1.1 三期目标

第三阶段（Day 5-7）的目标是在二期数据层与列表展示的基础上，实现**便签的完整增删改查交互流程**，让用户真正能够创建、编辑、删除便签，并支持分类管理和实时搜索。

| 功能点 | 说明 |
|--------|------|
| 新建/编辑便签弹窗 | 模态对话框，包含标题输入、内容编辑、颜色选择 |
| 富文本编辑器 | 支持加粗、斜体、颜色、列表等格式 |
| 颜色选择器 | 自定义 Widget，绘制圆形颜色块，点击选色 |
| 分类管理 | 新建、删除自定义分类，侧边栏联动刷新 |
| 搜索功能 | 标题栏搜索框，实时过滤便签列表 |
| 删除便签 | 卡片右键菜单或按钮触发删除 |

### 1.2 验收标准

- 点击"新建"按钮弹出编辑对话框，填写标题和内容后保存，列表立即刷新
- 双击便签卡片打开编辑对话框，修改后保存，列表立即更新
- 右键便签卡片弹出上下文菜单，可删除便签
- 搜索框输入关键字，列表实时过滤（标题和内容均参与匹配）
- 侧边栏点击分类，列表只显示该分类的便签
- 侧边栏可新建自定义分类，分类保存到 JSON 文件
- 颜色选择器显示至少 6 种颜色，点击选中后卡片背景色变化

---

## 二、新增文件结构

在二期基础上，三期新增以下文件：

```
StickyNoteManager/
└── src/
    ├── models/
    │   └── notefilterproxy.h/.cpp      # 新增：搜索与分类过滤代理模型
    └── ui/
        ├── noteeditdialog.h/.cpp       # 新增：便签编辑/新建弹窗
        ├── mainwindow.h/.cpp           # 修改：接入搜索框、新建按钮、右键菜单
        ├── sidebar.h/.cpp              # 修改：接入分类新建/删除功能
        └── components/                 # 新增目录
            └── colorselector.h/.cpp    # 新增：颜色选择器组件
```

同时需要更新 `CMakeLists.txt`，将新增的源文件加入编译列表。

---

## 三、类设计与职责

### 3.1 类关系图

```
MainWindow
├── TileBar（含搜索框 QLineEdit）
├── SideBar（分类列表 + 新建分类按钮）
└── NoteListView
    ├── NoteListModel
    │   └── NoteFilterProxyModel（套在 Model 外层，过滤数据）
    └── NoteCardDelegate

NoteEditDialog（独立弹窗）
├── QLineEdit（标题）
├── QTextEdit（富文本内容）
└── ColorSelector（颜色选择器）

ColorSelector（自定义 Widget）
└── 绘制多个圆形颜色块，点击选中
```

### 3.2 各类职责说明

| 类名 | 继承自 | 职责 |
|------|--------|------|
| `NoteFilterProxyModel` | `QSortFilterProxyModel` | 根据关键字和分类过滤便签列表 |
| `NoteEditDialog` | `QDialog` | 新建/编辑便签的模态弹窗 |
| `ColorSelector` | `QWidget` | 颜色选择器，绘制圆形色块，发出颜色选中信号 |

---

## 四、NoteFilterProxyModel — 过滤代理模型

### 4.1 头文件设计

```cpp
// src/models/notefilterproxy.h
#pragma once
#include <QSortFilterProxyModel>

class NoteFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit NoteFilterProxyModel(QObject* parent = nullptr);

    // 设置搜索关键字（空字符串表示不过滤）
    void setKeyword(const QString& keyword);
    // 设置分类过滤（"全部" 表示不过滤）
    void setCategory(const QString& category);

protected:
    // 重写此函数，返回 true 表示该行通过过滤，false 表示隐藏
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex& sourceParent) const override;

private:
    QString keyword_;   // 当前搜索关键字
    QString category_;  // 当前选中分类，"全部" 表示不过滤
};
```

### 4.2 实现要点

```cpp
// src/models/notefilterproxy.cpp
#include "notefilterproxy.h"
#include "notelistmodel.h"

NoteFilterProxyModel::NoteFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
    , category_("全部")
{}

void NoteFilterProxyModel::setKeyword(const QString& keyword) {
    keyword_ = keyword;
    invalidateFilter(); // 通知 View 重新过滤
}

void NoteFilterProxyModel::setCategory(const QString& category) {
    category_ = category;
    invalidateFilter(); // 通知 View 重新过滤
}

bool NoteFilterProxyModel::filterAcceptsRow(int sourceRow,
                                             const QModelIndex& sourceParent) const {
    QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);

    // 分类过滤
    if (category_ != "全部") {
        QString noteCategory = idx.data(NoteCategoryRole).toString();
        if (noteCategory != category_) return false;
    }

    // 关键字过滤（标题 + 内容）
    if (!keyword_.isEmpty()) {
        QString title   = idx.data(NoteTitleRole).toString();
        QString content = idx.data(NoteContentRole).toString();
        bool match = title.contains(keyword_, Qt::CaseInsensitive)
                  || content.contains(keyword_, Qt::CaseInsensitive);
        if (!match) return false;
    }

    return true;
}
```

### 4.3 关键点：`invalidateFilter()`

每次过滤条件改变后，必须调用 `invalidateFilter()`，它会触发 View 重新调用 `filterAcceptsRow()` 对所有行重新判断，从而刷新显示。

---

## 五、ColorSelector — 颜色选择器

### 5.1 头文件设计

```cpp
// src/ui/components/colorselector.h
#pragma once
#include <QWidget>
#include <QList>
#include <QColor>

class ColorSelector : public QWidget {
    Q_OBJECT
public:
    explicit ColorSelector(QWidget* parent = nullptr);

    // 设置当前选中颜色（用于编辑时回显）
    void setCurrentColor(const QString& colorHex);
    // 获取当前选中颜色
    QString currentColor() const;

signals:
    void colorSelected(const QString& colorHex); // 用户点击颜色时发出

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override; // 用于 hover 效果

private:
    // 预设颜色列表
    QList<QColor> colors_ = {
        QColor("#FFEAA7"), // 黄色（默认）
        QColor("#A8E6CF"), // 绿色
        QColor("#FFB3BA"), // 粉色
        QColor("#B3D9FF"), // 蓝色
        QColor("#E8D5FF"), // 紫色
        QColor("#FFD4A8"), // 橙色
        QColor("#FFFFFF"), // 白色
        QColor("#F0F0F0"), // 灰色
    };

    QString currentColor_; // 当前选中颜色的 Hex 字符串
    int     hoveredIndex_ = -1; // 当前鼠标悬停的颜色索引

    // 计算第 i 个颜色块的矩形区域
    QRect colorRect(int index) const;
    // 根据鼠标位置找到对应的颜色索引，找不到返回 -1
    int hitTest(const QPoint& pos) const;
};
```

### 5.2 实现要点

```cpp
// src/ui/components/colorselector.cpp
#include "colorselector.h"
#include <QPainter>
#include <QMouseEvent>

static constexpr int CIRCLE_SIZE   = 24; // 圆形直径
static constexpr int CIRCLE_MARGIN = 6;  // 圆形间距

ColorSelector::ColorSelector(QWidget* parent) : QWidget(parent) {
    currentColor_ = "#FFEAA7";
    // 根据颜色数量计算固定尺寸
    int cols = 4;
    int rows = (colors_.size() + cols - 1) / cols;
    int w = cols * (CIRCLE_SIZE + CIRCLE_MARGIN) + CIRCLE_MARGIN;
    int h = rows * (CIRCLE_SIZE + CIRCLE_MARGIN) + CIRCLE_MARGIN;
    setFixedSize(w, h);
    setMouseTracking(true); // 开启鼠标追踪，才能收到 mouseMoveEvent
}

QRect ColorSelector::colorRect(int index) const {
    int cols = 4;
    int col = index % cols;
    int row = index / cols;
    int x = CIRCLE_MARGIN + col * (CIRCLE_SIZE + CIRCLE_MARGIN);
    int y = CIRCLE_MARGIN + row * (CIRCLE_SIZE + CIRCLE_MARGIN);
    return QRect(x, y, CIRCLE_SIZE, CIRCLE_SIZE);
}

int ColorSelector::hitTest(const QPoint& pos) const {
    for (int i = 0; i < colors_.size(); ++i) {
        if (colorRect(i).contains(pos)) return i;
    }
    return -1;
}

void ColorSelector::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    for (int i = 0; i < colors_.size(); ++i) {
        QRect rect = colorRect(i);
        QColor color = colors_[i];

        // 绘制圆形背景
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(rect);

        // 选中状态：绘制外圈
        if (color.name() == currentColor_) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor("#333333"), 2));
            painter.drawEllipse(rect.adjusted(-3, -3, 3, 3));
        }

        // hover 状态：绘制半透明遮罩
        if (i == hoveredIndex_) {
            painter.setBrush(QColor(0, 0, 0, 30));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(rect);
        }
    }
}

void ColorSelector::mousePressEvent(QMouseEvent* event) {
    int idx = hitTest(event->position().toPoint());
    if (idx >= 0) {
        currentColor_ = colors_[idx].name();
        update(); // 触发重绘
        emit colorSelected(currentColor_);
    }
}

void ColorSelector::mouseMoveEvent(QMouseEvent* event) {
    int idx = hitTest(event->position().toPoint());
    if (idx != hoveredIndex_) {
        hoveredIndex_ = idx;
        update(); // 触发重绘
    }
}
```

---

## 六、NoteEditDialog — 编辑弹窗

### 6.1 头文件设计

```cpp
// src/ui/noteeditdialog.h
#pragma once
#include <QDialog>
#include "common/notedata.h"

class QLineEdit;
class QTextEdit;
class QComboBox;
class QPushButton;
class ColorSelector;

class NoteEditDialog : public QDialog {
    Q_OBJECT
public:
    // 新建模式：传入空 NoteData
    // 编辑模式：传入已有 NoteData
    explicit NoteEditDialog(const NoteData& note, QWidget* parent = nullptr);

    // 获取用户编辑后的数据
    NoteData result() const;

private slots:
    void onSaveClicked();
    void onCancelClicked();
    void onBoldClicked();    // 加粗
    void onItalicClicked();  // 斜体
    void onColorClicked();   // 文字颜色

private:
    void initUI();
    void connectSignals();
    void loadNote(const NoteData& note); // 将 note 数据填入控件

    NoteData note_; // 当前编辑的便签数据（保存时更新此对象）

    QLineEdit*     title_edit_    = nullptr;
    QTextEdit*     content_edit_  = nullptr;
    QComboBox*     category_combo_= nullptr;
    ColorSelector* color_selector_= nullptr;
    QPushButton*   bold_btn_      = nullptr;
    QPushButton*   italic_btn_    = nullptr;
    QPushButton*   save_btn_      = nullptr;
    QPushButton*   cancel_btn_    = nullptr;
};
```

### 6.2 实现要点

```cpp
// src/ui/noteeditdialog.cpp
#include "noteeditdialog.h"
#include "components/colorselector.h"
#include "core/notemanager.h"
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

NoteEditDialog::NoteEditDialog(const NoteData& note, QWidget* parent)
    : QDialog(parent), note_(note)
{
    setWindowTitle(note_.id.isEmpty() ? "新建便签" : "编辑便签");
    setMinimumSize(400, 350);
    initUI();
    connectSignals();
    loadNote(note_);
}

void NoteEditDialog::initUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // 标题输入
    title_edit_ = new QLineEdit(this);
    title_edit_->setPlaceholderText("便签标题...");
    mainLayout->addWidget(new QLabel("标题", this));
    mainLayout->addWidget(title_edit_);

    // 富文本工具栏
    auto* toolBar = new QHBoxLayout();
    bold_btn_   = new QPushButton("B", this);
    italic_btn_ = new QPushButton("I", this);
    bold_btn_->setCheckable(true);   // 可切换状态
    italic_btn_->setCheckable(true);
    toolBar->addWidget(bold_btn_);
    toolBar->addWidget(italic_btn_);
    toolBar->addStretch();
    mainLayout->addLayout(toolBar);

    // 内容编辑器
    content_edit_ = new QTextEdit(this);
    content_edit_->setPlaceholderText("便签内容...");
    mainLayout->addWidget(content_edit_);

    // 分类选择
    category_combo_ = new QComboBox(this);
    for (const QString& cat : NoteManager::instance()->categories()) {
        category_combo_->addItem(cat);
    }
    mainLayout->addWidget(new QLabel("分类", this));
    mainLayout->addWidget(category_combo_);

    // 颜色选择器
    color_selector_ = new ColorSelector(this);
    mainLayout->addWidget(new QLabel("颜色", this));
    mainLayout->addWidget(color_selector_);

    // 底部按钮
    auto* btnLayout = new QHBoxLayout();
    save_btn_   = new QPushButton("保存", this);
    cancel_btn_ = new QPushButton("取消", this);
    btnLayout->addStretch();
    btnLayout->addWidget(cancel_btn_);
    btnLayout->addWidget(save_btn_);
    mainLayout->addLayout(btnLayout);
}

void NoteEditDialog::connectSignals() {
    connect(save_btn_,   &QPushButton::clicked, this, &NoteEditDialog::onSaveClicked);
    connect(cancel_btn_, &QPushButton::clicked, this, &NoteEditDialog::onCancelClicked);
    connect(bold_btn_,   &QPushButton::clicked, this, &NoteEditDialog::onBoldClicked);
    connect(italic_btn_, &QPushButton::clicked, this, &NoteEditDialog::onItalicClicked);
}

void NoteEditDialog::loadNote(const NoteData& note) {
    title_edit_->setText(note.title);
    content_edit_->setHtml(note.content); // 富文本用 setHtml
    int idx = category_combo_->findText(note.category);
    if (idx >= 0) category_combo_->setCurrentIndex(idx);
    color_selector_->setCurrentColor(note.color);
}

void NoteEditDialog::onSaveClicked() {
    note_.title    = title_edit_->text().trimmed();
    note_.content  = content_edit_->toHtml(); // 保存富文本 HTML
    note_.category = category_combo_->currentText();
    note_.color    = color_selector_->currentColor();
    note_.modifiedAt = QDateTime::currentDateTime();
    accept(); // 关闭对话框，返回 QDialog::Accepted
}

void NoteEditDialog::onCancelClicked() {
    reject(); // 关闭对话框，返回 QDialog::Rejected
}

void NoteEditDialog::onBoldClicked() {
    // 切换选中文字的加粗状态
    QTextCharFormat fmt;
    fmt.setFontWeight(bold_btn_->isChecked() ? QFont::Bold : QFont::Normal);
    content_edit_->textCursor().mergeCharFormat(fmt);
}

void NoteEditDialog::onItalicClicked() {
    QTextCharFormat fmt;
    fmt.setFontItalic(italic_btn_->isChecked());
    content_edit_->textCursor().mergeCharFormat(fmt);
}

NoteData NoteEditDialog::result() const {
    return note_;
}
```

---

## 七、MainWindow 修改 — 接入搜索与新建

### 7.1 需要修改的地方

在 `MainWindow` 中需要做以下改动：

1. **在 TitleBar 中添加搜索框**（或在内容区顶部添加搜索栏）
2. **添加"新建便签"按钮**
3. **将 `NoteListModel` 套上 `NoteFilterProxyModel`**，View 绑定代理模型
4. **连接搜索框信号** → 调用 `proxy->setKeyword()`
5. **连接侧边栏分类信号** → 调用 `proxy->setCategory()`
6. **连接 NoteListView 的双击信号** → 打开编辑对话框
7. **连接 NoteListView 的右键菜单** → 删除便签

### 7.2 代理模型的接入方式

```cpp
// mainwindow.cpp 中的关键代码

// 创建模型和代理
note_model_ = new NoteListModel(this);
note_proxy_ = new NoteFilterProxyModel(this);
note_proxy_->setSourceModel(note_model_); // 代理包装原始模型

// View 绑定代理模型（不是原始模型！）
note_list_view_->setModel(note_proxy_);
note_list_view_->setItemDelegate(note_delegate_);

// 搜索框信号连接
connect(search_edit_, &QLineEdit::textChanged,
        note_proxy_,  &NoteFilterProxyModel::setKeyword);

// 侧边栏分类信号连接
connect(sidebar_, &SideBar::categoryChanged,
        note_proxy_, &NoteFilterProxyModel::setCategory);
```

### 7.3 新建便签流程

```cpp
void MainWindow::onNewNoteClicked() {
    // 创建一个空的 NoteData（工厂方法自动生成 id 和时间戳）
    NoteData newNote = NoteData::createNew();

    NoteEditDialog dlg(newNote, this);
    if (dlg.exec() == QDialog::Accepted) {
        // 用户点了保存
        NoteData result = dlg.result();
        NoteManager::instance()->addNote(result);
        // addNote 内部会 emit dataChanged()，NoteListModel 的 refresh() 会自动触发
    }
}
```

### 7.4 编辑便签流程

```cpp
void MainWindow::onNoteDoubleClicked(const QModelIndex& proxyIndex) {
    // 注意：View 绑定的是代理模型，需要将代理索引转换为源模型索引
    QModelIndex sourceIndex = note_proxy_->mapToSource(proxyIndex);

    // 从源模型获取 NoteData
    NoteData note = sourceIndex.data(NoteDataRole).value<NoteData>();

    NoteEditDialog dlg(note, this);
    if (dlg.exec() == QDialog::Accepted) {
        NoteData result = dlg.result();
        NoteManager::instance()->updateNote(result);
    }
}
```

### 7.5 删除便签流程（右键菜单）

```cpp
void MainWindow::onNoteContextMenu(const QPoint& pos) {
    QModelIndex proxyIndex = note_list_view_->indexAt(pos);
    if (!proxyIndex.isValid()) return;

    QModelIndex sourceIndex = note_proxy_->mapToSource(proxyIndex);
    QString noteId = sourceIndex.data(NoteIdRole).toString();

    QMenu menu(this);
    QAction* deleteAction = menu.addAction("删除便签");
    QAction* chosen = menu.exec(note_list_view_->viewport()->mapToGlobal(pos));

    if (chosen == deleteAction) {
        NoteManager::instance()->removeNote(noteId);
    }
}
```

---

## 八、SideBar 修改 — 分类管理

### 8.1 新增功能

在侧边栏底部添加"新建分类"按钮，点击后弹出输入框，输入分类名称后添加到 `NoteManager`。

```cpp
// sidebar.cpp 中新增
void SideBar::onAddCategoryClicked() {
    bool ok;
    QString name = QInputDialog::getText(this, "新建分类", "分类名称：",
                                         QLineEdit::Normal, "", &ok);
    if (ok && !name.trimmed().isEmpty()) {
        NoteManager::instance()->addCategory(name.trimmed());
        refreshCategories(); // 刷新侧边栏按钮
    }
}
```

> **注意**：需要在 `NoteManager` 中新增 `addCategory()` 和 `removeCategory()` 接口。

### 8.2 NoteManager 新增接口

```cpp
// notemanager.h 中新增
void addCategory(const QString& name);
void removeCategory(const QString& name);

// notemanager.cpp 中实现
void NoteManager::addCategory(const QString& name) {
    if (!customCategories_.contains(name)) {
        customCategories_.append(name);
        save();
        emit dataChanged();
    }
}

void NoteManager::removeCategory(const QString& name) {
    customCategories_.removeAll(name);
    save();
    emit dataChanged();
}
```

---

## 九、CMakeLists.txt 更新

在 `CMakeLists.txt` 的 `target_sources` 中新增以下文件：

```cmake
target_sources(StickyNoteManager PRIVATE
    # ... 已有文件 ...

    # 三期新增
    src/models/notefilterproxy.h
    src/models/notefilterproxy.cpp
    src/ui/noteeditdialog.h
    src/ui/noteeditdialog.cpp
    src/ui/components/colorselector.h
    src/ui/components/colorselector.cpp
)
```

---

## 十、开发顺序建议

按照以下顺序开发，每完成一步都能看到效果，避免一次性写太多代码调试困难：

```
Step 1: NoteFilterProxyModel
  → 先写好过滤逻辑，在 MainWindow 中接入，验证搜索和分类过滤是否生效

Step 2: ColorSelector
  → 独立组件，先单独测试绘制和点击是否正确

Step 3: NoteEditDialog（先不加 ColorSelector）
  → 实现基本的标题/内容编辑和保存，验证新建/编辑流程

Step 4: 将 ColorSelector 嵌入 NoteEditDialog
  → 验证颜色选择后卡片背景色是否变化

Step 5: SideBar 分类管理
  → 新增"新建分类"按钮，验证分类保存和侧边栏刷新

Step 6: 右键菜单删除
  → 最后添加删除功能，验证完整的增删改查流程
```

---

## 十一、数据流总览

```
用户操作
    │
    ▼
NoteEditDialog（编辑/新建）
    │ dlg.exec() == Accepted
    ▼
NoteManager::addNote() / updateNote() / removeNote()
    │ emit dataChanged()
    ▼
NoteListModel::refresh()
    │ beginResetModel() / endResetModel()
    ▼
NoteFilterProxyModel（自动重新过滤）
    │
    ▼
NoteListView（自动重绘）
    │
    ▼
NoteCardDelegate::paint()（绘制每张卡片）
```

---

## 十二、常见问题与注意事项

### 12.1 代理模型索引转换

View 绑定的是 `NoteFilterProxyModel`，所以双击、右键等操作拿到的 `QModelIndex` 都是**代理索引**，必须用 `mapToSource()` 转换为源模型索引，才能正确获取数据。

```cpp
// 错误：直接用代理索引访问源模型数据
NoteData note = proxyIndex.data(NoteDataRole).value<NoteData>(); // ✅ 这个其实可以，代理会转发

// 但如果需要源模型的行号（如删除操作），必须转换
QModelIndex sourceIndex = note_proxy_->mapToSource(proxyIndex);
int sourceRow = sourceIndex.row(); // 源模型中的真实行号
```

### 12.2 QTextEdit 富文本与纯文本

- `setHtml()` / `toHtml()`：读写富文本（保留格式）
- `setPlainText()` / `toPlainText()`：读写纯文本（丢失格式）
- 搜索过滤时，`NoteContentRole` 返回的是 HTML，需要用 `QTextDocument` 转为纯文本再匹配

### 12.3 QDialog::exec() 的阻塞特性

`exec()` 会**阻塞**当前线程，直到对话框关闭。这是模态对话框的标准用法，不需要担心信号槽的时序问题。

### 12.4 setMouseTracking

`ColorSelector` 需要在构造函数中调用 `setMouseTracking(true)`，否则只有在鼠标按下时才会收到 `mouseMoveEvent`，hover 效果无法实现。
