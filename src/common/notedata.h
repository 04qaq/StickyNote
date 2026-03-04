#pragma once
#include <QString>
#include <QDateTime>
#include <QJsonObject>

struct NoteData {
    QString   id;
    QString   title;
    QString   content;
    QString   category;
    QString   color;
    bool      pinned;       // 是否置顶，默认 false
    int       sortOrder;    // 手动排序序号，默认 0
    QDateTime createdAt;    // 创建时间
    QDateTime modifiedAt;   // 最后修改时间

    static NoteData fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;

    // 工厂方法：创建一条新便签，自动生成 id 和时间戳
    static NoteData createNew(const QString& title = "",
                              const QString& category = "未分类");
};