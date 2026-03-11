#include "notelistview.h"
#include "common/global.h"
#include "core/notemanager.h"
#include "models/notelistmodel.h"
#include "models/notefilterproxymodel.h"

#include <QMenu>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QMessageBox>
#include <QTimer>
#include <QScrollBar>

// ============================================================
// 构造函数
// ============================================================
NoteListView::NoteListView(QWidget* parent)
    : QListView(parent)
{
    setViewMode(QListView::IconMode);
    setSpacing(8);
    setFlow(QListView::LeftToRight);
    setWrapping(true);
    setResizeMode(QListView::Adjust);

    // ── 禁用 Qt 内置拖拽机制，完全由我们自己实现 ─────────────
    // 原因：Qt 内置的 InternalMove 在 IconMode 下行为不可靠，
    //       且无法实现半透明跟随和自定义插入线效果
    setDragEnabled(false);
    setAcceptDrops(false);
    setDragDropMode(QAbstractItemView::NoDragDrop);

    // 扩展多选：Ctrl+Click 多选，Shift+Click 范围选
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

    // ── 长按计时器 ────────────────────────────────────────────
    long_press_timer_ = new QTimer(this);
    long_press_timer_->setSingleShot(true);  // 只触发一次
    long_press_timer_->setInterval(LONG_PRESS_MS);
    connect(long_press_timer_, &QTimer::timeout, this, [this]() {
        long_press_ready_ = true;  // 长按时间到，允许拖拽
    });
}

// ============================================================
// 鼠标按下：记录起始位置，启动长按计时器
// ============================================================
void NoteListView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        press_pos_        = event->pos();
        long_press_ready_ = false;
        long_press_timer_->start();

        // 记录被按下的卡片索引（代理索引）
        drag_source_index_ = indexAt(event->pos());
    }

    // 先交给父类处理选中逻辑
    QListView::mousePressEvent(event);
}

// ============================================================
// 鼠标移动：判断是否进入拖拽状态，更新跟随位置
// ============================================================
void NoteListView::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        QListView::mouseMoveEvent(event);
        return;
    }

    // ── 条件：长按时间已到 + 移动距离超过阈值 + 有有效的源卡片 ──
    int dist = (event->pos() - press_pos_).manhattanLength();
    if (!is_dragging_ && long_press_ready_
        && dist > DRAG_THRESHOLD
        && drag_source_index_.isValid())
    {
        // 进入拖拽状态
        is_dragging_ = true;

        // 生成半透明预览图（只生成一次）
        drag_pixmap_ = generateDragPixmap(drag_source_index_);

        // 拖拽期间禁止选中变化，避免视觉干扰
        setSelectionMode(QAbstractItemView::NoSelection);
    }

    if (is_dragging_) {
        drag_current_pos_ = event->pos();

        // 计算当前悬停的目标位置（同时更新 drop_valid_）
        drop_target_index_ = calcDropTarget(event->pos());

        // 触发重绘，更新跟随图和插入线位置

        viewport()->update();
        return;  // 拖拽期间不走父类逻辑
    }

    QListView::mouseMoveEvent(event);
}

// ============================================================
// 鼠标释放：提交排序，清理拖拽状态
// ============================================================
void NoteListView::mouseReleaseEvent(QMouseEvent* event)
{
    long_press_timer_->stop();

    if (is_dragging_ && event->button() == Qt::LeftButton) {
        // 执行排序
        if (drag_source_index_.isValid()) {
            commitDrop(drag_source_index_, drop_target_index_);
        }

        // 清理拖拽状态
        is_dragging_       = false;
        long_press_ready_  = false;
        drag_source_index_ = QModelIndex();
        drop_target_index_ = QModelIndex();
        drop_valid_        = false;
        drag_pixmap_       = QPixmap();


        // 恢复选中模式
        setSelectionMode(QAbstractItemView::ExtendedSelection);

        viewport()->update();
        return;
    }

    QListView::mouseReleaseEvent(event);
}

