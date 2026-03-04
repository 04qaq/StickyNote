#include "notedata.h"
#include <QUuid>

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

NoteData NoteData::createNew(const QString& title, const QString& content,
                             const QString& category, const QString& color) {
    NoteData note;
    note.id         = QUuid::createUuid().toString(QUuid::WithoutBraces);
    note.title      = title;
    note.content    = content;
    note.category   = category;
    note.color      = color;

    note.pinned     = false;
    note.sortOrder  = 0;
    note.createdAt  = QDateTime::currentDateTime();
    note.modifiedAt = QDateTime::currentDateTime();
    return note;
}