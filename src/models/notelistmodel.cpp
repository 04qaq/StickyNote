#include "notelistmodel.h"
#include "core/notemanager.h"
#include <algorithm>


NoteListModel::NoteListModel(QObject* parent):
	QAbstractListModel(parent) {
	connect(NoteManager::instance(), &NoteManager::dataChanged, this, &NoteListModel::refresh);
}

int NoteListModel::rowCount(const QModelIndex& parent) const {
	if (parent.isValid()) return 0;
	return sortedNotes_.size();
}

QVariant NoteListModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid() || index.row() >= sortedNotes_.size())
		return QVariant();

	const NoteData& note = sortedNotes_[index.row()];
	switch (role) {
		case Qt::DisplayRole:
		case NoteTitleRole:      return note.title;
		case NoteIdRole:         return note.id;
		case NoteContentRole:    return note.content;
		case NoteCategoryRole:   return note.category;
		case NoteColorRole:      return note.color;
		case NotePinnedRole:     return note.pinned;
		case NoteSortOrderRole:  return note.sortOrder;
		case NoteModifiedAtRole: return note.modifiedAt;
		case NoteDataRole:       return QVariant::fromValue(note);
		default:                 return QVariant();
	}
}

// ── 拖拽排序：flags ────────────────────────────────────────────
Qt::ItemFlags NoteListModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags default_flags = QAbstractListModel::flags(index);
	if (index.isValid())
		return default_flags | Qt::ItemIsDragEnabled;   // 有效 item：可拖拽
	else
		return default_flags | Qt::ItemIsDropEnabled;   // 根节点：可接受放下
}

// ── 拖拽排序：支持的放下动作 ───────────────────────────────────
Qt::DropActions NoteListModel::supportedDropActions() const {
	return Qt::MoveAction;
}

// ── 拖拽排序：执行移动 ─────────────────────────────────────────
bool NoteListModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
							  const QModelIndex& destinationParent, int destinationChild) {
	// ── 置顶保护：禁止跨区域拖拽 ─────────────────────────────
	// 规则：置顶便签只能在置顶区域内排序，普通便签只能在普通区域内排序。
	// 实现方式：检查被拖拽的便签 和 目标插入位置两侧的便签，
	//           如果置顶状态不一致，则拒绝本次移动。
	if (sourceRow < 0 || sourceRow >= sortedNotes_.size()) return false;

	bool source_pinned = sortedNotes_[sourceRow].pinned;

	// destinationChild 是"插入到第 N 行之前"的语义
	// 目标位置的"邻居"是 destinationChild-1 和 destinationChild
	// 只要目标位置两侧有任何一个便签与被拖拽便签的置顶状态不同，就拒绝
	int total = sortedNotes_.size();

	// 计算目标位置左侧邻居（插入后的前一个）
	int left_neighbor  = destinationChild - 1;
	int right_neighbor = destinationChild;

	// 跳过自身（被拖拽的那张卡片）
	if (left_neighbor  == sourceRow) left_neighbor--;
	if (right_neighbor == sourceRow) right_neighbor++;

	// 检查左侧邻居
	if (left_neighbor >= 0 && left_neighbor < total) {
		if (sortedNotes_[left_neighbor].pinned != source_pinned) return false;
	}
	// 检查右侧邻居
	if (right_neighbor >= 0 && right_neighbor < total) {
		if (sortedNotes_[right_neighbor].pinned != source_pinned) return false;
	}

	// ── beginMoveRows 会验证参数合法性，非法时返回 false ──────
	if (!beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1,
					   destinationParent, destinationChild)) {
		return false;
	}

	// 执行数据移动
	// 注意：向下移动时 destinationChild > sourceRow，insert 位置需要 -1
	NoteData note = sortedNotes_.takeAt(sourceRow);
	int insert_at = (destinationChild > sourceRow) ? destinationChild - 1 : destinationChild;
	sortedNotes_.insert(insert_at, note);

	endMoveRows();

	// 更新 sortOrder 并持久化
	updateSortOrders();

	return true;
}


// ── 更新所有便签的 sortOrder 并保存 ───────────────────────────
void NoteListModel::updateSortOrders() {
	// 将 sortedNotes_ 中的顺序写回 NoteManager
	for (int i = 0; i < sortedNotes_.size(); ++i) {
		sortedNotes_[i].sortOrder = i;
		NoteData* note = NoteManager::instance()->findById(sortedNotes_[i].id);
		if (note) {
			note->sortOrder = i;
		}
	}
	NoteManager::instance()->save();
}

void NoteListModel::sortNotes() {
	QList<NoteData> notes = NoteManager::instance()->notes();
	std::stable_sort(notes.begin(), notes.end(), [](const NoteData& a, const NoteData& b) {
		if (a.pinned != b.pinned) {
			return a.pinned > b.pinned;
		}
		if (a.sortOrder != b.sortOrder) {
			return a.sortOrder < b.sortOrder;
		}
		return a.modifiedAt > b.modifiedAt;
	});
	sortedNotes_ = notes;
}

void NoteListModel::refresh() {
	beginResetModel();
	sortNotes();
	endResetModel();
}


