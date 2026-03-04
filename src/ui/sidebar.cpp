#include "sidebar.h"
#include "core/notemanager.h"
#include "common/global.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStyle>


SideBar::SideBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("SideBar");
    setFixedWidth(200);
    // 必须设置此属性，QWidget 子类才能正确响应样式表中的 background-color
    setAttribute(Qt::WA_StyledBackground, true);
    initUI();
    applyStyle();
    refreshCategories();
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
    category_layout_ = new QVBoxLayout(category_widget_);
    category_layout_->setContentsMargins(0, 8, 0, 8);
    category_layout_->setSpacing(2);
    category_layout_->addStretch();  // 底部弹性空间
    layout->addWidget(category_widget_, 1);

    // 设置按钮
    settings_button_ = new QPushButton("⚙  设置", this);
    settings_button_->setObjectName("settingsButton");
    settings_button_->setFixedHeight(40);
    layout->addWidget(settings_button_);
}

void SideBar::refreshCategories()
{
    // 清除旧按钮
    for (QPushButton* btn : category_buttons_) {
        category_layout_->removeWidget(btn);
        btn->deleteLater();
    }
    category_buttons_.clear();

    // 从 NoteManager 获取分类列表（内置 + 自定义）
    QStringList cats = NoteManager::instance()->categories();

    // 默认选中"全部"
    if (current_category_.isEmpty()) {
        current_category_ = cats.isEmpty() ? "全部" : cats.first();
    }

    // 在 stretch 之前插入按钮
    int insertPos = 0;
    for (const QString& cat : cats) {
        QPushButton* btn = new QPushButton(cat, category_widget_);
        btn->setObjectName("categoryButton");
        btn->setFixedHeight(36);
        btn->setCheckable(true);
        btn->setChecked(cat == current_category_);
        btn->setProperty("active", cat == current_category_);

        connect(btn, &QPushButton::clicked, this, [this, cat, btn]() {
            // 取消其他按钮的选中状态
            for (QPushButton* other : category_buttons_) {
                other->setChecked(other == btn);
                other->setProperty("active", other == btn);
                other->style()->unpolish(other);
                other->style()->polish(other);
            }
            current_category_ = cat;
            emit categoryChanged(cat);
        });

        category_layout_->insertWidget(insertPos++, btn);
        category_buttons_.append(btn);
    }
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
        QPushButton#categoryButton {
            color: #BDC3C7;
            background-color: transparent;
            border: none;
            border-radius: 4px;
            font-size: 13px;
            text-align: left;
            padding-left: 20px;
            margin: 0 8px;
        }
        QPushButton#categoryButton:hover {
            background-color: #3D5166;
            color: #ECF0F1;
        }
        QPushButton#categoryButton:checked {
            background-color: #3D5166;
            color: #FFFFFF;
            font-weight: bold;
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
