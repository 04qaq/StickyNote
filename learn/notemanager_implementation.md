# NoteManager 完整实现

## notemanager.h

```cpp
#pragma once

#include <QList>
#include <QObject>
#include <QStringList>
#include "common/notedata.h"

class NoteManager : public QObject {
    Q_OBJECT
public:
    static NoteManager* instance();

    // 查询
    const QList<NoteData>& notes() const;
    NoteData* fromId(QString id);
    QStringList categories() const;

    // 增删改
    void addNote(const NoteData& note);
    void updateNote(const NoteData& note);
    void removeNote(const NoteData& note);

    // 文件读写
    void load();
    void save();

signals:
    void dataChanged();

private:
    explicit NoteManager(QObject* parent = nullptr);

    QList<NoteData>  notes_;
    QStringList      customCategories_;
};
```

---

## notemanager.cpp

```cpp
#include "notemanager.h"
#include "common/global.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

// ─── 单例 ────────────────────────────────────────────────
NoteManager* NoteManager::instance() {
    static NoteManager instance;
    return &instance;
}

NoteManager::NoteManager(QObject* parent) : QObject(parent) {}

// ─── 查询接口 ─────────────────────────────────────────────
const QList<NoteData>& NoteManager::notes() const {
    return notes_;
}

NoteData* NoteManager::fromId(QString id) {
    for (auto& note : notes_) {
        if (note.id == id) {
            return &note;
        }
    }
    return nullptr;
}

QStringList NoteManager::categories() const {
    QStringList result = BUILTIN_CATEGORIES;
    result += customCategories_;
    return result;
}

// ─── 增删改 ───────────────────────────────────────────────
void NoteManager::addNote(const NoteData& note) {
    notes_.append(note);
    save();
    emit dataChanged();
}

void NoteManager::updateNote(const NoteData& note) {
    for (NoteData& n : notes_) {
        if (n.id == note.id) {
            n = note;
            break;
        }
    }
    save();
    emit dataChanged();
}

void NoteManager::removeNote(const NoteData& note) {
    notes_.removeIf([&note](const NoteData& n) {
        return n.id == note.id;
    });
    save();
    emit dataChanged();
}

// ─── 加载 ─────────────────────────────────────────────────
void NoteManager::load() {
    QString path = notesFilePath();
    QFile file(path);

    // 文件不存在时以空列表启动
    if (!file.exists()) {
        qDebug() << "数据文件不存在，以空列表启动：" << path;
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开数据文件：" << path;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    // 解析 JSON
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "JSON 解析失败：" << error.errorString();
        return;
    }

    QJsonObject root = doc.object();

    // 加载便签列表
    QJsonArray notesArr = root["notes"].toArray();
    for (const QJsonValue& val : notesArr) {
        if (val.isObject()) {
            notes_.append(NoteData::fromJson(val.toObject()));
        }
    }

    // 加载自定义分类
    QJsonArray catsArr = root["customCategories"].toArray();
    for (const QJsonValue& val : catsArr) {
        QString cat = val.toString();
        if (!cat.isEmpty()) {
            customCategories_.append(cat);
        }
    }

    qDebug() << "加载便签数量：" << notes_.size();
}

// ─── 保存（原子替换）─────────────────────────────────────
void NoteManager::save() {
    QString path = notesFilePath();

    // 确保目录存在
    QDir dir = QFileInfo(path).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 构建 JSON 内容
    QJsonArray notesArr;
    for (const NoteData& note : notes_) {
        notesArr.append(note.toJson());
    }

    QJsonArray catsArr;
    for (const QString& cat : customCategories_) {
        catsArr.append(cat);
    }

    QJsonObject root;
    root["notes"]            = notesArr;
    root["customCategories"] = catsArr;

    QByteArray jsonData = QJsonDocument(root).toJson(QJsonDocument::Indented);

    // 写入临时文件
    QString tmpPath = path + ".tmp";
    QFile tmpFile(tmpPath);

    if (!tmpFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "无法写入临时文件：" << tmpPath;
        return;
    }

    tmpFile.write(jsonData);
    tmpFile.close();

    // 原子替换：删除旧文件 → 重命名临时文件
    QFile::remove(path);
    if (!QFile::rename(tmpPath, path)) {
        qWarning() << "文件重命名失败：" << tmpPath << " → " << path;
    }
}
```

---

## 代码说明

| 部分 | 说明 |
|------|------|
| **单例** | 用局部静态变量实现，线程安全，无需手动管理指针 |
| **load()** | 文件不存在 → 空列表启动；JSON 损坏 → 打印警告不崩溃 |
| **save()** | 先写 `.tmp` 临时文件，再原子替换，防止崩溃丢数据 |
| **增删改** | 统一模式：修改内存 → `save()` → `emit dataChanged()` |
| **categories()** | 内置分类 + 自定义分类合并返回 |

### 头文件关键点

- **移除了 `instance_` 成员变量** —— 局部静态单例不需要类成员指针
- **补充了 `dataChanged` 信号和 `categories()` 方法** —— `.cpp` 中用到了但头文件缺少声明
- **构造函数设为 `private`** —— 防止外部直接创建实例，强制使用单例
