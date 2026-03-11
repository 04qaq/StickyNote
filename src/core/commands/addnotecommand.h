#pragma once
#include "../undocommand.h"
#include "../../common/notedata.h"

class NoteManager;

// ============================================================
// AddNoteCommand —— 新建便签命令
//   execute: 将便签写入 NoteManager
//   undo:    从 NoteManager 删除该便签
// ============================================================
class AddNoteCommand : public UndoCommand {
public:
    explicit AddNoteCommand(const NoteData& note);

    void    execute() override;
    void    undo()    override;
    QString description() const override { return "新建便签"; }

private:
    NoteData note_;  // 保存便签数据，undo 时用于恢复
};
