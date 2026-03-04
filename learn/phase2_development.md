# StickyNoteManager 第二阶段完整开发文档

---

## 一、阶段概述

### 1.1 二期目标

第二阶段（Day 3-4）的目标是在一期窗口骨架的基础上，实现**数据层**与**便签列表展示**，让应用能够真正加载、存储和展示便签数据。

| 功能点 | 说明 |
|--------|------|
| 定义 `NoteData` 数据结构 | 包含 id、title、content、color、category 等字段，支持 JSON 序列化 |
| 实现 `NoteManager` 数据管理 | 负责 JSON 文件的读写，提供增删改查接口 |
| 实现 `NoteListModel` 数据模型 | 继承 `QAbstractListModel`，将数据适配给 View |
| 实现 `NoteCardDelegate` 卡片委托 | 继承 `QStyledItemDelegate`，用 `QPainter` 绘制卡片 |
| 实现 `NoteListView` 列表视图 | 继承 `QListView`，配置网格布局 |
| 将列表嵌入主窗口内容区 | 替换一期的占位 `QLabel` |

### 1.2 验收标准

- 启动时从 `~/.stickynote/notes.json` 加载数据，文件不存在时以空列表启动
- 便签以卡片形式展示，卡片显示标题、正文预览（过滤 HTML 后截取 50 字）、分类标签、修改时间
- 卡片背景色跟随便签 `color` 字段
- 置顶便签显示 📌 标记并排在列表最前
- 数据变更后自动写回 JSON 文件（原子替换，防止崩溃丢失）
- JSON 文件损坏或不存在时可优雅降级（空列表启动）

---

## 二、新增文件结构

在一期基础上，二期新增以下文件：

```
StickyNoteManager/
└── src/
    ├── common/                         # 新增目录
    │   ├── notedata.h                  # 便签数据结构定义
    │   └── global.h                    # 全局常量与枚举
    ├── core/                           # 新增目录
    │   └── notemanager.h/.cpp          # 便签数据管理（增删改查、JSON 读写）
    ├── models/                         # 新增目录
    │   └── notelistmodel.h/.cpp        # 便签列表数据模型
    └── ui/
        ├── notelistview.h/.cpp         # 新增：便签列表视图
        ├── notecarddelegate.h/.cpp     # 新增：便签卡片绘制委托
        ├── mainwindow.h/.cpp           # 修改：嵌入列表视图
        └── sidebar.h/.cpp              # 修改：接入分类数据
```

同时需要更新 `CMakeLists.txt`，将新增的源文件加入编译列表。

---

## 三、类设计与职责

### 3.1 类关系图

```
MainWindow
├── TileBar
├── SideBar  ←──────────────────────────── NoteManager（读取分类列表）
└── NoteListView（内容区）
    ├── NoteFilterProxyModel（代理，二期可先不实现，直接用 NoteListModel）
    ├── NoteListModel  ←──────────────────── NoteManager（数据源）
    └── NoteCardDelegate（绘制每张卡片）

NoteManager
└── QList<NoteData>（内存数据）
    ├── load()  ← notes.json
    └── save()  → notes.json（原子替换）
```

### 3.2 各类职责说明

| 类 | 文件 | 职责 |
|----|------|------|
| `NoteData` | `src/common/notedata.h` | 便签数据结构，支持 `fromJson` / `toJson` |
| `NoteManager` | `src/core/notemanager.h/.cpp` | 单例，管理 `QList<NoteData>`，提供增删改查和 JSON 读写 |
| `NoteListModel` | `src/models/notelistmodel.h/.cpp` | 继承 `QAbstractListModel`，将数据暴露给 View |
| `NoteCardDelegate` | `src/ui/notecarddelegate.h/.cpp` | 继承 `QStyledItemDelegate`，用 `QPainter` 绘制卡片 |
| `NoteListView` | `src/ui/notelistview.h/.cpp` | 继承 `QListView`，配置网格布局和右键菜单（占位） |

---

## 四、数据结构设计

### 4.1 NoteData 结构体

