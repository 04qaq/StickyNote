#include "notemanager.h"
#include "common/global.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

NoteManager::NoteManager(QObject* parent) : QObject(parent) {}

NoteManager* NoteManager::instance() {
	static NoteManager instance;
	return &instance;
}


void NoteManager::addNote(const NoteData& note) {
	notes_.append(note);
	save();

	emit dataChanged();
}
void NoteManager::updateNote(const NoteData& note) {
	for(auto& n : notes_) {
		if (n.id == note.id) {
			n = note;
			break;
		}
	}
	save();

	emit dataChanged();

}
void NoteManager::removeNote(const QString& id) {
    for (auto it = notes_.begin(); it != notes_.end(); ++it) {
        if (it->id == id) {
            notes_.erase(it);
            break;
        }
    }
    save();
    emit dataChanged();
}


const QList<NoteData>& NoteManager::notes() const {
	return notes_;

}
NoteData* NoteManager::findById(const QString& id) {
	for (auto& note : notes_) {
		if (note.id == id) {
			return &note;
		}
	}
	return nullptr;
}

QStringList NoteManager::categories() const {
	QStringList results = BUILTIN_CATEGORIES;
	results += customCategories_;
	return results;
}


void NoteManager::load() {
	QString filePath = notesFilePath();
	QFile file(filePath);

	if(!file.exists()) {
		qDebug()<<"File not exists"<<filePath;
		return;
	}

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qWarning()<<"Failed to open file"<<filePath;
		return;
	}

	QByteArray data = file.readAll();
	file.close();

	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(data,&error);

	if(error.error != QJsonParseError::NoError) {
		qWarning()<<"Failed to parse json"<<filePath;
		return;
	}

	QJsonObject root = doc.object();
	QJsonArray notesArry = root["notes"].toArray();

	notes_.clear();
	for (const auto& note : notesArry) {
		if(note.isObject()) {
			notes_.append(NoteData::fromJson(note.toObject()));
		}
	}

	QJsonArray categoriesArry = root["customCategories"].toArray();

	customCategories_.clear();
	for (const auto& category : categoriesArry) {
		if(category.isString()) {
			customCategories_.append(category.toString());
		}
	}

	qDebug()<<"Load notes"<<notes_.size();

}
void NoteManager::save() {
	QString filePath = notesFilePath();
	QDir dir = QFileInfo(filePath).dir();

	if (!dir.exists()) {
		dir.mkpath(".");
	}

	QJsonArray notesArray;
	for (auto note : notes_) {
		notesArray.append(note.toJson());
	}

	QJsonArray categoriesArray;
	for (const QString&  category : customCategories_) {
		categoriesArray.append(category);
	}

	QJsonObject root;
	root["notes"] = notesArray;
	root["customCategories"] = categoriesArray;


	QByteArray jsonData = QJsonDocument(root).toJson();

	// 写入临时文件
	QString tmpPath = filePath + ".tmp";
	QFile tmpFile(tmpPath);

	if (!tmpFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		qWarning() << "无法写入临时文件：" << tmpPath;
		return;
	}

	tmpFile.write(jsonData);
	tmpFile.close();

	// 原子替换：删除旧文件 → 重命名临时文件
	QFile::remove(filePath);
	if (!QFile::rename(tmpPath, filePath)) {
		qWarning() << "文件重命名失败：" << tmpPath << " → " << filePath;
	}
}