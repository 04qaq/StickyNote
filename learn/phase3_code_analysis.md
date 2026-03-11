# StickyNote Manager 三期功能代码实现与设计分析

本篇文档基于《便签管理器（StickyNote Manager）产品需求文档》中关于“三期：编辑功能与交互”的需求，对项目中的核心代码实现进行详细分析。我们将重点探讨为什么这样设计，以及为什么这样实现。

---

## 整体架构：MVC 模式的深度应用

在介绍具体功能前，有必要先说明一下支撑这些功能的底层架构：MVC（Model-View-Controller）。

项目中将核心逻辑拆分为：
*   **Model**: `NoteListModel`, `NoteFilterProxyModel`, 以及单例数据管理 `NoteManager`
*   **View**: `NoteListView`, `NoteEditDialog`, `MainWindow`
*   **Controller**: `NoteController`

**设计原因**：
将 Controller 从 MainWindow 或 View 中抽离出来，是典型的“瘦身”设计。如果让 MainWindow 承担所有的逻辑处理，代码会迅速膨胀，变得难以维护。`NoteController` 作为一个中转站，专门负责协调 View（发送信号）和 Model（数据更新）。这种**关注点分离（Separation of Concerns, SoC）**使得代码职责清晰，扩展性极强。

---

## 核心功能一：新建与编辑便签（NoteEditDialog）

**需求回顾**：新建和编辑需要使用同一个模态弹窗。支持富文本编辑、分类选择、颜色修改，以及“未保存时拦截退出”。

### 1. 弹窗复用设计

**代码实现**：
在 `NoteController` 中：
```cpp
void NoteController::onNewNoteRequested() {
    NoteData note; // 空的便签数据
    NoteEditDialog dialog(note, view_);
    if (dialog.exec() == QDialog::Accepted) {
        NoteManager::instance()->addNote(dialog.result());
        model_->refresh();
    }
}

void NoteController::onNoteEdited(const QModelIndex& proxyIndex) {
    // 获取被点击的真实数据索引
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    // 传入已有数据回填
    NoteEditDialog dialog(note, view_);
    if (dialog.exec() == QDialog::Accepted) {
        NoteManager::instance()->updateNote(dialog.result());
        model_->refresh();
    }
}
```

**为什么这么设计**：
*   **弹窗复用**：`NoteEditDialog` 的构造函数接收一个 `NoteData` 对象。如果是新建，就传入一个按默认值初始化的 `NoteData`；如果是编辑，就传入选中的那条便签的 `NoteData`。通过这种方式，完美复用了整个 UI 和逻辑代码，避免了 `NoteNewDialog` 和 `NoteUpdateDialog` 的代码冗余。
*   **数据闭环**：`NoteEditDialog` 内部维护了一份传入数据的拷贝，只有当用户点击“保存”时，才会通过 `result()` 返回最新的数据。Controller 拿到结果后再去更新底层 Model。如果用户点击“取消”，数据自然被丢弃，不会污染底层源数据。

### 2. 未保存修改的防丢拦截

**代码实现**：
在 `NoteEditDialog.cpp` 中：
```cpp
bool NoteEditDialog::isModified() {
    // 检测当前 UI 的值是否与传入时的 original_data_ 不同
    // ...
}

void NoteEditDialog::closeEvent(QCloseEvent* event) {
    if (!isModified()) {
        QDialog::closeEvent(event);  // 无修改，直接关闭
        return;
    }

    QMessageBox mesgBox(this);
    // ... 弹出二次确认框
    if (mesgBox.clickedButton() == save_btn) {
        event->ignore(); // 阻止默认关闭事件
        onSaveClicked(); // 走正常保存流程
    }
    // ...
}
```

**为什么这么实现**：
重写 `closeEvent` 是 Qt 中处理窗口关闭前置逻辑的标准做法。通过对比当前输入的值和初始传入的值 `isModified()`，系统智能判断是否需要打扰用户。当用户选择了“保存”时，调用 `event->ignore()` 非常关键，它告诉 Qt 框架：“我处理了关闭操作，请先不要强制销毁窗口”，随后再调用 `onSaveClicked` 执行正常的保存和 `accept()` 流程，保证了数据的安全。

---

## 核心功能二：删除与批量删除

**需求回顾**：右键菜单支持删除，支持 `Ctrl+Click` 多选后批量删除。

### 数据索引的映射机制