```cpp
// src/common/notedata.h
#pragma once
#include <QString>
#include <QDateTime>
#include <QJsonObject>

struct NoteData {
    QString  id;          // UUID，唯一标识
    QString  title;       // 便签标题
    QString  content;     // 便签正文（富文本 HTML）
    QString  category;    // 分类标识，如 "工作"、"生活"
    QString  color;       // 背景色，如 "#FFEAA7"
    bool     pinned;      // 是否置顶，默认 false
    int      sortOrder;   // 手动排序序号，默认 0
    QDateTime createdAt;  // 创建时间
    QDateTime modifiedAt; // 最后修改时间

    // JSON 序列化
    QJsonObject toJson() const;
    static NoteData fromJson(const QJsonObject& obj);

    // 生成新便签（自动填充 id 和时间）
    static NoteData createNew(const QString& title = "",
                              const QString& content = "",
                              const QString& category = "未分类",
                              const QString& color = "#FFEAA7");
};
```

**字段说明：**

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `id` | `QString` | UUID | 用 `QUuid::createUuid().toString(QUuid::WithoutBraces)` 生成 |
| `title` | `QString` | `""` | 便签标题 |
| `content` | `QString` | `""` | 富文本 HTML，二期可先存纯文本 |
| `category` | `QString` | `"未分类"` | 分类名称 |
| `color` | `QString` | `"#FFEAA7"` | 5 种颜色之一 |
| `pinned` | `bool` | `false` | 置顶标记 |
| `sortOrder` | `int` | `0` | 排序序号，越小越靠前 |
| `createdAt` | `QDateTime` | 当前时间 | 创建时间 |
| `modifiedAt` | `QDateTime` | 当前时间 | 修改时间，每次保存时更新 |

### 4.2 JSON 序列化实现

```cpp
// notedata.cpp
QJsonObject NoteData::toJson() const {
    QJsonObject obj;
    obj["id"]         = id;
    obj["title"]      = title;
    obj["content"]    = content;
    obj["category"]   = category;
    obj["color"]      = color;
    obj["pinned"]     = pinned;
    obj["sortOrder"]  = sortOrder;
    obj["createdAt"]  = createdAt.toString(Qt::ISODate);
    obj["modifiedAt"] = modifiedAt.toString(Qt::ISODate);
    return obj;
}

NoteData NoteData::fromJson(const QJsonObject& obj) {
    NoteData note;
    note.id         = obj["id"].toString();
    note.title      = obj["title"].toString();
    note.content    = obj["content"].toString();
    note.category   = obj["category"].toString("未分类");
    note.color      = obj["color"].toString("#FFEAA7");
    note.pinned     = obj["pinned"].toBool(false);
    note.sortOrder  = obj["sortOrder"].toInt(0);
    note.createdAt  = QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate);
    note.modifiedAt = QDateTime::fromString(obj["modifiedAt"].toString(), Qt::ISODate);
    return note;
}
```

> ⚠️ **注意**：`fromJson` 中对每个字段都要提供默认值（`toString("默认值")`），防止 JSON 字段缺失时崩溃。

### 4.3 全局常量

```cpp
// src/common/global.h
#pragma once
#include <QStringList>

// 便签背景色色板
inline const QStringList NOTE_COLORS = {
    "#FFEAA7",  // 黄
    "#81ECEC",  // 青
    "#DFE6E9",  // 灰
    "#FAB1A0",  // 红
    "#A29BFE"   // 紫
};

// 内置分类（不可删除）
inline const QStringList BUILTIN_CATEGORIES = {
    "全部", "工作", "生活", "学习"
};

// 数据文件路径
inline QString notesFilePath() {
    return QDir::homePath() + "/.stickynote/notes.json";
}
```

---

## 五、NoteManager 数据管理

### 5.1 接口设计

