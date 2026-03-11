#pragma once
#include <QListView>
#include <QPoint>
#include <QPixmap>
#include <QTimer>

// ============================================================
// NoteListView —— 便签列表视图
//
// 【自定义拖拽排序实现原理】
//
//  Qt 内置的 InternalMove 在 IconMode（网格布局）下行为不可靠，
//  因此我们完全绕过 Qt 的 MIME 拖拽机制，自己实现：
//
//  1. mousePressEvent  → 记录按下位置，启动长按计时器
//  2. mouseMoveEvent   → 超过拖拽阈值后进入拖拽状态，生成半透明预览图
//  3. paintEvent       → 在 View 上绘制：
//                          · 半透明卡片跟随鼠标
//                          · 目标位置的插入线
//  4. mouseReleaseEvent → 计算目标位置，调用 Model::moveRows 完成排序
//
//  关键数据：
//    drag_source_index_  被拖拽的卡片在代理模型中的索引
//    drag_pixmap_        拖拽时跟随鼠标的半透明卡片截图
//    drag_current_pos_   鼠标当前位置（用于绘制跟随图）
//    drop_target_index_  当前鼠标悬停的目标位置索引
// ============================================================
class NoteListView : public QListView {
    Q_OBJECT
public:
    explicit NoteListView(QWidget* parent = nullptr);

signals:
    void deleteNoteRequested(const QModelIndex& index);
    void editNoteRequested(const QModelIndex& index);
    void pinToggleRequested(const QModelIndex& index);
    void changeCategoryRequested(const QModelIndex& index, const QString& category);
    void changeColorRequested(const QModelIndex& index, const QString& color);
    void openPreviewRequested(const QModelIndex& index);
    void deleteMultipleRequested(const QModelIndexList& indexes);

protected:
    // ── 鼠标事件：驱动自定义拖拽状态机 ──────────────────────
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    // ── 绘制：叠加拖拽视觉效果（跟随图 + 插入线）────────────
    void paintEvent(QPaintEvent* event) override;

    // ── 其他事件 ──────────────────────────────────────────────
    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    // ── 生成拖拽预览图（卡片截图 + 半透明处理）───────────────
    QPixmap generateDragPixmap(const QModelIndex& index);

    // ── 根据鼠标位置计算插入目标索引 ─────────────────────────
    // 返回值：目标位置的代理索引（无效索引 + drop_valid_=true 表示末尾；
    //         无效索引 + drop_valid_=false 表示跨区域拒绝，不显示插入线）
    QModelIndex calcDropTarget(const QPoint& pos);


    // ── 执行排序：将 source 移动到 target 前面 ───────────────
    void commitDrop(const QModelIndex& source, const QModelIndex& target);

    // ── 拖拽状态 ──────────────────────────────────────────────
    bool        is_dragging_       = false;   // 是否处于拖拽状态
    QPoint      press_pos_;                   // 鼠标按下位置
    QModelIndex drag_source_index_;           // 被拖拽的卡片（代理索引）
    QPixmap     drag_pixmap_;                 // 跟随鼠标的半透明预览图
    QPoint      drag_current_pos_;            // 鼠标当前位置
    QModelIndex drop_target_index_;           // 当前悬停的目标位置（代理索引）
    bool        drop_valid_        = false;   // 当前目标位置是否合法（跨区域时为 false）


    // 长按计时器：避免普通点击误触发拖拽
    QTimer*     long_press_timer_  = nullptr;
    bool        long_press_ready_  = false;   // 长按时间已到，可以开始拖拽

    // 拖拽阈值：鼠标移动超过此距离才真正进入拖拽状态
    static constexpr int DRAG_THRESHOLD    = 8;   // px
    // 长按时间：按住多少毫秒后才允许拖拽
    static constexpr int LONG_PRESS_MS     = 150; // ms
};