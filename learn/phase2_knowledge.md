# StickyNoteManager 第二阶段知识详解

> 本文档面向完全的新手，将二期开发文档中涉及的所有知识点逐一解释清楚。  
> 建议先阅读 `phase2_development.md`，再回来查阅本文档中对应的知识点。

---

## 目录

1. [Qt Model/View 架构](#一qt-modelview-架构)
2. [QAbstractListModel — 数据模型基类](#二qabstractlistmodel--数据模型基类)
3. [QStyledItemDelegate — 自定义绘制委托](#三qstyleditemdelegate--自定义绘制委托)
4. [QListView — 列表视图](#四qlistview--列表视图)
5. [QPainter — 2D 绘图](#五qpainter--2d-绘图)
6. [JSON 读写（Qt JSON 模块）](#六json-读写qt-json-模块)
7. [单例模式（Singleton）](#七单例模式singleton)
8. [QVariant 与元类型系统](#八qvariant-与元类型系统)
9. [std::stable_sort 与 Lambda 排序](#九stdstable_sort-与-lambda-排序)
10. [QUuid — 唯一标识符生成](#十quuid--唯一标识符生成)
11. [QDateTime — 日期时间处理](#十一qdatetime--日期时间处理)
12. [QTextDocument — HTML 转纯文本](#十二qtextdocument--html-转纯文本)
13. [原子文件替换](#十三原子文件替换)
14. [Qt 信号与槽（复习与进阶）](#十四qt-信号与槽复习与进阶)
15. [inline 变量（C++17）](#十五inline-变量c17)

---

## 一、Qt Model/View 架构

### 1.1 为什么需要 Model/View？

假设你有 1000 条便签数据，最直接的做法是创建 1000 个 `QLabel` 或 `QWidget` 放到界面上。但这样做有两个问题：

1. **性能差**：1000 个控件同时存在于内存中，即使屏幕上只能看到 10 个。
2. **耦合高**：数据和界面混在一起，改一个地方要改很多地方。

Qt 的 **Model/View 架构**把数据和界面彻底分离：

```
┌─────────────┐     数据请求      ┌─────────────┐
│    View     │ ──────────────→  │    Model    │
│  (显示数据)  │ ←──────────────  │  (存储数据)  │
└─────────────┘     数据返回      └─────────────┘
       ↑
       │ 自定义绘制
┌─────────────┐
│  Delegate   │
│ (绘制每一项) │
└─────────────┘
```

- **Model（模型）**：只负责存储和管理数据，不关心如何显示。
- **View（视图）**：只负责显示，不存储数据，需要数据时向 Model 请求。
- **Delegate（委托）**：负责每一项的具体绘制方式，可以完全自定义外观。

### 1.2 三者的分工

| 角色 | 对应类 | 职责 |
|------|--------|------|
| Model | `NoteListModel` | 管理便签列表，提供数据给 View |
| View | `NoteListView` | 显示便签卡片列表 |
| Delegate | `NoteCardDelegate` | 绘制每张便签卡片的外观 |

### 1.3 数据流动过程

View 需要显示第 3 行数据时，会调用：

```cpp
model->data(index, role)
```

Model 根据 `index`（第几行）和 `role`（要什么数据）返回对应的值。Delegate 拿到这些值后，用 `QPainter` 绘制出卡片。

---

## 二、QAbstractListModel — 数据模型基类

### 2.1 什么是抽象基类？

`QAbstractListModel` 是一个**抽象基类**，意思是它本身不能直接使用，必须继承它并实现特定的方法，才能得到一个可用的 Model。

就像"动物"是一个抽象概念，你必须具体到"猫"或"狗"才能实例化。

### 2.2 必须实现的两个方法

继承 `QAbstractListModel` 后，**必须**重写以下两个纯虚函数：

```cpp
// 1. 告诉 View 一共有多少行数据
int rowCount(const QModelIndex& parent = QModelIndex()) const override;

// 2. 根据行号和角色，返回对应的数据
QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
```

**示例实现：**

```cpp
int NoteListModel::rowCount(const QModelIndex& parent) const {
    // parent 用于树形结构，列表模型直接忽略
    if (parent.isValid()) return 0;
    return sortedNotes_.size();  // 返回便签总数
}
```

### 2.3 QModelIndex 是什么？

`QModelIndex` 是 Model 中某一项数据的"地址"，包含：
- `row()`：第几行
- `column()`：第几列（列表模型通常只有第 0 列）
- `isValid()`：是否有效（无效的 index 表示根节点）

```cpp
// 使用示例
const NoteData& note = sortedNotes_.at(index.row());
```

### 2.4 数据角色（Role）是什么？

同一行数据可能有多种用途：标题用于显示，颜色用于绘制背景，ID 用于操作。Qt 用**角色（Role）**来区分同一项数据的不同用途。

Qt 内置了一些角色：

| 角色常量 | 值 | 用途 |
|---------|-----|------|
| `Qt::DisplayRole` | 0 | 默认显示文本 |
| `Qt::DecorationRole` | 1 | 图标 |
| `Qt::ToolTipRole` | 3 | 提示文字 |
| `Qt::UserRole` | 256 | 自定义角色的起始值 |

二期中自定义了一组角色：

```cpp
enum NoteRole {
    NoteIdRole       = Qt::UserRole + 1,  // 257
    NoteTitleRole,                         // 258
    NoteContentRole,                       // 259
    // ...
};
```

> ⚠️ **注意**：自定义角色必须从 `Qt::UserRole + 1` 开始，避免与 Qt 内置角色冲突。

### 2.5 通知 View 数据已变化

当数据发生变化时，必须通知 View 刷新，否则界面不会更新。`QAbstractListModel` 提供了几个方法：

```cpp
// 通知 View 所有数据都变了（最简单粗暴，适合全量刷新）
beginResetModel();
// ... 修改数据 ...
endResetModel();
```

二期的 `refresh()` 方法就是这样实现的：

```cpp
void NoteListModel::refresh() {
    beginResetModel();   // 告诉 View：数据要重置了
    sortNotes();         // 重新从 NoteManager 加载并排序
    endResetModel();     // 告诉 View：数据重置完了，请刷新
}
```

> ⚠️ **注意**：`beginResetModel()` 和 `endResetModel()` 必须成对调用，否则 View 会进入异常状态。

---

## 三、QStyledItemDelegate — 自定义绘制委托

### 3.1 为什么需要 Delegate？

默认情况下，`QListView` 只会把 `Qt::DisplayRole` 返回的文本显示出来，非常朴素。如果想要绘制带背景色、圆角、多行文字的卡片，就需要自定义 Delegate。

### 3.2 需要重写的方法

```cpp
// 绘制一项（核心方法）
void paint(QPainter* painter,
           const QStyleOptionViewItem& option,
           const QModelIndex& index) const override;

// 返回每项的建议尺寸
QSize sizeHint(const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
```

### 3.3 QStyleOptionViewItem 是什么？

`QStyleOptionViewItem` 包含了绘制时需要的上下文信息：

| 字段 | 说明 |
|------|------|
| `option.rect` | 这一项在 View 中的矩形区域（位置和大小） |
| `option.state` | 当前状态（是否选中、是否悬停等） |
| `option.font` | 默认字体 |

`option.rect` 是最常用的，它告诉你"在哪里画"。

### 3.4 paint() 的基本结构

```cpp
void NoteCardDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    painter->save();   // 保存当前绘图状态（字体、颜色、变换等）

    // ... 绘制内容 ...

    painter->restore(); // 恢复绘图状态，避免影响其他项的绘制
}
```

> ⚠️ **重要**：`save()` 和 `restore()` 必须成对调用。每次 `paint()` 调用都会修改 `painter` 的状态（字体、颜色等），如果不恢复，会影响后续项的绘制。

---

## 四、QListView — 列表视图

### 4.1 两种显示模式

`QListView` 有两种显示模式，通过 `setViewMode()` 切换：

| 模式 | 常量 | 效果 |
|------|------|------|
| 列表模式 | `QListView::ListMode` | 每项占一行，从上到下排列（默认） |
| 图标模式 | `QListView::IconMode` | 每项像图标一样，从左到右、从上到下排列 |

二期使用 `IconMode` 实现网格卡片布局：

```cpp
setViewMode(QListView::IconMode);
setFlow(QListView::LeftToRight);  // 从左到右排列
setWrapping(true);                // 超出宽度时换行
setResizeMode(QListView::Adjust); // 窗口大小变化时重新排列
setSpacing(8);                    // 卡片间距 8px
```

### 4.2 setResizeMode(QListView::Adjust) 的作用

当窗口宽度变化时，`Adjust` 模式会自动重新计算每行能放几张卡片，并重新排列。如果不设置，拖拽窗口变宽时卡片不会自动重排。

### 4.3 去掉选中高亮

默认情况下，点击某项时会出现蓝色高亮背景，这会覆盖 Delegate 绘制的卡片背景色。解决方法：

```cpp
// 方法一：在 QListView 的样式表中设置
setStyleSheet("QListView::item:selected { background: transparent; }");

// 方法二：在 Delegate 的 paint() 中不处理选中态，完全自绘
```

---

## 五、QPainter — 2D 绘图

### 5.1 QPainter 基础概念

`QPainter` 是 Qt 的 2D 绘图引擎，可以在任何 `QPaintDevice`（`QWidget`、`QPixmap`、`QImage` 等）上绘图。

绘图前必须设置好"画笔"和"画刷"：

| 概念 | 类 | 作用 |
|------|-----|------|
| 画笔（Pen） | `QPen` | 控制线条颜色、宽度、样式（用于描边） |
| 画刷（Brush） | `QBrush` | 控制填充颜色和样式（用于填充区域） |

```cpp
// 设置画笔（描边）
painter->setPen(QColor("#2D3436"));   // 深灰色描边
painter->setPen(Qt::NoPen);           // 不描边

// 设置画刷（填充）
painter->setBrush(QColor("#FFEAA7")); // 黄色填充
painter->setBrush(Qt::NoBrush);       // 不填充
```

### 5.2 常用绘图方法

```cpp
// 绘制矩形
painter->drawRect(QRect(x, y, width, height));

// 绘制圆角矩形
painter->drawRoundedRect(rect, xRadius, yRadius);

// 绘制文字
painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, "文字内容");

// 填充路径（不描边，只填充）
painter->fillPath(path, QColor("#FFEAA7"));
```

### 5.3 QPainterPath — 复杂路径

`QPainterPath` 用于描述复杂的几何形状（圆角矩形、贝塞尔曲线等）：

```cpp
QPainterPath path;
path.addRoundedRect(cardRect, 8, 8);  // 添加圆角矩形路径，圆角半径 8px

// 用路径填充颜色
painter->fillPath(path, QColor(color));

// 用路径描边
painter->drawPath(path);
```

**为什么用 `fillPath` 而不是 `drawRoundedRect`？**

`drawRoundedRect` 会同时描边和填充，而 `fillPath` 只填充不描边。当你不想要边框线时，用 `fillPath` 更简洁（不需要先设置 `Qt::NoPen`）。

### 5.4 抗锯齿

圆角矩形的边缘默认是锯齿状的，开启抗锯齿后会更平滑：

```cpp
painter->setRenderHint(QPainter::Antialiasing);
```

> ⚠️ **注意**：抗锯齿会略微降低绘制性能，但对于卡片这种静态内容影响可以忽略。

### 5.5 QRect::adjusted() — 矩形调整

`adjusted(dx1, dy1, dx2, dy2)` 返回一个调整后的新矩形：

```cpp
QRect cardRect = option.rect.adjusted(6, 6, -6, -6);
// 等价于：
// left   += 6   （左边向右缩进 6px）
// top    += 6   （上边向下缩进 6px）
// right  -= 6   （右边向左缩进 6px）
// bottom -= 6   （下边向上缩进 6px）
// 结果：四周各留出 6px 的间距
```

### 5.6 文字省略（elidedText）

当文字太长超出矩形时，用省略号截断：

```cpp
QString elided = painter->fontMetrics().elidedText(
    titleText,          // 原始文字
    Qt::ElideRight,     // 省略号在右边（"周会纪要..."）
    titleRect.width()   // 可用宽度
);
painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, elided);
```

`Qt::ElideRight` 表示超出部分用 `...` 替代，显示在右边。

### 5.7 文字对齐标志

`drawText` 的第二个参数是对齐标志，可以用 `|` 组合：

| 标志 | 说明 |
|------|------|
| `Qt::AlignLeft` | 水平左对齐 |
| `Qt::AlignRight` | 水平右对齐 |
| `Qt::AlignHCenter` | 水平居中 |
| `Qt::AlignTop` | 垂直顶部对齐 |
| `Qt::AlignBottom` | 垂直底部对齐 |
| `Qt::AlignVCenter` | 垂直居中 |
| `Qt::TextWordWrap` | 自动换行 |

---

## 六、JSON 读写（Qt JSON 模块）

### 6.1 JSON 基础

JSON（JavaScript Object Notation）是一种轻量级的数据交换格式，便签数据存储在 JSON 文件中：

```json
{
    "notes": [
        {
            "id": "abc123",
            "title": "周会纪要",
            "content": "讨论了Q2的规划方向...",
            "category": "工作",
            "color": "#FFEAA7",
            "pinned": true,
            "sortOrder": 0,
            "createdAt": "2026-03-04T10:00:00",
            "modifiedAt": "2026-03-04T10:30:00"
        }
    ],
    "customCategories": ["项目A", "项目B"]
}
```

### 6.2 Qt JSON 相关类

| 类 | 说明 |
|----|------|
| `QJsonDocument` | JSON 文档的顶层容器，负责解析和序列化 |
| `QJsonObject` | JSON 对象（`{}`），键值对集合 |
| `QJsonArray` | JSON 数组（`[]`），有序列表 |
| `QJsonValue` | JSON 值，可以是字符串、数字、布尔、对象、数组或 null |
| `QJsonParseError` | 解析错误信息 |

### 6.3 读取 JSON 文件

```cpp
// 第一步：读取文件内容
QFile file("notes.json");
file.open(QIODevice::ReadOnly);
QByteArray data = file.readAll();
file.close();

// 第二步：解析 JSON
QJsonParseError error;
QJsonDocument doc = QJsonDocument::fromJson(data, &error);

// 第三步：检查解析是否成功
if (error.error != QJsonParseError::NoError) {
    qWarning() << "解析失败：" << error.errorString();
    return;
}

// 第四步：取出数据
QJsonObject root = doc.object();           // 取根对象 {}
QJsonArray arr = root["notes"].toArray();  // 取 notes 数组

for (const QJsonValue& val : arr) {
    QJsonObject obj = val.toObject();
    QString title = obj["title"].toString();
    // ...
}
```

### 6.4 写入 JSON 文件

```cpp
// 第一步：构建 JSON 结构
QJsonArray notesArr;
for (const NoteData& note : notes_) {
    notesArr.append(note.toJson());  // 每条便签转为 QJsonObject
}

QJsonObject root;
root["notes"] = notesArr;

// 第二步：序列化为字节数组
QJsonDocument doc(root);
QByteArray jsonData = doc.toJson(QJsonDocument::Indented);  // 格式化输出（有缩进）

// 第三步：写入文件
QFile file("notes.json");
file.open(QIODevice::WriteOnly | QIODevice::Truncate);
file.write(jsonData);
file.close();
```

### 6.5 QJsonValue 的类型转换

从 `QJsonObject` 取值时，返回的是 `QJsonValue`，需要转换为具体类型：

```cpp
QJsonObject obj = /* ... */;

// 转换方法（带默认值，字段不存在时返回默认值）
QString  title    = obj["title"].toString("默认标题");
bool     pinned   = obj["pinned"].toBool(false);
int      order    = obj["sortOrder"].toInt(0);
double   score    = obj["score"].toDouble(0.0);
QJsonArray arr    = obj["items"].toArray();
QJsonObject child = obj["child"].toObject();
```

> ⚠️ **重要**：始终为 `toString()`、`toBool()` 等方法提供默认值参数。如果 JSON 文件中某个字段缺失，没有默认值时会返回空字符串或 `false`，可能导致数据异常。

---

## 七、单例模式（Singleton）

### 7.1 什么是单例？

单例模式保证一个类在整个程序运行期间**只有一个实例**，并提供一个全局访问点。

`NoteManager` 使用单例，是因为整个应用只需要一个数据管理器，所有地方都访问同一份数据。

### 7.2 Qt 中的单例实现

```cpp
// notemanager.h
class NoteManager : public QObject {
public:
    static NoteManager* instance();  // 全局访问点

private:
    explicit NoteManager(QObject* parent = nullptr);  // 构造函数私有化
    static NoteManager* instance_;                    // 唯一实例指针
};

// notemanager.cpp
NoteManager* NoteManager::instance_ = nullptr;  // 初始化为 nullptr

NoteManager* NoteManager::instance() {
    if (!instance_) {
        instance_ = new NoteManager();  // 第一次调用时创建
    }
    return instance_;
}
```

**使用方式：**

```cpp
// 任何地方都可以这样访问
NoteManager::instance()->load();
NoteManager::instance()->addNote(note);
```

### 7.3 为什么构造函数要私有化？

如果构造函数是 `public` 的，任何地方都可以 `new NoteManager()`，就无法保证只有一个实例。私有化构造函数后，外部代码只能通过 `instance()` 获取实例，无法自己创建。

### 7.4 单例的注意事项

- 单例对象的生命周期与程序相同，不会被自动销毁
- 在多线程环境下，上面的简单实现不是线程安全的（二期是单线程，暂不考虑）
- 单例会增加代码耦合度，不要滥用

---

## 八、QVariant 与元类型系统

### 8.1 QVariant 是什么？

`QVariant` 是 Qt 的"万能类型"，可以存储任意类型的值：

```cpp
QVariant v1 = 42;           // 存储 int
QVariant v2 = "hello";      // 存储 QString
QVariant v3 = true;         // 存储 bool
QVariant v4 = QDateTime::currentDateTime();  // 存储 QDateTime
```

Model 的 `data()` 方法返回 `QVariant`，是因为不同的 Role 可能返回不同类型的数据，用 `QVariant` 统一包装。

### 8.2 为什么需要注册元类型？

Qt 的信号槽、`QVariant` 等机制依赖**元对象系统（Meta-Object System）**。对于 Qt 内置类型（`QString`、`int`、`QDateTime` 等），Qt 已经自动注册了。但对于自定义结构体（如 `NoteData`），需要手动注册。

**注册步骤：**

```cpp
// 第一步：在头文件末尾声明（告诉 Qt 这个类型可以存入 QVariant）
Q_DECLARE_METATYPE(NoteData)

// 第二步：在 main.cpp 中注册（让 Qt 运行时知道这个类型）
qRegisterMetaType<NoteData>();
```

**使用：**

```cpp
// 存入 QVariant
QVariant v = QVariant::fromValue(note);

// 从 QVariant 取出
NoteData note = v.value<NoteData>();
```

### 8.3 QVariant 的类型转换

```cpp
QVariant v = index.data(NoteTitleRole);

// 转换为具体类型
QString  title    = v.toString();
bool     pinned   = v.toBool();
int      order    = v.toInt();
QDateTime dt      = v.toDateTime();
NoteData  note    = v.value<NoteData>();  // 自定义类型用 value<T>()
```

---

## 九、std::stable_sort 与 Lambda 排序

### 9.1 std::sort vs std::stable_sort

| 函数 | 说明 |
|------|------|
| `std::sort` | 不稳定排序，相等元素的相对顺序可能改变 |
| `std::stable_sort` | 稳定排序，相等元素的相对顺序保持不变 |

二期使用 `std::stable_sort`，是因为当两条便签的 `sortOrder` 和 `modifiedAt` 都相同时，希望保持它们原来的顺序，避免每次刷新时顺序随机变化。

### 9.2 Lambda 表达式基础

Lambda 是 C++11 引入的匿名函数，语法：

```cpp
[捕获列表](参数列表) -> 返回类型 {
    函数体
}
```

**示例：**

```cpp
// 最简单的 lambda
auto add = [](int a, int b) { return a + b; };
int result = add(3, 4);  // result = 7

// 捕获外部变量
int threshold = 10;
auto isLarge = [threshold](int x) { return x > threshold; };
```

### 9.3 用 Lambda 自定义排序规则

`std::stable_sort` 的第三个参数是一个**比较函数**，返回 `true` 表示第一个参数应该排在第二个参数前面：

```cpp
std::stable_sort(sortedNotes_.begin(), sortedNotes_.end(),
    [](const NoteData& a, const NoteData& b) {
        // 规则 1：置顶的排在前面
        // a.pinned=true, b.pinned=false → true（a 排前面）✅
        // a.pinned=false, b.pinned=true → false（b 排前面）✅
        if (a.pinned != b.pinned) return a.pinned > b.pinned;

        // 规则 2：sortOrder 小的排前面
        if (a.sortOrder != b.sortOrder) return a.sortOrder < b.sortOrder;

        // 规则 3：修改时间新的排前面（倒序）
        return a.modifiedAt > b.modifiedAt;
    });
```

**理解 `a.pinned > b.pinned`：**

`bool` 类型中，`true` 等于 `1`，`false` 等于 `0`。所以：
- `a.pinned=true(1) > b.pinned=false(0)` → `1 > 0` → `true` → a 排前面 ✅
- `a.pinned=false(0) > b.pinned=true(1)` → `0 > 1` → `false` → b 排前面 ✅

---

## 十、QUuid — 唯一标识符生成

### 10.1 什么是 UUID？

UUID（Universally Unique Identifier，通用唯一标识符）是一个 128 位的数字，用于唯一标识某个对象。格式如：

```
550e8400-e29b-41d4-a716-446655440000
```

每次生成的 UUID 在全球范围内几乎不可能重复，因此非常适合作为便签的唯一 ID。

### 10.2 生成 UUID

```cpp
#include <QUuid>

// 生成带花括号的格式：{550e8400-e29b-41d4-a716-446655440000}
QString id1 = QUuid::createUuid().toString();

// 生成不带花括号的格式：550e8400-e29b-41d4-a716-446655440000
QString id2 = QUuid::createUuid().toString(QUuid::WithoutBraces);
```

二期使用 `QUuid::WithoutBraces` 格式，存入 JSON 时更简洁。

---

## 十一、QDateTime — 日期时间处理

### 11.1 获取当前时间

```cpp
QDateTime now = QDateTime::currentDateTime();  // 当前本地时间
QDate today = QDate::currentDate();            // 今天的日期
```

### 11.2 时间比较

`QDateTime` 支持直接用 `>`、`<`、`==` 比较：

```cpp
QDateTime dt1 = /* ... */;
QDateTime dt2 = /* ... */;

if (dt1 > dt2) {
    // dt1 比 dt2 更晚（更新）
}
```

### 11.3 时间格式化

```cpp
QDateTime dt = QDateTime::currentDateTime();

// 转为字符串（ISO 8601 格式，适合存储）
QString str = dt.toString(Qt::ISODate);
// 结果：2026-03-04T10:30:00

// 自定义格式
QString str2 = dt.toString("HH:mm");    // 10:30
QString str3 = dt.toString("M月d日");   // 3月4日
```

### 11.4 从字符串解析时间

```cpp
// 从 ISO 格式字符串解析
QDateTime dt = QDateTime::fromString("2026-03-04T10:30:00", Qt::ISODate);
```

### 11.5 日期加减

```cpp
QDate today = QDate::currentDate();
QDate yesterday = today.addDays(-1);  // 昨天
QDate nextWeek  = today.addDays(7);   // 下周
```

---

## 十二、QTextDocument — HTML 转纯文本

### 12.1 为什么需要过滤 HTML？

便签的 `content` 字段存储的是富文本 HTML，例如：

```html
<p><b>重要事项</b>：明天开会</p>
```

如果直接显示在卡片预览中，会看到 `<p><b>重要事项</b>：明天开会</p>` 这样的原始 HTML 标签，非常难看。

### 12.2 使用 QTextDocument 过滤

```cpp
QString NoteCardDelegate::stripHtml(const QString& html) {
    QTextDocument doc;
    doc.setHtml(html);          // 解析 HTML
    return doc.toPlainText();   // 返回纯文本（去掉所有标签）
}
```

**效果：**

```
输入：<p><b>重要事项</b>：明天开会</p>
输出：重要事项：明天开会
```

### 12.3 截取预览文字

```cpp
QString preview = stripHtml(content).left(50);
// .left(50) 取前 50 个字符（中文也算 1 个字符）
```

---

## 十三、原子文件替换

### 13.1 直接写入的风险

如果直接向目标文件写入数据：

```cpp
QFile file("notes.json");
file.open(QIODevice::WriteOnly | QIODevice::Truncate);
file.write(jsonData);  // 如果程序在这里崩溃...
file.close();
```

如果程序在写入过程中崩溃（断电、内存错误等），文件会处于"部分写入"的损坏状态，下次启动时无法读取，所有数据丢失。

### 13.2 原子替换策略

```
第一步：将完整数据写入临时文件 notes.json.tmp
第二步：删除旧文件 notes.json
第三步：将 notes.json.tmp 重命名为 notes.json
```

关键在于第三步的 `rename` 操作：在大多数操作系统上，同一磁盘分区内的文件重命名是**原子操作**，要么成功，要么失败，不存在中间状态。

```cpp
QString tmpPath = path + ".tmp";
QFile tmpFile(tmpPath);
tmpFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
tmpFile.write(jsonData);
tmpFile.close();

// 原子替换
QFile::remove(path);          // 删除旧文件
QFile::rename(tmpPath, path); // 重命名（原子操作）
```

### 13.3 确保目录存在

如果 `~/.stickynote/` 目录不存在，写文件会失败。需要先创建目录：

```cpp
QDir dir = QFileInfo(path).dir();  // 获取文件所在目录
if (!dir.exists()) {
    dir.mkpath(".");  // 递归创建目录（包括所有父目录）
}
```

`mkpath(".")` 中的 `"."` 表示当前目录（即 `dir` 本身），等价于 `dir.mkpath(dir.absolutePath())`。

---

## 十四、Qt 信号与槽（复习与进阶）

### 14.1 信号与槽的基本用法（复习）

```cpp
// 连接信号与槽
connect(sender, &SenderClass::signalName,
        receiver, &ReceiverClass::slotName);

// 发出信号
emit signalName(参数);
```

### 14.2 二期中的信号设计

`NoteManager` 定义了一个 `dataChanged()` 信号：

```cpp
signals:
    void dataChanged();
```

当数据发生变化时（增删改），`NoteManager` 发出这个信号，`NoteListModel` 收到后调用 `refresh()` 刷新界面：

```cpp
// 连接
connect(NoteManager::instance(), &NoteManager::dataChanged,
        note_model_, &NoteListModel::refresh);

// NoteManager 中发出信号
void NoteManager::addNote(const NoteData& note) {
    notes_.append(note);
    save();
    emit dataChanged();  // 通知所有监听者
}
```

### 14.3 信号与槽的优势

相比直接调用函数，信号与槽的优势：

- **解耦**：`NoteManager` 不需要知道 `NoteListModel` 的存在，只需发出信号
- **一对多**：一个信号可以连接多个槽，例如数据变化时同时刷新列表和侧边栏
- **跨线程**：Qt 的信号槽机制支持跨线程通信（二期暂不涉及）

---

## 十五、inline 变量（C++17）

### 15.1 传统的全局常量问题

在 C++17 之前，如果在头文件中定义全局变量：

```cpp
// global.h
const QStringList NOTE_COLORS = { "#FFEAA7", ... };  // ❌ 多个 .cpp 包含时会重复定义
```

每个包含 `global.h` 的 `.cpp` 文件都会生成一份 `NOTE_COLORS` 的副本，链接时报"重复定义"错误。

传统解决方案是在 `.cpp` 文件中定义，头文件中只声明 `extern`，比较繁琐。

### 15.2 C++17 的 inline 变量

C++17 引入了 `inline` 变量，允许在头文件中定义变量，编译器保证只有一份实例：

```cpp
// global.h
inline const QStringList NOTE_COLORS = {
    "#FFEAA7",  // 黄
    "#81ECEC",  // 青
    "#DFE6E9",  // 灰
    "#FAB1A0",  // 红
    "#A29BFE"   // 紫
};
```

这样任何 `.cpp` 文件都可以直接 `#include "global.h"` 使用 `NOTE_COLORS`，不会有重复定义问题。

### 15.3 inline 函数

同理，`inline` 函数也可以在头文件中定义：

```cpp
// global.h
inline QString notesFilePath() {
    return QDir::homePath() + "/.stickynote/notes.json";
}
```

`inline` 函数的语义是"建议编译器内联展开"，但现代编译器会自行决定是否内联，`inline` 关键字在这里更重要的作用是**允许在头文件中定义函数而不产生重复定义错误**。

---

## 附录：二期涉及的 Qt 类速查

| Qt 类 | 头文件 | 用途 |
|-------|--------|------|
| `QAbstractListModel` | `<QAbstractListModel>` | 列表数据模型基类 |
| `QStyledItemDelegate` | `<QStyledItemDelegate>` | 自定义绘制委托基类 |
| `QListView` | `<QListView>` | 列表视图 |
| `QModelIndex` | `<QModelIndex>` | Model 中数据项的索引 |
| `QPainter` | `<QPainter>` | 2D 绘图引擎 |
| `QPainterPath` | `<QPainterPath>` | 复杂几何路径 |
| `QPen` | `<QPen>` | 画笔（描边） |
| `QBrush` | `<QBrush>` | 画刷（填充） |
| `QFont` | `<QFont>` | 字体 |
| `QFontMetrics` | `<QFontMetrics>` | 字体度量（文字宽高计算） |
| `QJsonDocument` | `<QJsonDocument>` | JSON 文档 |
| `QJsonObject` | `<QJsonObject>` | JSON 对象 `{}` |
| `QJsonArray` | `<QJsonArray>` | JSON 数组 `[]` |
| `QJsonValue` | `<QJsonValue>` | JSON 值 |
| `QJsonParseError` | `<QJsonParseError>` | JSON 解析错误 |
| `QVariant` | `<QVariant>` | 万能类型容器 |
| `QUuid` | `<QUuid>` | UUID 生成 |
| `QDateTime` | `<QDateTime>` | 日期时间 |
| `QDate` | `<QDate>` | 日期 |
| `QTextDocument` | `<QTextDocument>` | 富文本文档（用于 HTML 转纯文本） |
| `QFile` | `<QFile>` | 文件读写 |
| `QDir` | `<QDir>` | 目录操作 |
| `QFileInfo` | `<QFileInfo>` | 文件信息 |
| `QSettings` | `<QSettings>` | 应用配置持久化（一期已用） |

---

## 附录：二期涉及的 C++ 知识速查

| 知识点 | 说明 |
|--------|------|
| `struct` | 数据结构体，`NoteData` 用 `struct` 而非 `class`，因为所有成员默认 `public` |
| `static` 成员 | 属于类本身而非实例，单例的 `instance_` 和 `instance()` 都是 `static` |
| `const` 成员函数 | 函数末尾加 `const` 表示不修改成员变量，如 `rowCount() const` |
| `override` | 明确表示重写基类虚函数，编译器会检查签名是否匹配 |
| Lambda 表达式 | `[](参数) { 函数体 }` 匿名函数，用于排序比较器 |
| `std::stable_sort` | 稳定排序，保持相等元素的相对顺序 |
| `inline` 变量/函数 | C++17，允许在头文件中定义变量/函数而不产生重复定义 |
| `#pragma once` | 头文件保护，防止同一头文件被多次包含 |
| `Q_DECLARE_METATYPE` | Qt 宏，将自定义类型注册到元类型系统 |
| `Q_UNUSED` | Qt 宏，标记未使用的参数，消除编译器警告 |