```cpp
// src/core/notemanager.h
#pragma once
#include <QObject>
#include <QList>
#include "common/notedata.h"

class NoteManager : public QObject {
    Q_OBJECT
public:
    // 单例访问
    static NoteManager* instance();

    // 数据加载与保存
    void load();   // 从 JSON 文件加载
    void save();   // 保存到 JSON 文件（原子替换）

    // 数据访问
    const QList<NoteData>& notes() const { return notes_; }
    NoteData* findById(const QString& id);

    // 增删改
    void addNote(const NoteData& note);
    void updateNote(const NoteData& note);
    void removeNote(const QString& id);

    // 分类管理
    QStringList categories() const;  // 返回所有分类（内置 + 自定义）

signals:
    void dataChanged();  // 数据变更时发出，通知 Model 刷新

private:
    explicit NoteManager(QObject* parent = nullptr);
    static NoteManager* instance_;

    QList<NoteData> notes_;
    QStringList     customCategories_;  // 自定义分类列表
};
```

### 5.2 JSON 文件读写

#### 加载（load）

```cpp
void NoteManager::load() {
    QString path = notesFilePath();
    QFile file(path);

    if (!file.exists()) {
        // 文件不存在：首次启动，以空列表运行
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件：" << path;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "JSON 解析失败：" << error.errorString();
        // 文件损坏：优雅降级，以空列表运行
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray  arr  = root["notes"].toArray();

    notes_.clear();
    for (const QJsonValue& val : arr) {
        if (val.isObject()) {
            notes_.append(NoteData::fromJson(val.toObject()));
        }
    }

    // 加载自定义分类
    QJsonArray catArr = root["customCategories"].toArray();
    customCategories_.clear();
    for (const QJsonValue& val : catArr) {
        customCategories_.append(val.toString());
    }
}
```

#### 保存（save）—— 原子替换

```cpp
void NoteManager::save() {
    QString path = notesFilePath();

    // 确保目录存在
    QDir dir = QFileInfo(path).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 构建 JSON
    QJsonArray notesArr;
    for (const NoteData& note : notes_) {
        notesArr.append(note.toJson());
    }

    QJsonArray catArr;
    for (const QString& cat : customCategories_) {
        catArr.append(cat);
    }

    QJsonObject root;
    root["notes"]            = notesArr;
    root["customCategories"] = catArr;

    QJsonDocument doc(root);
    QByteArray    jsonData = doc.toJson(QJsonDocument::Indented);

    // 原子替换：先写临时文件，再重命名
    QString tmpPath = path + ".tmp";
    QFile   tmpFile(tmpPath);

    if (!tmpFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "无法写入临时文件：" << tmpPath;
        return;
    }

    tmpFile.write(jsonData);
    tmpFile.close();

    // 删除旧文件，重命名临时文件
    QFile::remove(path);
    QFile::rename(tmpPath, path);
}
```

**为什么要原子替换？**

直接写入目标文件时，如果程序在写入过程中崩溃，会导致文件内容损坏（部分写入）。原子替换的策略是：先将完整内容写入临时文件，写入成功后再用 `rename` 替换原文件。`rename` 操作在大多数操作系统上是原子的，不会出现中间状态。

---

## 六、NoteListModel 数据模型

### 6.1 接口设计

```cpp
// src/models/notelistmodel.h
#pragma once
#include <QAbstractListModel>
#include "common/notedata.h"

// 自定义数据角色
enum NoteRole {
    NoteIdRole       = Qt::UserRole + 1,
    NoteTitleRole,
    NoteContentRole,
    NoteCategoryRole,
    NoteColorRole,
    NotePinnedRole,
    NoteSortOrderRole,
    NoteModifiedAtRole,
    NoteDataRole,       // 返回完整的 NoteData（QVariant 包装）
};

class NoteListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit NoteListModel(QObject* parent = nullptr);

    // QAbstractListModel 必须实现的接口
    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    // 刷新数据（从 NoteManager 重新加载）
    void refresh();

private:
    QList<NoteData> sortedNotes_;  // 排序后的便签列表

    // 排序：置顶 > sortOrder > modifiedAt 倒序
    void sortNotes();
};
```

### 6.2 排序规则实现

