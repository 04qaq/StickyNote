#pragma once
#include "../undocommand.h"
#include "../../common/notedata.h"

// ============================================================
// EditNoteCommand —— 编辑便签命令
//   execute: 用 new_data 更新便签
//   undo:    用 old_data 恢复便签
// ============================================================
class EditNoteCommand : public UndoCommand {
public:
    EditNoteCommand(const NoteData& old_data, const NoteData& new_data);

    void    execute() override;
    void    undo()    override;
    QString description() const override { return "编辑便签"; }

private:
    NoteData old_data_;  // 撤销时恢复的旧数据
    NoteData new_data_;  // 执行时应用的新数据
};
