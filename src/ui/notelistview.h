#pragma once
#include <QListView>

class NoteListView : public QListView {
  Q_OBJECT
 public:
  explicit NoteListView(QWidget *parent = nullptr);
signals:
	void deleteNoteRequested(const QModelIndex& index);
	void editNoteRequested(const QModelIndex& index);
	void pinToggleRequested(const QModelIndex& index);
	void changeCategoryRequested(const QModelIndex& index, const QString& category);
	void changeColorRequested(const QModelIndex& index, const QString& color);
	void openPreviewRequested(const QModelIndex& index);
	void deleteMultipleRequested(const QModelIndexList& indexes);
protected:
	void contextMenuEvent(QContextMenuEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
};