```cpp
void NoteListModel::sortNotes() {
    sortedNotes_ = NoteManager::instance()->notes();

    std::stable_sort(sortedNotes_.begin(), sortedNotes_.end(),
        [](const NoteData& a, const NoteData& b) {
            // 1. 置顶优先
            if (a.pinned != b.pinned) return a.pinned > b.pinned;
            // 2. sortOrder 升序
            if (a.sortOrder != b.sortOrder) return a.sortOrder < b.sortOrder;
            // 3. 修改时间倒序（越新越靠前）
            return a.modifiedAt > b.modifiedAt;
        });
}
```

### 6.3 data() 实现

```cpp
QVariant NoteListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= sortedNotes_.size())
        return QVariant();

    const NoteData& note = sortedNotes_.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case NoteTitleRole:    return note.title;
    case NoteContentRole:  return note.content;
    case NoteCategoryRole: return note.category;
    case NoteColorRole:    return note.color;
    case NotePinnedRole:   return note.pinned;
    case NoteSortOrderRole:return note.sortOrder;
    case NoteModifiedAtRole: return note.modifiedAt;
    case NoteDataRole:     return QVariant::fromValue(note);
    default:               return QVariant();
    }
}
```

> ⚠️ **注意**：`NoteDataRole` 使用 `QVariant::fromValue(note)` 需要在头文件中声明 `Q_DECLARE_METATYPE(NoteData)`，并在 `main.cpp` 中调用 `qRegisterMetaType<NoteData>()`。

---

## 七、NoteCardDelegate 卡片绘制

### 7.1 卡片布局规格

```
┌────────────────────────────────────────┐  ← 卡片背景（圆角 8px，背景色跟随 color 字段）
│  📌 周会纪要                            │  ← 标题（14px bold），置顶时前缀 📌
│  讨论了Q2的规划方向，重点是...           │  ← 正文预览（12px，灰色，最多 50 字）
│  🏷️ 工作                    今天 10:30  │  ← 分类标签（左）+ 修改时间（右，10px）
└────────────────────────────────────────┘
```

| 项目 | 规格 |
|------|------|
| 卡片圆角 | 8px |
| 卡片内边距 | 12px |
| 标题字号 | 14px bold |
| 正文字号 | 12px，颜色 `#636E72` |
| 时间字号 | 10px，颜色 `#B2BEC3` |
| 分类标签字号 | 10px，颜色 `#636E72` |
| 卡片尺寸 | 宽度自适应列宽，高度约 100px |

### 7.2 paint() 实现思路

```cpp
void NoteCardDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // 1. 获取数据
    QString  title      = index.data(NoteTitleRole).toString();
    QString  content    = index.data(NoteContentRole).toString();
    QString  category   = index.data(NoteCategoryRole).toString();
    QString  color      = index.data(NoteColorRole).toString();
    bool     pinned     = index.data(NotePinnedRole).toBool();
    QDateTime modifiedAt = index.data(NoteModifiedAtRole).toDateTime();

    // 2. 过滤 HTML 标签，截取正文预览
    QString preview = stripHtml(content).left(50);

    // 3. 绘制卡片背景（圆角矩形）
    QRect cardRect = option.rect.adjusted(6, 6, -6, -6);  // 留出间距
    QPainterPath path;
    path.addRoundedRect(cardRect, 8, 8);
    painter->fillPath(path, QColor(color));

    // 4. 绘制标题
    QString titleText = pinned ? ("📌 " + title) : title;
    QFont titleFont = painter->font();
    titleFont.setPixelSize(14);
    titleFont.setBold(true);
    painter->setFont(titleFont);
    painter->setPen(QColor("#2D3436"));
    QRect titleRect = cardRect.adjusted(12, 10, -12, 0);
    titleRect.setHeight(20);
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                      painter->fontMetrics().elidedText(titleText, Qt::ElideRight, titleRect.width()));

    // 5. 绘制正文预览
    QFont previewFont = painter->font();
    previewFont.setPixelSize(12);
    previewFont.setBold(false);
    painter->setFont(previewFont);
    painter->setPen(QColor("#636E72"));
    QRect previewRect = cardRect.adjusted(12, 36, -12, 0);
    previewRect.setHeight(36);
    painter->drawText(previewRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, preview);

    // 6. 绘制分类标签（左下）和修改时间（右下）
    QFont footerFont = painter->font();
    footerFont.setPixelSize(10);
    painter->setFont(footerFont);

    QRect footerRect = cardRect.adjusted(12, 0, -12, -10);
    footerRect.setTop(cardRect.bottom() - 24);

    // 分类标签
    painter->setPen(QColor("#636E72"));
    painter->drawText(footerRect, Qt::AlignLeft | Qt::AlignVCenter, "🏷️ " + category);

    // 修改时间
    painter->setPen(QColor("#B2BEC3"));
    painter->drawText(footerRect, Qt::AlignRight | Qt::AlignVCenter,
                      formatTime(modifiedAt));

    painter->restore();
}
```

