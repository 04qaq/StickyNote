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

	int      rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // ── 拖拽排序所需接口 ──────────────────────────────────────
    Qt::ItemFlags    flags(const QModelIndex& index) const override;
    Qt::DropActions  supportedDropActions() const override;
    bool             moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                              const QModelIndex& destinationParent, int destinationChild) override;

public slots:
    void refresh();

private:
	QList<NoteData> sortedNotes_;
    void sortNotes();

    // 移动后更新所有便签的 sortOrder 并持久化
    void updateSortOrders();
};