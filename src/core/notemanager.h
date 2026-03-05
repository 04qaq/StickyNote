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
