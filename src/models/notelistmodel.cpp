#include "notelistmodel.h"
#include "core/notemanager.h"
#include <algorithm>

NoteListModel::NoteListModel(QObject* parent):
	QAbstractListModel(parent) {
	connect(NoteManager::instance(), &NoteManager::dataChanged, this, &NoteListModel::refresh);
}

int NoteListModel::rowCount(const QModelIndex& indx) const {
	if (indx.isValid())return 0;
	return sortedNotes_.size();

}
QVariant NoteListModel::data(const QModelIndex& index, int role) const {
	if(!index.isValid()||index.row()>=sortedNotes_.size())
		return QVariant();

	const NoteData& note = sortedNotes_[index.row()];
	switch (role) {
		case Qt::DisplayRole:
		case NoteTitleRole:      return note.title;
		case NoteIdRole:         return note.id;

		case NoteContentRole:    return note.content;
		case NoteCategoryRole:   return note.category;
		case NoteColorRole:      return note.color;
		case NotePinnedRole:     return note.pinned;
		case NoteSortOrderRole:  return note.sortOrder;
		case NoteModifiedAtRole: return note.modifiedAt;
		case NoteDataRole:       return QVariant::fromValue(note);
		default:                 return QVariant();
	}
}

void NoteListModel::sortNotes() {
	QList<NoteData> notes = NoteManager::instance()->notes();
	std::stable_sort(notes.begin(), notes.end(), [](const NoteData& a, const NoteData& b) {
		      
		if (a.pinned != b.pinned) {
			return a.pinned>b.pinned;
		}

		if (a.sortOrder != b.sortOrder) {
			return a.sortOrder<b.sortOrder;
		}

		return a.modifiedAt>b.modifiedAt;
	});
	sortedNotes_ = notes;
}

void NoteListModel::refresh() {
	beginResetModel();
	sortNotes();
	endResetModel();
}