### 7.3 辅助函数

```cpp
// 过滤 HTML 标签，返回纯文本
QString NoteCardDelegate::stripHtml(const QString& html) {
    QTextDocument doc;
    doc.setHtml(html);
    return doc.toPlainText();
}

// 格式化时间：今天显示 "今天 HH:mm"，昨天显示 "昨天"，更早显示 "M月d日"
QString NoteCardDelegate::formatTime(const QDateTime& dt) {
    QDate today = QDate::currentDate();
    QDate date  = dt.date();

    if (date == today)
        return "今天 " + dt.toString("HH:mm");
    else if (date == today.addDays(-1))
        return "昨天";
    else
        return date.toString("M月d日");
}
```

### 7.4 sizeHint() 实现

```cpp
QSize NoteCardDelegate::sizeHint(const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const {
    Q_UNUSED(option)
    Q_UNUSED(index)
    // 固定卡片高度，宽度由 View 决定
    return QSize(200, 110);
}
```

---

## 八、NoteListView 列表视图

### 8.1 配置要点

```cpp
// src/ui/notelistview.cpp
NoteListView::NoteListView(QWidget* parent) : QListView(parent) {
    // 网格布局模式
    setViewMode(QListView::IconMode);
    setFlow(QListView::LeftToRight);
    setWrapping(true);
    setResizeMode(QListView::Adjust);
    setSpacing(8);

    // 禁用默认拖拽（二期先不实现拖拽排序）
    setDragEnabled(false);
    setAcceptDrops(false);

    // 去掉选中时的蓝色高亮框（由 Delegate 自行绘制选中态）
    setSelectionMode(QAbstractItemView::SingleSelection);

    // 去掉滚动条样式（可选）
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 背景透明，让父控件背景色透出
    setStyleSheet("QListView { background: transparent; border: none; }");
}
```

### 8.2 将 NoteListView 嵌入主窗口

在 `MainWindow::initUI()` 中，将一期的占位 `QLabel` 替换为 `NoteListView`：

```cpp
// mainwindow.cpp
#include "notelistview.h"
#include "models/notelistmodel.h"
#include "notecarddelegate.h"
#include "core/notemanager.h"

void MainWindow::initUI() {
    // ... 一期代码 ...

    // 替换内容区占位 Label
    note_model_    = new NoteListModel(this);
    note_delegate_ = new NoteCardDelegate(this);
    note_list_view_ = new NoteListView(this);
    note_list_view_->setModel(note_model_);
    note_list_view_->setItemDelegate(note_delegate_);

    // 将 note_list_view_ 加入内容区布局
    content_layout_->addWidget(note_list_view_);

    // 加载数据
    NoteManager::instance()->load();
    note_model_->refresh();
}
```

---

## 九、CMakeLists.txt 更新

在 `SOURCES` 和 `HEADERS` 中添加新文件：

```cmake
set(SOURCES
    src/main.cpp
    src/ui/mainwindow.cpp
    src/ui/tilebar.cpp
    src/ui/sidebar.cpp
    src/ui/notelistview.cpp
    src/ui/notecarddelegate.cpp
    src/core/notemanager.cpp
    src/models/notelistmodel.cpp
    src/utils/windowhelper.cpp
)

set(HEADERS
    src/ui/mainwindow.h
    src/ui/tilebar.h
    src/ui/sidebar.h
    src/ui/notelistview.h
    src/ui/notecarddelegate.h
    src/core/notemanager.h
    src/models/notelistmodel.h
    src/common/notedata.h
    src/common/global.h
    src/utils/windowhelper.h
)

# 更新包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
)
```

