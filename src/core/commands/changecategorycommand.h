#pragma once
#include "../undocommand.h"
#include <QString>

// ============================================================
// ChangeCategoryCommand —— 更改分类命令
//   execute: 将便签分类改为 new_category
//   undo:    恢复为 old_category
// ============================================================
class ChangeCategoryCommand : public UndoCommand {
public:
    ChangeCategoryCommand(const QString& note_id,
                          const QString& old_category,
                          const QString& new_category);

    void    execute() override;
    void    undo()    override;
    QString description() const override { return "更改分类"; }

private:
    QString note_id_;
    QString old_category_;
    QString new_category_;
};
