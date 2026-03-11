#pragma once
#include <QObject>
#include <vector>
#include "undocommand.h"


// ============================================================
// UndoStack —— 撤销/重做操作栈
//   · 维护两个栈：undo_stack_ 和 redo_stack_
//   · push() 执行命令并入栈，同时清空 redo 栈
//   · 栈深度限制为 MAX_STACK_SIZE（20步）
//   · 栈状态变化时发出 stackChanged 信号，驱动 UI 按钮状态
// ============================================================
class UndoStack : public QObject {
    Q_OBJECT
public:
    static constexpr int MAX_STACK_SIZE = 20;

    explicit UndoStack(QObject* parent = nullptr);

    // 执行命令并入栈（同时清空 redo 栈）
    void push(UndoCommandPtr cmd);

    void undo();
    void redo();

    bool canUndo() const { return !undo_stack_.empty(); }
    bool canRedo() const { return !redo_stack_.empty(); }


    // 清空所有历史（切换数据源时使用）
    void clear();

signals:
    // 栈状态变化时通知 UI 更新按钮状态（可用/禁用）
    void stackChanged();

private:
    // 使用 std::vector 存储 unique_ptr
    // 注意：QVector/QList 内部会尝试拷贝元素，而 unique_ptr 不可拷贝，必须用 std::vector
    std::vector<UndoCommandPtr> undo_stack_;
    std::vector<UndoCommandPtr> redo_stack_;

};
