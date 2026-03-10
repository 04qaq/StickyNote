#include "notecontroller.h"
#include "notemanager.h"
#include "models/notelistmodel.h"
#include "models/notefilterproxymodel.h"
#include "ui/notelistview.h"
#include "ui/noteeditdialog.h"
#include "ui/notepreviewdialog.h"
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
// 新建便签：弹出编辑对话框，确认后写入数据层
// ----------------------------------------------------------------
void NoteController::onNewNoteRequested()
{
    NoteData note;
    NoteEditDialog dialog(note, view_);
    if (dialog.exec() == QDialog::Accepted) {
        NoteManager::instance()->addNote(dialog.result());
        model_->refresh();
    }
}

// ----------------------------------------------------------------
// 双击便签：弹出预览对话框（只读）
// ----------------------------------------------------------------
void NoteController::onNoteDoubleClicked(const QModelIndex& proxyIndex)
{
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NotePreviewDialog dialog(note, view_);
    dialog.exec();
}

// ----------------------------------------------------------------
// 删除便签：从数据层移除，刷新视图
// ----------------------------------------------------------------
void NoteController::onNoteDeleted(const QModelIndex& proxyIndex)
{
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NoteManager::instance()->removeNote(note.id);
    model_->refresh();
}

// ----------------------------------------------------------------
// 编辑便签：弹出编辑对话框，确认后更新数据层
// ----------------------------------------------------------------
void NoteController::onNoteEdited(const QModelIndex& proxyIndex)
{
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NoteEditDialog dialog(note, view_);
    if (dialog.exec() == QDialog::Accepted) {
        NoteManager::instance()->updateNote(dialog.result());
        model_->refresh();
    }
}

// ----------------------------------------------------------------
// 切换置顶：更新数据层，刷新视图
// ----------------------------------------------------------------
void NoteController::onNotePinToggled(const QModelIndex& proxyIndex)
{
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NoteManager::instance()->togglePin(note.id);
    model_->refresh();
}

// ----------------------------------------------------------------
// 修改分类：更新数据层，刷新视图
// ----------------------------------------------------------------
void NoteController::onNoteCategoryChanged(const QModelIndex& proxyIndex, const QString& category)
{
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NoteManager::instance()->updateNoteCategory(note.id, category);
    model_->refresh();
}

// ----------------------------------------------------------------
// 修改颜色：更新数据层，刷新视图
// ----------------------------------------------------------------
void NoteController::onNoteColorChanged(const QModelIndex& proxyIndex, const QString& color)
{
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NoteManager::instance()->updateNoteColor(note.id, color);
    model_->refresh();
}

// ----------------------------------------------------------------
// 预览便签：以独立顶层窗口打开预览对话框
// ----------------------------------------------------------------
void NoteController::onNotePreviewRequested(const QModelIndex& proxyIndex)
{
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NotePreviewDialog* dialog = new NotePreviewDialog(note, nullptr);  // parent=nullptr 使其独立
    dialog->setAttribute(Qt::WA_DeleteOnClose);  // 关闭时自动释放内存
    dialog->setWindowFlag(Qt::Window);           // 作为独立顶层窗口
    dialog->show();
}

// ----------------------------------------------------------------
// 批量删除：收集所有选中便签的 id，一次性从数据层移除
// ----------------------------------------------------------------
void NoteController::onNotesDeletedMultiple(const QModelIndexList& proxyIndexes)
{
    QStringList ids;
    for (const QModelIndex& proxyIndex : proxyIndexes) {
        QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
        NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
        ids.append(note.id);
    }
    NoteManager::instance()->removeNotes(ids);
    model_->refresh();
}
