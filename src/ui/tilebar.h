#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
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

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void initUI();
    void applyStyleSheet();

    QLabel      *title_label_    = nullptr;
    QPushButton *close_button_   = nullptr;
    QPushButton *minimize_button_ = nullptr;
    QPushButton *maximize_button_ = nullptr;
};