---

## 十、数据流说明

```
启动流程：
main.cpp
  → MainWindow::initUI()
      → NoteManager::instance()->load()   // 从 notes.json 读取数据到内存
      → NoteListModel::refresh()          // 从 NoteManager 拷贝数据，排序
      → NoteListView::setModel()          // View 向 Model 请求数据
          → NoteCardDelegate::paint()     // 绘制每张卡片

数据变更流程（以后续三期为例）：
用户操作（新建/编辑/删除）
  → NoteManager::addNote() / updateNote() / removeNote()
  → NoteManager::save()                  // 原子写入 notes.json
  → emit NoteManager::dataChanged()
  → NoteListModel::refresh()             // 重新排序，通知 View 刷新
  → NoteListView 重绘
```

---

## 十一、测试数据

为了方便开发调试，可以在 `NoteManager::load()` 中，当文件不存在时插入一批测试数据：

```cpp
if (!file.exists()) {
#ifdef QT_DEBUG
    // 调试模式下插入测试数据
    notes_ = {
        NoteData::createNew("周会纪要",   "讨论了Q2的规划方向，重点是...", "工作",  "#FFEAA7"),
        NoteData::createNew("买菜清单",   "西红柿🍅、鸡蛋、牛奶...",       "生活",  "#81ECEC"),
        NoteData::createNew("Qt学习笔记", "Model/View 架构：...",          "学习",  "#A29BFE"),
        NoteData::createNew("项目计划",   "第一阶段先完成窗口骨架...",      "工作",  "#FAB1A0"),
    };
    notes_[0].pinned = true;  // 第一条置顶
#endif
    return;
}
```

---

## 十二、注意事项与常见问题

| 问题 | 说明 | 解决方案 |
|------|------|---------|
| `NoteData` 无法存入 `QVariant` | 未注册元类型 | 在 `notedata.h` 末尾加 `Q_DECLARE_METATYPE(NoteData)`，在 `main.cpp` 中调用 `qRegisterMetaType<NoteData>()` |
| 卡片背景色不生效 | `QListView` 默认会绘制选中高亮覆盖 Delegate | 在 `paint()` 中先调用 `painter->fillPath(path, color)` 再绘制内容，并在 `QListView` 样式中设置 `selection-background-color: transparent` |
| 正文预览显示 HTML 标签 | 未过滤 HTML | 使用 `QTextDocument::toPlainText()` 过滤 |
| 卡片高度不一致 | `sizeHint` 返回值不统一 | 在 `sizeHint()` 中返回固定高度 `QSize(200, 110)` |
| 一期事件过滤器影响列表交互 | `MainWindow::eventFilter` 拦截了 `MouseMove` | 在 `eventFilter` 中判断：如果事件来自 `NoteListView` 或其子控件，则不做光标修改，直接 `return false` |
| 数据文件路径在 Windows 下异常 | `~` 在 Windows 下不等于 `QDir::homePath()` | 始终使用 `QDir::homePath() + "/.stickynote/notes.json"` |

> ⚠️ **重要提示**：一期在 `MainWindow::initUI()` 末尾为所有子控件安装了事件过滤器。二期新增 `NoteListView` 后，该视图及其内部的 `QScrollBar`、`QAbstractScrollArea` 等子控件也会被安装过滤器。如果发现列表的鼠标事件（点击、滚动）出现异常，请检查 `eventFilter` 中的光标更新逻辑是否误拦截了列表区域的事件。建议在 `eventFilter` 中增加判断：

```cpp
// 如果鼠标在内容区（NoteListView）内，不修改光标
if (note_list_view_ && note_list_view_->rect().contains(
        note_list_view_->mapFromGlobal(QCursor::pos()))) {
    return QWidget::eventFilter(watched, event);
}
```
