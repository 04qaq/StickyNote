#pragma once
#include <QSortFilterProxyModel>
#include "notelistmodel.h"



class NoteFilterProxyModel:public QSortFilterProxyModel {
    Q_OBJECT
public:
	NoteFilterProxyModel(QObject *parent = nullptr);

	// 设置过滤关键词，后续可以添加按照时间过滤
	void setKeyword(const QString& keyword);
	void setCategory(const QString& category);


protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;




private:
	QString keyword_;
	QString category_;
};