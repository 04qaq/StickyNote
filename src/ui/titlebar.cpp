#include "titlebar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QWindow>

TitleBar::TitleBar(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(36);
    initUI();
    applyStyle();
}

void TitleBar::setTitle(const QString& title)
{
    if (m_titleLabel)
        m_titleLabel->setText(title);
}

void TitleBar::updateMaxButton(bool isMaximized)
{
    if (m_maxButton)
        m_maxButton->setText(isMaximized ? "❐" : "□");
}

// ----------------------------------------------------------------
// 界面初始化
// ----------------------------------------------------------------
void TitleBar::initUI()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 8, 0);
    layout->setSpacing(4);

    // 标题文字
    m_titleLabel = new QLabel("便签管理器", this);
    m_titleLabel->setObjectName("titleLabel");
    layout->addWidget(m_titleLabel);
    layout->addStretch();

    // 最小化按钮
    m_minButton = new QPushButton("─", this);
    m_minButton->setObjectName("minButton");
    m_minButton->setFixedSize(28, 28);
    m_minButton->setToolTip("最小化");
    connect(m_minButton, &QPushButton::clicked, this, &TitleBar::minimizeRequested);
    layout->addWidget(m_minButton);

    // 最大化/还原按钮
    m_maxButton = new QPushButton("□", this);
    m_maxButton->setObjectName("maxButton");
    m_maxButton->setFixedSize(28, 28);
    m_maxButton->setToolTip("最大化");
    connect(m_maxButton, &QPushButton::clicked, this, &TitleBar::maximizeRequested);
    layout->addWidget(m_maxButton);

    // 关闭按钮
    m_closeButton = new QPushButton("✕", this);
    m_closeButton->setObjectName("closeButton");
    m_closeButton->setFixedSize(28, 28);
    m_closeButton->setToolTip("关闭");
    connect(m_closeButton, &QPushButton::clicked, this, &TitleBar::closeRequested);
    layout->addWidget(m_closeButton);
}

void TitleBar::applyStyle()
{
    setStyleSheet(R"(
        TitleBar {
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
            background-color: transparent;
            border: none;
            border-radius: 4px;
            font-size: 13px;
        }
        QPushButton#minButton:hover,
        QPushButton#maxButton:hover {
            background-color: rgba(255, 255, 255, 0.2);
        }
        QPushButton#minButton:pressed,
        QPushButton#maxButton:pressed {
            background-color: rgba(255, 255, 255, 0.1);
        }

        QPushButton#closeButton {
            color: #ffffff;
            background-color: transparent;
            border: none;
            border-radius: 4px;
            font-size: 13px;
        }
        QPushButton#closeButton:hover {
            background-color: #E53935;
        }
        QPushButton#closeButton:pressed {
            background-color: #C62828;
        }
    )");
}

// ----------------------------------------------------------------
// 鼠标事件：拖拽移动父窗口
// ----------------------------------------------------------------
void TitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->globalPosition().toPoint() - window()->frameGeometry().topLeft();
        m_isDragging = true;
        event->accept();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        // 最大化状态下拖拽先还原
        QWidget* w = window();
        if (w->isMaximized()) {
            w->showNormal();
            // 重新计算拖拽起始点，使窗口跟随鼠标
            m_dragStartPos = QPoint(w->width() / 2, height() / 2);
        }
        w->move(event->globalPosition().toPoint() - m_dragStartPos);
        event->accept();
    }
}

void TitleBar::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        event->accept();
    }
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit maximizeRequested();
        event->accept();
    }
}
