#include "tilebar.h"
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QTimer>



TileBar::TileBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("TileBar");
    setFixedHeight(36);
    // 必须设置此属性，QWidget 子类才能正确响应样式表中的 background-color
    setAttribute(Qt::WA_StyledBackground, true);

    // 初始化防抖定时器：300ms 后才真正触发搜索
    search_debounce_timer_ = new QTimer(this);
    search_debounce_timer_->setSingleShot(true);  // 单次触发，不循环
    search_debounce_timer_->setInterval(300);     // 300ms 防抖

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

    // 标题,后续可以替换成图标
    title_label_ = new QLabel("Sticky Notes", this);
    title_label_->setObjectName("titleLabel");
    layout->addWidget(title_label_);
    //新建按钮
    new_note_button_ = new QPushButton("＋ 新建", this);
    new_note_button_->setObjectName("newNoteButton");
    new_note_button_->setFixedHeight(24);
    new_note_button_->setToolTip("新建便签");
    connect(new_note_button_, &QPushButton::clicked, this, &TileBar::newNoteRequested);
    layout->addWidget(new_note_button_);

    layout->addStretch();

    // ---- 搜索框 ----
    search_edit_ = new QLineEdit(this);
    search_edit_->setObjectName("searchEdit");
    search_edit_->setPlaceholderText("🔍  搜索便签...");
    search_edit_->setFixedHeight(24);
    search_edit_->setMinimumWidth(160);
    search_edit_->setMaximumWidth(280);
    // 输入即搜：textChanged 触发防抖定时器，300ms 后发出 searchTextChanged 信号
    connect(search_edit_, &QLineEdit::textChanged,
        this, [this](const QString&) {
            search_debounce_timer_->start();  // 每次输入都重置定时器
        });
    // 定时器超时后才真正发出搜索信号
    connect(search_debounce_timer_, &QTimer::timeout,
        this, [this]() {
            emit searchTextChanged(search_edit_->text());
        });
    // 按回车立即搜索（不等防抖）
    connect(search_edit_, &QLineEdit::returnPressed,
        this, [this]() {
            search_debounce_timer_->stop();
            emit searchTextChanged(search_edit_->text());
        });
    layout->addWidget(search_edit_);


    layout->addSpacing(8);


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

        /* 搜索框：半透明白色背景，融入蓝色标题栏 */
        QLineEdit#searchEdit {
            background-color: rgba(255, 255, 255, 0.18);
            border: 1px solid rgba(255, 255, 255, 0.40);
            border-radius: 12px;
            color: #ffffff;
            font-size: 12px;
            padding: 0 10px 0 10px;
            selection-background-color: rgba(255, 255, 255, 0.35);
        }
        QLineEdit#searchEdit:focus {
            background-color: rgba(255, 255, 255, 0.28);
            border: 1px solid rgba(255, 255, 255, 0.75);
        }
        QLineEdit#searchEdit::placeholder {
            color: rgba(255, 255, 255, 0.65);
        }
    )");


}