**代码实现**：
在 `NoteController.cpp` 中：
```cpp
void NoteController::onNotesDeletedMultiple(const QModelIndexList& proxyIndexes) {
    QStringList ids;
    for (const QModelIndex& proxyIndex : proxyIndexes) {
        // 【关键】将视图层的 proxyIndex 映射回底层真实数据的 sourceIndex
        QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
        NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
        ids.append(note.id);
    }
    NoteManager::instance()->removeNotes(ids);
    model_->refresh();
}
```

**为什么这么设计**：
这是一个非常典型且易错的 Model/View 架构问题。
在项目中，由于加入了分类筛选和搜索功能，`NoteListView` 显示的数据并不直接来自 `NoteListModel`，而是经过了 `NoteFilterProxyModel`（代理模型）的处理。
假设列表被过滤后，第一项显示的是原数据列表中的第十项。如果此时用户要求删除第一项（`proxyIndex.row() == 0`），如果不做映射直接去原数组删除第 0 项，就会导致误删。
因此，必须通过 `proxy_model_->mapToSource(proxyIndex)` 将“代理视图索引”还原为“真实数据索引”，才能安全地通过 `NoteManager` 删除准确的源数据。

---

## 核心功能三：搜索与分类过滤

**需求回顾**：搜索框输入即搜（包含标题和正文），点击侧边栏切换分类。

### ProxyModel 的“一箭双雕”

虽然搜索和分类看似是两个不同的 UI 组件触发的，但在底层架构上，它们殊途同归。

**代码实现**：
项目中实现了一个继承自 `QSortFilterProxyModel` 的类 `NoteFilterProxyModel`。
```cpp
// NoteFilterProxyModel.h
class NoteFilterProxyModel : public QSortFilterProxyModel {
    // ...
    void setKeyword(const QString& keyword);
    void setCategory(const QString& category);
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
};
```
当搜索框文本改变或分类点击时，调用对应的 `setKeyword` 或 `setCategory` 方法。在这些方法内部，会触发 `invalidateFilter()`。
随后 Qt 框架会自动回调 `filterAcceptsRow` 方法对每一行数据进行判定。在这个函数里，我们会综合判断当前行是否符合 `keyword` 并且符合 `category`，只有同时满足才返回 `true` 使其显示在界面上。

**为什么这么设计**：
*   **性能优异**：基于代理模型的过滤发生在数据和视图中间层。不需要对真实的底层数组进行增删操作，也不需要频繁重建 UI 卡片，只需要通知 View 重新渲染通过过滤的索引即可，速度极快。
*   **逻辑解耦**：无论是按关键字搜索，还是按分类筛选，底层过滤规则都被收拢在 `NoteFilterProxyModel` 中，后续如果需要增加“按时间过滤”，只需要在这个类里增加条件即可，非常方便扩展。

---

## 核心功能四：动态调整与即时反馈

**需求回顾**：右键菜单修改便签分类和颜色。

**代码实现**：
```cpp
void NoteController::onNoteColorChanged(const QModelIndex& proxyIndex, const QString& color) {
    QModelIndex sourceIndex = proxy_model_->mapToSource(proxyIndex);
    NoteData note = model_->data(sourceIndex, NoteDataRole).value<NoteData>();
    NoteManager::instance()->updateNoteColor(note.id, color);
    model_->refresh();
}
```

**为什么这么设计**：
*   **极致便捷**：用户无需双击打开繁重的 `NoteEditDialog`，直接在列表视图上通过右键即可完成轻量级属性修改。
*   **数据流向单向化**：用户的右键操作只是发送一个修改请求（带着索引和新颜色）给 `NoteController`。Controller 去找 `NoteManager` 修改数据源，然后粗暴但有效地调用 `model_->refresh()`。由 Model 层的变化驱动 View 层重新绘制卡片。这种单向数据流保证了状态的一致性，绝对不会出现“界面颜色变了，但底下数据没改”的 bug。

---

## 总结

在“三期：编辑功能与交互”的实现中，我们可以看到：
1.  **高度重视代码复用**：`NoteEditDialog` 统一处理新建和编辑。
2.  **严格遵循 Qt 核心机制**：大量使用信号与槽（Signals and Slots）来传递用户的交互意图，用 `QSortFilterProxyModel` 处理数据的呈现。
3.  **防呆防错设计**：如编辑对话框中的 `isModified` 检测机制，以及删除时绝对不能少的 `mapToSource` 映射机制。

这些设计决策共同确保了项目不仅仅是“能跑”，而且具备了优秀的结构、可维护性以及后续迭代的扩展空间。
