#pragma once
#include "../undocommand.h"
#include <QString>

// ============================================================
// PinNoteCommand —— 置顶/取消置顶命令
//   execute: 切换置顶状态
//   undo:    再次切换（恢复原状态）
// ============================================================
class PinNoteCommand : public UndoCommand {
public:
    PinNoteCommand(const QString& note_id, bool old_pinned);

    void    execute() override;
    void    undo()    override;
    QString description() const override { return "切换置顶"; }

private:
    QString note_id_;
    bool    old_pinned_;  // 操作前的置顶状态
};
