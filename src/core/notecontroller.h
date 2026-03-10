#pragma once

#include <QObject>
#include <QModelIndex>

class NoteListView;
class NoteListModel;
class NoteFilterProxyModel;

// ============================================================
// NoteController —— MVC 中的 Controller 层
//   · 接收 View 发出的用户操作信号
//   · 调用 NoteManager（数据层）执行数据操作
//   · 通知 NoteListModel 刷新视图
// ============================================================
class NoteController : public QObject
{
    Q_OBJECT

public:
    // 构造时注入所有需要协调的对象
    explicit NoteController(NoteListView*         view,
                            NoteListModel*        model,
                            NoteFilterProxyModel* proxyModel,
                            QObject*              parent = nullptr);

    // 初始化数据：加载持久化数据，首次运行时插入演示便签
    void initData();

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
    // 将 proxyIndex 转换为 sourceIndex，并取出对应的 NoteData
    // 这是 Controller 中最常用的辅助操作，抽成私有方法避免重复
    NoteListView*         view_;
    NoteListModel*        model_;
    NoteFilterProxyModel* proxy_model_;
};
