#pragma once
#include "common/notedata.h"
#include <QAbstractListModel>

enum NoteRole {
	NoteIdRole = Qt::UserRole + 1,
    NoteTitleRole,
    NoteContentRole,
    NoteCategoryRole,
    NoteColorRole,
    NotePinnedRole,
    NoteSortOrderRole,
    NoteModifiedAtRole,
    NoteDataRole,
};
class NoteListModel : public QAbstractListModel {
	Q_OBJECT
public:
	explicit NoteListModel(QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void refresh();

private:
	QList<NoteData> sortedNotes_;
    void sortNotes();
};

