#pragma once
#include "../undocommand.h"
#include "../../common/notedata.h"

// ============================================================
// DeleteNoteCommand —— 删除便签命令
//   execute: 从 NoteManager 删除便签
//   undo:    将便签重新插入 NoteManager
// ============================================================
class DeleteNoteCommand : public UndoCommand {
public:
    explicit DeleteNoteCommand(const NoteData& note);

    void    execute() override;
    void    undo()    override;
    QString description() const override { return "删除便签"; }

private:
    NoteData note_;  // 保存被删除的便签数据，undo 时恢复
};
