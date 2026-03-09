#pragma once
#include <QDialog>
#include "common/notedata.h"

class QLineEdit;
class QTextEdit;
class QComboBox;
class QLabel;
class QPushButton;
class ColorSelector; // 前向声明，避免在头文件中 include，减少编译依赖



class NoteEditDialog : public QDialog
{
    Q_OBJECT

public:
    NoteEditDialog(const NoteData& data, QWidget* parent = nullptr);

    NoteData result();

public slots:
    void onSaveClicked();
    void onCancelClicked();

protected:
    void initUI();
    void connectSignals();
    void loadData(const NoteData& data);
    bool isModified();
    void closeEvent(QCloseEvent* event) override;

private:
    void saveData();

private:
    NoteData data_;
    NoteData original_data_;
    QLineEdit* title_edit_ = nullptr;
    QTextEdit* content_edit_ = nullptr;
    QComboBox*     category_combo_ = nullptr;
    ColorSelector* color_selector_  = nullptr; // 颜色选择器组件
    QPushButton*   bold_btn_        = nullptr;
    QLabel* word_count_label_ = nullptr;

    QPushButton* italic_btn_ = nullptr;
    QPushButton* save_btn_ = nullptr;
    QPushButton* cancel_btn_ = nullptr;

};