// ============================================================
// paintEvent：在正常绘制之上叠加拖拽视觉效果
// ============================================================
void NoteListView::paintEvent(QPaintEvent* event)
{
    // 先让父类完成正常的列表绘制
    QListView::paintEvent(event);

    if (!is_dragging_) return;

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);

    // ── 1. 绘制插入线 ─────────────────────────────────────────
    // 只有目标位置合法时才绘制（跨区域时 drop_valid_=false，不绘制）
    if (drop_valid_) {
        QRect insert_rect;
        if (drop_target_index_.isValid()) {
            insert_rect = visualRect(drop_target_index_);
        } else if (model() && model()->rowCount() > 0) {
            // 目标是末尾：取最后一个卡片的位置
            QModelIndex last = model()->index(model()->rowCount() - 1, 0);
            insert_rect = visualRect(last);
        }

        if (!insert_rect.isNull()) {
            // 在目标卡片左侧绘制竖线（宽 3px，圆角，蓝色）
            int line_x     = insert_rect.left() - 4;
            int line_top   = insert_rect.top()  + 6;
            int line_bottom= insert_rect.bottom()- 6;

            // 绘制竖线
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor("#0984E3"));
            painter.drawRoundedRect(QRect(line_x, line_top, 3, line_bottom - line_top), 1.5, 1.5);

            // 绘制上下两个圆点（让插入线更明显）
            painter.drawEllipse(QPoint(line_x + 1, line_top),    5, 5);
            painter.drawEllipse(QPoint(line_x + 1, line_bottom), 5, 5);
        }
    }


    // ── 2. 绘制半透明跟随卡片 ─────────────────────────────────
    if (!drag_pixmap_.isNull()) {
        // 卡片中心跟随鼠标
        QPoint draw_pos = drag_current_pos_ - QPoint(drag_pixmap_.width() / 2,
                                                      drag_pixmap_.height() / 2);
        painter.setOpacity(0.75);
        painter.drawPixmap(draw_pos, drag_pixmap_);
        painter.setOpacity(1.0);
    }
}

// ============================================================
// generateDragPixmap：截取卡片区域，生成半透明预览图
// ============================================================
QPixmap NoteListView::generateDragPixmap(const QModelIndex& index)
{
    QRect rect = visualRect(index);
    if (rect.isEmpty()) return QPixmap();

    // 截取 viewport 上该卡片区域的像素
    QPixmap pixmap = viewport()->grab(rect);

    // 叠加半透明遮罩（让拖拽图看起来是"浮起来"的）
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    painter.fillRect(pixmap.rect(), QColor(0, 0, 0, 200));  // alpha=200，约 78% 不透明
    painter.end();

    return pixmap;
}

// ============================================================
// calcDropTarget：根据鼠标位置计算插入目标索引
//
// 逻辑：找到鼠标下方的卡片，如果鼠标在该卡片左半部分，
//       则插入到该卡片前面；右半部分则插入到该卡片后面（即下一张前面）
//
// 置顶保护：如果目标位置跨越了置顶/非置顶边界，返回无效索引
//           （不显示插入线，也不执行移动）
// ============================================================
QModelIndex NoteListView::calcDropTarget(const QPoint& pos)
{
    if (!model() || !drag_source_index_.isValid()) {
        drop_valid_ = false;
        return QModelIndex();
    }


    // 获取被拖拽卡片的置顶状态
    bool source_pinned = drag_source_index_.data(NotePinnedRole).toBool();

    // 先尝试直接命中某张卡片
    QModelIndex hit = indexAt(pos);
    if (hit.isValid()) {
        QRect rect = visualRect(hit);

        QModelIndex candidate;
        // 鼠标在卡片左半部分 → 插入到 hit 前面（返回 hit）
        // 鼠标在卡片右半部分 → 插入到 hit 后面（返回 hit+1）
        if (pos.x() < rect.center().x()) {
            candidate = hit;
        } else {
            int next_row = hit.row() + 1;
            if (next_row < model()->rowCount()) {
                candidate = model()->index(next_row, 0);
            } else {
                candidate = QModelIndex();  // 末尾
            }
        }

        // ── 置顶保护：检查目标位置两侧邻居的置顶状态 ─────────
        // candidate 表示"插入到第 candidate.row() 行之前"
        // 左侧邻居 = candidate.row() - 1，右侧邻居 = candidate.row()
        int insert_row = candidate.isValid() ? candidate.row() : model()->rowCount();
        int left_row   = insert_row - 1;
        int right_row  = insert_row;

        // 跳过被拖拽的卡片自身
        if (left_row  == drag_source_index_.row()) left_row--;
        if (right_row == drag_source_index_.row()) right_row++;

        if (left_row >= 0 && left_row < model()->rowCount()) {
            bool left_pinned = model()->index(left_row, 0).data(NotePinnedRole).toBool();
            if (left_pinned != source_pinned) {
                drop_valid_ = false;
                return QModelIndex();  // 跨区域，拒绝
            }
        }
        if (right_row >= 0 && right_row < model()->rowCount()) {
            bool right_pinned = model()->index(right_row, 0).data(NotePinnedRole).toBool();
            if (right_pinned != source_pinned) {
                drop_valid_ = false;
                return QModelIndex();  // 跨区域，拒绝
            }
        }

        drop_valid_ = true;
        return candidate;

    }

    // 没有命中任何卡片（鼠标在空白区域）
    // 检查末尾位置是否合法（末尾的左侧邻居必须与被拖拽卡片同类型）
    int total = model()->rowCount();
    if (total > 0) {
        int last_row = total - 1;
        if (last_row == drag_source_index_.row()) last_row--;
        if (last_row >= 0) {
            bool last_pinned = model()->index(last_row, 0).data(NotePinnedRole).toBool();
            if (last_pinned != source_pinned) {
                drop_valid_ = false;
                return QModelIndex();  // 跨区域，拒绝
            }
        }
    }

    drop_valid_ = true;
    return QModelIndex();  // 末尾

}


