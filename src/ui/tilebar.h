#pragma once

#include <QWidget>
#include <QTimer>

class QLabel;
class QPushButton;
class QLineEdit;
class QMouseEvent;


class TileBar : public QWidget {
    Q_OBJECT
public:
    explicit TileBar(QWidget *parent = nullptr);
    void updateMaximizeButton(bool isMaximized);

signals:
    void closeRequested();
    void minimizeRequested();
    void maximizeRequested();
    void searchTextChanged(const QString& text);  // 搜索框内容变化时发出
    void newNoteRequested();// 新建笔记请求


protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void initUI();
    void applyStyleSheet();

    QLabel      *title_label_     = nullptr;
    QLineEdit   *search_edit_      = nullptr;  // 搜索框
    QPushButton *close_button_    = nullptr;
    QPushButton *minimize_button_ = nullptr;
    QPushButton *maximize_button_ = nullptr;
    QPushButton *new_note_button_ = nullptr;
    QTimer      *search_debounce_timer_ = nullptr;  // 防抖定时器
};
