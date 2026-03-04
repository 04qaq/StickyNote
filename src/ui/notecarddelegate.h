#pragma once
#include <QStyledItemDelegate>
#include <QDateTime>

class NoteCardDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit NoteCardDelegate(QObject* parent = nullptr);

    // 绘制每张卡片
    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    // 返回卡片尺寸
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

private:
    // 过滤 HTML 标签，返回纯文本
    static QString stripHtml(const QString& html);

    // 格式化时间：今天/昨天/M月d日
    static QString formatTime(const QDateTime& dt);
};
