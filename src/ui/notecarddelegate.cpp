#include "notecarddelegate.h"
#include "models/notelistmodel.h"
#include <QPainter>
#include <QPainterPath>
#include <QTextDocument>
#include <QDate>

NoteCardDelegate::NoteCardDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

void NoteCardDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // 1. 从 Model 获取数据
    QString   title      = index.data(NoteTitleRole).toString();
    QString   content    = index.data(NoteContentRole).toString();
    QString   category   = index.data(NoteCategoryRole).toString();
    QString   color      = index.data(NoteColorRole).toString();
    bool      pinned     = index.data(NotePinnedRole).toBool();
    QDateTime modifiedAt = index.data(NoteModifiedAtRole).toDateTime();

    // 2. 过滤 HTML，截取正文预览（最多 50 字）
    QString preview = stripHtml(content).left(50);

    // 3. 卡片区域（留出 6px 间距，避免卡片紧贴边缘）
    QRect cardRect = option.rect.adjusted(6, 6, -6, -6);

    // 4. 绘制卡片背景（圆角 8px）
    QPainterPath path;
    path.addRoundedRect(cardRect, 8, 8);
    painter->fillPath(path, QColor(color.isEmpty() ? "#FFEAA7" : color));

    // 4.1 选中状态：绘制高亮边框
    bool isSelected = option.state & QStyle::State_Selected;
    if (isSelected) {
        painter->setPen(QPen(QColor("#2D3436"), 2.5));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(path);
    }


    // 5. 绘制标题（14px bold，置顶时前缀 📌）
    QString titleText = pinned ? ("📌 " + title) : title;
    QFont titleFont = painter->font();
    titleFont.setPixelSize(14);
    titleFont.setBold(true);
    painter->setFont(titleFont);
    painter->setPen(QColor("#2D3436"));
    QRect titleRect = cardRect.adjusted(12, 10, -12, 0);
    titleRect.setHeight(20);
    painter->drawText(titleRect,
                      Qt::AlignLeft | Qt::AlignVCenter,
                      painter->fontMetrics().elidedText(titleText, Qt::ElideRight, titleRect.width()));

    // 6. 绘制正文预览（12px，灰色，最多两行）
    QFont previewFont = painter->font();
    previewFont.setPixelSize(12);
    previewFont.setBold(false);
    painter->setFont(previewFont);
    painter->setPen(QColor("#636E72"));
    QRect previewRect = cardRect.adjusted(12, 36, -12, 0);
    previewRect.setHeight(36);
    painter->drawText(previewRect,
                      Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                      preview);

    // 7. 绘制底部：分类标签（左）+ 修改时间（右）
    QFont footerFont = painter->font();
    footerFont.setPixelSize(10);
    painter->setFont(footerFont);
    QRect footerRect = cardRect.adjusted(12, 0, -12, -10);
    footerRect.setTop(cardRect.bottom() - 24);

    painter->setPen(QColor("#636E72"));
    painter->drawText(footerRect, Qt::AlignLeft | Qt::AlignVCenter, "🏷️ " + category);

    painter->setPen(QColor("#B2BEC3"));
    painter->drawText(footerRect, Qt::AlignRight | Qt::AlignVCenter, formatTime(modifiedAt));

    painter->restore();
}

QSize NoteCardDelegate::sizeHint(const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    // 固定卡片高度 110px，宽度由 View 的列宽决定
    return QSize(200, 110);
}

QString NoteCardDelegate::stripHtml(const QString& html)
{
    QTextDocument doc;
    doc.setHtml(html);
    return doc.toPlainText();
}

QString NoteCardDelegate::formatTime(const QDateTime& dt)
{
    QDate today = QDate::currentDate();
    QDate date  = dt.date();

    if (date == today)
        return "今天 " + dt.toString("HH:mm");
    else if (date == today.addDays(-1))
        return "昨天";
    else
        return date.toString("M月d日");
}
