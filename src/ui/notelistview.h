#pragma once
#include <QListView>

class NoteListView : public QListView {
  Q_OBJECT
 public:
  explicit NoteListView(QWidget *parent = nullptr);

protected:
	void contextMenuEvent(QContextMenuEvent* event) override;
};