// ============================================================
// commitDrop：将 source 卡片移动到 target 前面
// ============================================================
void NoteListView::commitDrop(const QModelIndex& source, const QModelIndex& target)
{
    if (!source.isValid()) return;

    int from_row = source.row();
    int to_row   = target.isValid() ? target.row() : model()->rowCount();

    // 相同位置或移动到自身后面一格，不需要操作
    if (from_row == to_row || from_row + 1 == to_row) return;

    // ── 将代理索引映射到源模型索引 ───────────────────────────
    // 因为 View 使用的是 NoteFilterProxyModel，
    // 而 moveRows 需要操作源模型（NoteListModel）的行号
    auto* proxy = qobject_cast<NoteFilterProxyModel*>(model());
    if (!proxy) return;

    QModelIndex source_src_index = proxy->mapToSource(source);
    int source_src_row = source_src_index.row();

    // target 可能是无效索引（末尾），需要特殊处理
    int target_src_row;
    if (target.isValid()) {
        target_src_row = proxy->mapToSource(target).row();
    } else {
        target_src_row = proxy->sourceModel()->rowCount();
    }

    // 调用源模型的 moveRows
    proxy->sourceModel()->moveRows(
        QModelIndex(), source_src_row, 1,
        QModelIndex(), target_src_row
    );
}

// ============================================================
// contextMenuEvent：右键菜单
// ============================================================
void NoteListView::contextMenuEvent(QContextMenuEvent* event)
{
    // 拖拽期间不弹出右键菜单
    if (is_dragging_) return;

    QModelIndex index = indexAt(event->pos());
    if (!index.isValid()) return;
    setCurrentIndex(index);

    NoteData note = index.data(NoteDataRole).value<NoteData>();

    QMenu menu(this);

    QAction* editAction    = menu.addAction("📝  编辑");
    QAction* pinAction     = menu.addAction(note.pinned ? "📌  取消置顶" : "📌  置顶");

    menu.addSeparator();

    QMenu* categoryMenu = menu.addMenu("🏷️  更改分类");
    QStringList cats = NoteManager::instance()->categories();
    for (const QString& cat : cats) {
        if (cat == "全部") continue;
        QAction* catAction = categoryMenu->addAction(cat);
        catAction->setCheckable(true);
        catAction->setChecked(cat == note.category);
    }

    QMenu* colorMenu = menu.addMenu("🎨  更改颜色");
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
    QAction* previewAction = menu.addAction("🪟  独立窗口打开");
    menu.addSeparator();
    QAction* deleteAction  = menu.addAction("🗑️  删除");

    QAction* result = menu.exec(event->globalPos());
    if (!result) return;

    if (result == editAction) {
        emit editNoteRequested(index);
    } else if (result == pinAction) {
        emit pinToggleRequested(index);
    } else if (result == previewAction) {
        emit openPreviewRequested(index);
    } else if (result == deleteAction) {
        QMessageBox::StandardButton btn = QMessageBox::question(
            this, "确认删除",
            QString("确定要删除便签「%1」吗？").arg(
                note.title.isEmpty() ? "（无标题）" : note.title),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No
        );
        if (btn == QMessageBox::Yes) {
            emit deleteNoteRequested(index);
        }
    } else if (categoryMenu->actions().contains(result)) {
        emit changeCategoryRequested(index, result->text());
    } else if (colorMenu->actions().contains(result)) {
        for (const auto& [name, hex] : colorOptions) {
            if (name == result->text()) {
                emit changeColorRequested(index, hex);
                break;
            }
        }
    }
}

// ============================================================
// keyPressEvent：Delete 键批量删除
// ============================================================
void NoteListView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete) {
        QModelIndexList selected = selectedIndexes();
        if (selected.isEmpty()) return;

        QString msg = selected.size() == 1
            ? QString("确定要删除便签「%1」吗？").arg(
                selected.first().data(NoteTitleRole).toString())
            : QString("确定要删除选中的 %1 条便签吗？").arg(selected.size());

        QMessageBox::StandardButton btn = QMessageBox::question(
            this, "确认删除", msg,
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No
        );
        if (btn == QMessageBox::Yes) {
            emit deleteMultipleRequested(selected);
        }
        return;
    }
    QListView::keyPressEvent(event);
}