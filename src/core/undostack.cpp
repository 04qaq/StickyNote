#include "undostack.h"

UndoStack::UndoStack(QObject* parent)
    : QObject(parent)
{
}

void UndoStack::push(UndoCommandPtr cmd) {
    // 1. 执行命令
    cmd->execute();

    // 2. 入 undo 栈
    undo_stack_.push_back(std::move(cmd));

    // 3. 清空 redo 栈（新操作覆盖历史分支）
    redo_stack_.clear();

    // 4. 限制栈深度，移除最旧的操作（栈底）
    while (static_cast<int>(undo_stack_.size()) > MAX_STACK_SIZE) {
        undo_stack_.erase(undo_stack_.begin());
    }

    emit stackChanged();
}

void UndoStack::undo() {
    if (undo_stack_.empty()) return;

    // 从 undo 栈顶取出命令（move 出来，避免拷贝）
    UndoCommandPtr cmd = std::move(undo_stack_.back());
    undo_stack_.pop_back();

    // 执行撤销
    cmd->undo();

    // 移入 redo 栈
    redo_stack_.push_back(std::move(cmd));

    emit stackChanged();
}

void UndoStack::redo() {
    if (redo_stack_.empty()) return;

    // 从 redo 栈顶取出命令
    UndoCommandPtr cmd = std::move(redo_stack_.back());
    redo_stack_.pop_back();

    // 重新执行
    cmd->execute();

    // 移回 undo 栈
    undo_stack_.push_back(std::move(cmd));

    emit stackChanged();
}

void UndoStack::clear() {
    undo_stack_.clear();
    redo_stack_.clear();
    emit stackChanged();
}
