#pragma once
#include "../undocommand.h"
#include "../../common/notedata.h"
#include <QStringList>

// ============================================================
// DeleteMultipleNotesCommand —— 批量删除便签命令
//   execute: 批量删除
//   undo:    逐条恢复
// ============================================================
class DeleteMultipleNotesCommand : public UndoCommand {
public:
    explicit DeleteMultipleNotesCommand(const QList<NoteData>& notes);

    void    execute() override;
    void    undo()    override;
    QString description() const override { return "批量删除便签"; }

private:
    QList<NoteData> notes_;  // 保存所有被删除的便签
};
