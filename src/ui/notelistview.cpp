#include "notelistview.h"
#include "common/global.h"
#include "core/notemanager.h"
#include "models/notelistmodel.h"
#include <QMenu>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMessageBox>

NoteListView::NoteListView(QWidget* parent):
	QListView(parent)
{
	setViewMode(QListView::IconMode);
	setSpacing(8);
	setFlow(QListView::LeftToRight);
	setWrapping(true);
	setResizeMode(QListView::Adjust);

	setAcceptDrops(false);
	setDragEnabled(false);
	// 改为扩展多选：Ctrl+Click 多选，Shift+Click 范围选
	setSelectionMode(QAbstractItemView::ExtendedSelection);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    setStyleSheet(
        "QListView {"
        "  background: transparent;"
        "  border: none;"
        "  outline: none;"
        "}"
        "QListView::item:selected {"
        "  background: transparent;"
        "}"
    );
}

void NoteListView::contextMenuEvent(QContextMenuEvent* event) {
    QModelIndex index = indexAt(event->pos());
    if (!index.isValid()) return;
    setCurrentIndex(index);

    // 获取当前便签数据
    NoteData note = index.data(NoteDataRole).value<NoteData>();

    QMenu menu(this);

    // ── 编辑 ──────────────────────────────────────────────────
    QAction* editAction = menu.addAction("📝  编辑");

    // ── 置顶 / 取消置顶 ───────────────────────────────────────
    QAction* pinAction = menu.addAction(note.pinned ? "📌  取消置顶" : "📌  置顶");

    menu.addSeparator();

    // ── 更改分类（子菜单）────────────────────────────────────
    QMenu* categoryMenu = menu.addMenu("🏷️  更改分类");
    QStringList cats = NoteManager::instance()->categories();
    for (const QString& cat : cats) {
        if (cat == "全部") continue;  // "全部" 不是真实分类，跳过
        QAction* catAction = categoryMenu->addAction(cat);
        // 当前分类打勾标记
        catAction->setCheckable(true);
        catAction->setChecked(cat == note.category);
    }

    // ── 更改颜色（子菜单）────────────────────────────────────
    QMenu* colorMenu = menu.addMenu("🎨  更改颜色");
    // 颜色名称映射
    QList<QPair<QString, QString>> colorOptions = {
        {"🟡  黄色", "#FFEAA7"},
        {"🩵  青色", "#81ECEC"},
        {"⚪  灰色", "#DFE6E9"},
        {"🔴  红色", "#FAB1A0"},
        {"🟣  紫色", "#A29BFE"},
    };
    for (const auto& [name, hex] : colorOptions) {
        QAction* colorAction = colorMenu->addAction(name);
        colorAction->setCheckable(true);
        colorAction->setChecked(hex == note.color);
    }

    menu.addSeparator();

    // ── 独立窗口打开 ──────────────────────────────────────────
    QAction* previewAction = menu.addAction("🪟  独立窗口打开");

    menu.addSeparator();

    // ── 删除 ──────────────────────────────────────────────────
    QAction* deleteAction = menu.addAction("🗑️  删除");

    // ── 执行菜单 ──────────────────────────────────────────────
    QAction* result = menu.exec(event->globalPos());
    if (!result) return;

    if (result == editAction) {
        emit editNoteRequested(index);
    }
    else if (result == pinAction) {
        emit pinToggleRequested(index);
    }
    else if (result == previewAction) {
        emit openPreviewRequested(index);
    }
    else if (result == deleteAction) {
        // 二次确认
        QMessageBox::StandardButton btn = QMessageBox::question(
            this, "确认删除",
            QString("确定要删除便签「%1」吗？").arg(note.title.isEmpty() ? "（无标题）" : note.title),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if (btn == QMessageBox::Yes) {
            emit deleteNoteRequested(index);
        }
    }
    // 更改分类
    else if (categoryMenu->actions().contains(result)) {
        emit changeCategoryRequested(index, result->text());
    }
    // 更改颜色
    else if (colorMenu->actions().contains(result)) {
        // 从颜色选项中找到对应的 hex
        for (const auto& [name, hex] : colorOptions) {
            if (name == result->text()) {
                emit changeColorRequested(index, hex);
                break;
            }
        }
    }
}

void NoteListView::keyPressEvent(QKeyEvent* event) {
    // Delete 键批量删除选中的便签
    if (event->key() == Qt::Key_Delete) {
        QModelIndexList selected = selectedIndexes();
        if (selected.isEmpty()) return;

        QString msg = selected.size() == 1
            ? QString("确定要删除便签「%1」吗？").arg(
                selected.first().data(NoteTitleRole).toString())
            : QString("确定要删除选中的 %1 条便签吗？").arg(selected.size());

        QMessageBox::StandardButton btn = QMessageBox::question(
            this, "确认删除", msg,
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if (btn == QMessageBox::Yes) {
            emit deleteMultipleRequested(selected);
        }
        return;
    }
    QListView::keyPressEvent(event);
}