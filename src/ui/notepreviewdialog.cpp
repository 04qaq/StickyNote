#include "notepreviewdialog.h"

#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

NotePreviewDialog::NotePreviewDialog(const NoteData& data, QWidget* parent)
    : QDialog(parent), data_(data)
{
    // 去掉问号按钮，只保留关闭按钮
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle("便签预览");
    setMinimumSize(420, 320);
    resize(480, 380);

    initUI();
    loadData(data_);
    applyStyle();
}

void NotePreviewDialog::initUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(12);

    // ── 标题 ──────────────────────────────────────────────────
    title_label_ = new QLabel(this);
    title_label_->setObjectName("previewTitle");
    title_label_->setWordWrap(true);   // 超长标题自动换行
    mainLayout->addWidget(title_label_);

    // ── 分割线 ────────────────────────────────────────────────
    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setObjectName("previewDivider");
    mainLayout->addWidget(line);

    // ── 内容（只读）──────────────────────────────────────────
    content_view_ = new QTextEdit(this);
    content_view_->setObjectName("previewContent");
    content_view_->setReadOnly(true);               // 关键：设为只读
    content_view_->setFrameShape(QFrame::NoFrame);  // 去掉边框，更简洁
    mainLayout->addWidget(content_view_, 1);        // stretch=1，撑满剩余空间
}

void NotePreviewDialog::loadData(const NoteData& data)
{
    title_label_->setText(data.title.isEmpty() ? "（无标题）" : data.title);

    // 内容可能是富文本 HTML，也可能是纯文本，统一用 setHtml 显示
    if (data.content.trimmed().startsWith('<')) {
        content_view_->setHtml(data.content);
    } else {
        content_view_->setPlainText(data.content);
    }
}

void NotePreviewDialog::applyStyle()
{
    // 用便签自身的颜色作为对话框背景，增强视觉关联感
    QString bgColor = data_.color.isEmpty() ? "#FFFFFF" : data_.color;

    setStyleSheet(QString(R"(
        NotePreviewDialog {
            background-color: %1;
        }
        QLabel#previewTitle {
            font-size: 18px;
            font-weight: bold;
            color: #2C3E50;
            background: transparent;
        }
        QFrame#previewDivider {
            color: #D0D0D0;
        }
        QTextEdit#previewContent {
            font-size: 14px;
            color: #34495E;
            background: transparent;
            border: none;
        }
        QPushButton#previewCloseBtn {
            background-color: #2C3E50;
            color: #FFFFFF;
            border: none;
            border-radius: 6px;
            font-size: 13px;
        }
        QPushButton#previewCloseBtn:hover {
            background-color: #3D5166;
        }
        QPushButton#previewCloseBtn:pressed {
            background-color: #1A252F;
        }
    )").arg(bgColor));
}
