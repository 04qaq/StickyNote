#pragma once
#include "../undocommand.h"
#include <QString>

// ============================================================
// ChangeColorCommand —— 更改颜色命令
//   execute: 将便签颜色改为 new_color
//   undo:    恢复为 old_color
// ============================================================
class ChangeColorCommand : public UndoCommand {
public:
    ChangeColorCommand(const QString& note_id,
                       const QString& old_color,
                       const QString& new_color);

    void    execute() override;
    void    undo()    override;
    QString description() const override { return "更改颜色"; }

private:
    QString note_id_;
    QString old_color_;
    QString new_color_;
};
