#pragma once

#include <QObject>
#include <QModelIndex>
#include "undostack.h"

class NoteListView;
class NoteListModel;
class NoteFilterProxyModel;

// ============================================================
// NoteController —— MVC 中的 Controller 层
//   · 接收 View 发出的用户操作信号
//   · 将操作封装为 Command 推入 UndoStack（支持撤销/重做）
//   · 操作完成后通过 ToastManager 显示轻提示
// ============================================================
class NoteController : public QObject
{
    Q_OBJECT

public:
    explicit NoteController(NoteListView*         view,
                            NoteListModel*        model,
                            NoteFilterProxyModel* proxyModel,
                            QObject*              parent = nullptr);

    void initData();

    // 提供给 MainWindow 连接按钮状态
    UndoStack* undoStack() const { return undo_stack_; }

public slots:
    void onNewNoteRequested();
    void onNoteDoubleClicked(const QModelIndex& proxyIndex);
    void onNoteDeleted(const QModelIndex& proxyIndex);
    void onNoteEdited(const QModelIndex& proxyIndex);
    void onNotePinToggled(const QModelIndex& proxyIndex);
    void onNoteCategoryChanged(const QModelIndex& proxyIndex, const QString& category);
    void onNoteColorChanged(const QModelIndex& proxyIndex, const QString& color);
    void onNotePreviewRequested(const QModelIndex& proxyIndex);
    void onNotesDeletedMultiple(const QModelIndexList& proxyIndexes);

private:
    NoteListView*         view_;
    NoteListModel*        model_;
    NoteFilterProxyModel* proxy_model_;
    UndoStack*            undo_stack_ = nullptr;
};
