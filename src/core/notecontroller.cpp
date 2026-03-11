#include "notecontroller.h"
#include "notemanager.h"
#include "undostack.h"
#include "commands/addnotecommand.h"
#include "commands/deletenotecommand.h"
#include "commands/deletemultiplenotescommand.h"
#include "commands/editnotecommand.h"
#include "commands/pinnotecommand.h"
#include "commands/changecategorycommand.h"
#include "commands/changecolorcommand.h"
#include "models/notelistmodel.h"
#include "models/notefilterproxymodel.h"
#include "ui/notelistview.h"
#include "ui/noteeditdialog.h"
#include "ui/notepreviewdialog.h"
#include "ui/components/toastmanager.h"
#include "common/notedata.h"

#include <QDialog>

NoteController::NoteController(NoteListView*         view,
                               NoteListModel*        model,
                               NoteFilterProxyModel* proxyModel,
                               QObject*              parent)
    : QObject(parent)
    , view_(view)
    , model_(model)
    , proxy_model_(proxyModel)
{
    undo_stack_ = new UndoStack(this);

    // UndoStack 执行/撤销后，NoteManager 的数据已变更，刷新 Model
    connect(undo_stack_, &UndoStack::stackChanged, model_, &NoteListModel::refresh);
}

// ----------------------------------------------------------------
// 初始化数据：加载持久化数据，首次运行时插入演示便签
// ----------------------------------------------------------------
void NoteController::initData()
{
    NoteManager::instance()->load();

    if (NoteManager::instance()->notes().isEmpty()) {
        NoteManager::instance()->addNote(NoteData::createNew(
            "欢迎使用 Sticky Notes",
            "这是你的第一条便签！\n你可以在这里记录任何想法、待办事项或灵感。",
            "工作", "#FFEAA7"));
        NoteManager::instance()->addNote(NoteData::createNew(
            "今日计划",
            "1. 完成项目报告\n2. 回复邮件\n3. 下午 3 点开会",
            "工作", "#FAB1A0"));
        NoteManager::instance()->addNote(NoteData::createNew(
            "购物清单",
            "- 牛奶\n- 面包\n- 鸡蛋\n- 苹果",
            "生活", "#81ECEC"));
        NoteManager::instance()->addNote(NoteData::createNew(
            "Qt 学习笔记",
            "Model/View 架构要点：\n• QAbstractListModel 负责数据\n• QListView 负责展示\n• Delegate 负责绘制每一项",
            "学习", "#A29BFE"));
        NoteManager::instance()->addNote(NoteData::createNew(
            "读书摘录",
            "「代码是写给人读的，只是顺便让机器执行。」\n—— Harold Abelson",
            "学习", "#DFE6E9"));
    }

    model_->refresh();
}

// ----------------------------------------------------------------
// 新建便签：弹出编辑对话框，确认后通过 Command 写入数据层
// ----------------------------------------------------------------
void NoteController::onNewNoteRequested()
{
    NoteData note;
    NoteEditDialog dialog(note, view_);
    if (dialog.exec() == QDialog::Accepted) {
        auto cmd = std::make_unique<AddNoteCommand>(dialog.result());
        undo_stack_->push(std::move(cmd));
        ToastManager::instance()->show("便签已创建");
    }
}

// ----------------------------------------------------------------
// 双击便签：弹出预览对话框（只读，不产生 Command）
// ----------------------------------------------------------------
void NoteController::onNoteDoubleClicked(const QModelIndex& proxyIndex)
{
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NotePreviewDialog dialog(note, view_);
    dialog.exec();
}

// ----------------------------------------------------------------
// 删除便签：通过 Command 删除，支持撤销
// ----------------------------------------------------------------
void NoteController::onNoteDeleted(const QModelIndex& proxyIndex)
{
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    auto cmd = std::make_unique<DeleteNoteCommand>(note);
    undo_stack_->push(std::move(cmd));
    ToastManager::instance()->show("便签已删除（Ctrl+Z 可撤销）");
}

// ----------------------------------------------------------------
// 编辑便签：弹出编辑对话框，确认后通过 Command 更新数据层
// ----------------------------------------------------------------
void NoteController::onNoteEdited(const QModelIndex& proxyIndex)
{
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData old_note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NoteEditDialog dialog(old_note, view_);
    if (dialog.exec() == QDialog::Accepted) {
        auto cmd = std::make_unique<EditNoteCommand>(old_note, dialog.result());
        undo_stack_->push(std::move(cmd));
        ToastManager::instance()->show("便签已更新");
    }
}

// ----------------------------------------------------------------
// 切换置顶：通过 Command 切换，支持撤销
// ----------------------------------------------------------------
void NoteController::onNotePinToggled(const QModelIndex& proxyIndex)
{
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    auto cmd = std::make_unique<PinNoteCommand>(note.id, note.pinned);
    undo_stack_->push(std::move(cmd));
    ToastManager::instance()->show(note.pinned ? "已取消置顶" : "已置顶");
}

// ----------------------------------------------------------------
// 修改分类：通过 Command 修改，支持撤销
// ----------------------------------------------------------------
void NoteController::onNoteCategoryChanged(const QModelIndex& proxyIndex, const QString& category)
{
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    if (note.category == category) return;  // 未变化，不产生命令
    auto cmd = std::make_unique<ChangeCategoryCommand>(note.id, note.category, category);
    undo_stack_->push(std::move(cmd));
    ToastManager::instance()->show(QString("分类已改为「%1」").arg(category));
}

// ----------------------------------------------------------------
// 修改颜色：通过 Command 修改，支持撤销
// ----------------------------------------------------------------
void NoteController::onNoteColorChanged(const QModelIndex& proxyIndex, const QString& color)
{
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    if (note.color == color) return;  // 未变化，不产生命令
    auto cmd = std::make_unique<ChangeColorCommand>(note.id, note.color, color);
    undo_stack_->push(std::move(cmd));
    ToastManager::instance()->show("颜色已更新");
}

// ----------------------------------------------------------------
// 预览便签：以独立顶层窗口打开（不产生 Command）
// ----------------------------------------------------------------
void NoteController::onNotePreviewRequested(const QModelIndex& proxyIndex)
{
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NotePreviewDialog* dialog = new NotePreviewDialog(note, nullptr);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowFlag(Qt::Window);
    dialog->show();
}

// ----------------------------------------------------------------
// 批量删除：通过 Command 批量删除，支持撤销
// ----------------------------------------------------------------
void NoteController::onNotesDeletedMultiple(const QModelIndexList& proxyIndexes)
{
    QList<NoteData> notes;
    for (const QModelIndex& proxyIndex : proxyIndexes) {
        QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
        NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
        notes.append(note);
    }
    auto cmd = std::make_unique<DeleteMultipleNotesCommand>(notes);
    undo_stack_->push(std::move(cmd));
    ToastManager::instance()->show(
        QString("已删除 %1 条便签（Ctrl+Z 可撤销）").arg(notes.size()));
}