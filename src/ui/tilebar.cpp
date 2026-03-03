#include "tilebar.h"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMouseEvent>

TileBar::TileBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("TileBar");
    setFixedHeight(36);
    // 必须设置此属性，QWidget 子类才能正确响应样式表中的 background-color
    setAttribute(Qt::WA_StyledBackground, true);
    initUI();
    applyStyleSheet();

}


void TileBar::updateMaximizeButton(bool isMaximized)
{
    if (isMaximized) {
        maximize_button_->setText("❐");
        maximize_button_->setToolTip("还原");
    } else {
        maximize_button_->setText("□");
        maximize_button_->setToolTip("最大化");
    }
}

void TileBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    emit maximizeRequested();
}

void TileBar::initUI()

{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 4, 0);
    layout->setSpacing(0);

    // 标题
    title_label_ = new QLabel("Sticky Notes", this);
    title_label_->setObjectName("titleLabel");
    layout->addWidget(title_label_);

    layout->addStretch();

    // 最小化按钮
    minimize_button_ = new QPushButton("─", this);
    minimize_button_->setObjectName("minButton");
    minimize_button_->setFixedSize(46, 36);
    minimize_button_->setToolTip("最小化");
    layout->addWidget(minimize_button_);

    // 最大化按钮
    maximize_button_ = new QPushButton("□", this);
    maximize_button_->setObjectName("maxButton");
    maximize_button_->setFixedSize(46, 36);
    maximize_button_->setToolTip("最大化");
    layout->addWidget(maximize_button_);

    // 关闭按钮
    close_button_ = new QPushButton("✕", this);
    close_button_->setObjectName("closeButton");
    close_button_->setFixedSize(46, 36);
    close_button_->setToolTip("关闭");
    layout->addWidget(close_button_);

    // 连接信号
    connect(minimize_button_, &QPushButton::clicked, this, &TileBar::minimizeRequested);
    connect(maximize_button_, &QPushButton::clicked, this, &TileBar::maximizeRequested);
    connect(close_button_,    &QPushButton::clicked, this, &TileBar::closeRequested);
}

void TileBar::applyStyleSheet()
{
    setStyleSheet(R"(
        QWidget#TileBar {
            background-color: #1890FF;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
        }
        QLabel#titleLabel {
            color: #ffffff;
            font-size: 14px;
            font-weight: bold;
            background: transparent;
        }
        QPushButton#minButton,
        QPushButton#maxButton {
            color: #ffffff;
            background: transparent;
            border: none;
            font-size: 14px;
        }
        QPushButton#minButton:hover,
        QPushButton#maxButton:hover {
            background-color: rgba(255, 255, 255, 0.2);
        }
        QPushButton#closeButton {
            color: #ffffff;
            background: transparent;
            border: none;
            font-size: 13px;
            border-top-right-radius: 8px;
        }
        QPushButton#closeButton:hover {
            background-color: #E81123;
        }
    )");
}