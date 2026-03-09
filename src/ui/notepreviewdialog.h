#pragma once
#include <QDialog>
#include "common/notedata.h"

class QLabel;
class QTextEdit;
class QPushButton;

// 便签只读预览对话框
// 职责：仅展示便签的标题和内容，不提供任何编辑功能
class NotePreviewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NotePreviewDialog(const NoteData& data, QWidget* parent = nullptr);

private:
    void initUI();
    void applyStyle();
    void loadData(const NoteData& data);

    NoteData     data_;
    QLabel*      title_label_  = nullptr;  // 显示标题
    QTextEdit*   content_view_ = nullptr;  // 显示内容（只读）
    QPushButton* close_btn_    = nullptr;  // 关闭按钮
};
