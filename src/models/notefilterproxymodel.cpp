#include "notefilterproxymodel.h"

NoteFilterProxyModel::NoteFilterProxyModel(QObject* parent):
	QSortFilterProxyModel(parent) {
    category_ = "全部";
    keyword_ = "";

}

void NoteFilterProxyModel::setKeyword(const QString& keyword) {
	keyword_ = keyword;
	invalidateFilter();
}

void NoteFilterProxyModel::setCategory(const QString& category) {
    category_ = category;
    invalidateFilter();
}

bool NoteFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {

    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    QString category = index.data(NoteRole::NoteCategoryRole).toString();
    QString keyword = index.data(NoteRole::NoteContentRole).toString();
    QString title = index.data(NoteRole::NoteTitleRole).toString();

    return (category_ == "全部" || category_ == category) && 
        (keyword_.isEmpty() || 
         keyword.contains(keyword_, Qt::CaseInsensitive) || 
         title.contains(keyword_, Qt::CaseInsensitive));

}

