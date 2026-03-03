#include "sidebar.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

SideBar::SideBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("SideBar");
    setFixedWidth(200);
    // 必须设置此属性，QWidget 子类才能正确响应样式表中的 background-color
    setAttribute(Qt::WA_StyledBackground, true);
    initUI();
    applyStyle();

}

void SideBar::initUI()

{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 应用名称标签
    name_label_ = new QLabel("Sticky Notes", this);
    name_label_->setFixedHeight(56);
    name_label_->setObjectName("appNameLabel");
    name_label_->setAlignment(Qt::AlignCenter);
    layout->addWidget(name_label_);


    // 分类区域
    category_widget_ = new QWidget(this);
    category_widget_->setObjectName("categoryArea");
    layout->addWidget(category_widget_, 1);

    // 设置按钮
    settings_button_ = new QPushButton("⚙  设置", this);
    settings_button_->setObjectName("settingsButton");
    settings_button_->setFixedHeight(40);
    layout->addWidget(settings_button_);
}

void SideBar::applyStyle()
{
    setStyleSheet(R"(
        QWidget#SideBar {
            background-color: #2C3E50;
            border-bottom-left-radius: 8px;
        }
        QLabel#appNameLabel {
            color: #ECF0F1;
            font-size: 15px;
            font-weight: bold;
            background-color: #243342;
        }
        QWidget#categoryArea {
            background-color: transparent;
        }
        QPushButton#settingsButton {
            color: #BDC3C7;
            background-color: transparent;
            border: none;
            border-top: 1px solid #3D5166;
            font-size: 13px;
            text-align: left;
            padding-left: 20px;
        }
        QPushButton#settingsButton:hover {
            background-color: #3D5166;
            color: #ECF0F1;
        }
    )");
}