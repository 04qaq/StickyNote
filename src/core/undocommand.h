#pragma once
#include <QString>
#include <memory>

// ============================================================
// UndoCommand —— 撤销/重做命令的抽象基类
//   · 每个用户操作封装为一个 Command 对象
//   · execute() 执行操作，undo() 撤销操作
//   · 遵循 Command 设计模式
// ============================================================
class UndoCommand {
public:
    virtual ~UndoCommand() = default;

    virtual void    execute()           = 0;
    virtual void    undo()              = 0;
    virtual QString description() const = 0;
};

// 用 unique_ptr 管理命令对象的生命周期，避免手动 delete
using UndoCommandPtr = std::unique_ptr<UndoCommand>;
