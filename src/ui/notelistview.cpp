#include "notelistview.h"

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
	setSelectionMode(QAbstractItemView::SingleSelection);

    // ── 3. 滚动条 ─────────────────────────────────────────────
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 隐藏横向滚动条
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);    // 纵向按需显示

    // ── 4. 样式 ───────────────────────────────────────────────
    // 背景透明：让父控件（content_widget_）的背景色透出
    // border: none：去掉 QListView 默认的边框线
    // selection-background-color: transparent：防止系统蓝色选中框
    //   覆盖 NoteCardDelegate 绘制的卡片背景
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
    // 三期实现：根据 indexAt(event->pos()) 获取当前项，弹出右键菜单
    // 二期暂不实现，调用父类默认行为（什么都不做）
    QListView::contextMenuEvent(event);
}