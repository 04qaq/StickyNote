#pragma once
#include <QList>
#include <QObject>
#include <QStringList>
#include "common/notedata.h"

class NoteManager : public QObject {
	Q_OBJECT
public:
	static NoteManager* instance();

	void addNote(const NoteData& note);
	void updateNote(const NoteData& note);
	void removeNote(const QString& id);
	void removeNotes(const QStringList& ids);  // 批量删除
	void togglePin(const QString& id);         // 切换置顶状态
	void updateNoteCategory(const QString& id, const QString& category);
	void updateNoteColor(const QString& id, const QString& color);


	void addCategory(const QString& category);
	void removeCategory(const QString& category);
	
	const QList<NoteData>& notes() const;
	NoteData* findById(const QString& id);
	QStringList categories() const;


	void load();
	void save();

signals:
	void dataChanged();
private:
	NoteManager(QObject* parent = nullptr);

	QList<NoteData> notes_;
	QStringList customCategories_;
};
