#include "sidebar.h"
#include "core/notemanager.h"
#include "common/global.h"
#include "common/notedata.h"
#include <QLabel>

#include <QPushButton>
#include <QVBoxLayout>
#include <QStyle>
#include <QInputDialog>

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

    // 监听 NoteManager 数据变化，自动刷新分类列表
    // 为什么在这里连接？因为 addCategory/removeCategory 会 emit dataChanged()
    // SideBar 只需要监听这个信号，不需要关心是谁触发的
    connect(NoteManager::instance(), &NoteManager::dataChanged,
            this, &SideBar::refreshCategories);
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

    // 不指定父对象，由 addLayout 管理，避免重复设置布局的 Qt 警告
    QHBoxLayout* Hlayout = new QHBoxLayout();

    // 设置按钮
    settings_button_ = new QPushButton("⚙  设置", this);
    settings_button_->setObjectName("settingsButton");
    settings_button_->setFixedHeight(40);
    Hlayout->addWidget(settings_button_);

    // 新建分类按钮
    new_category_button_ = new QPushButton("➕ 新建分类", this);
    new_category_button_->setObjectName("newCategoryButton");
    new_category_button_->setFixedHeight(40);
    // 连接点击信号到槽函数，否则点击没有任何反应
    connect(new_category_button_, &QPushButton::clicked,
            this, &SideBar::onAddCategoryClicked);
    Hlayout->addWidget(new_category_button_);


    layout->addLayout(Hlayout);


}

void SideBar::refreshCategories()
{
    // 清除旧的行容器
    // takeAt(0) 把 item 从布局中取出并返回，我们负责 delete 它
    // 比 removeWidget 更彻底，能处理 widget、子 layout、spacer 所有类型
    while (QLayoutItem* item = category_layout_->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    category_buttons_.clear();

    // 重新加一个 stretch，保证分类按钮从顶部开始排列
    category_layout_->addStretch();


    // 从 NoteManager 获取分类列表（内置 + 自定义）
    QStringList cats = NoteManager::instance()->categories();

    // 默认选中"全部"
    if (current_category_.isEmpty()) {
        current_category_ = cats.isEmpty() ? "全部" : cats.first();
    }

    // 在 stretch 之前插入按钮（insertPos 从 0 开始，stretch 在末尾）
    int insertPos = 0;
    // 获取所有便签，用于计算各分类数量
    const QList<NoteData>& allNotes = NoteManager::instance()->notes();

    for (const QString& cat : cats) {
        // 计算该分类下的便签数量
        int count = 0;
        if (cat == "全部") {
            count = allNotes.size();
        } else {
            for (const NoteData& note : allNotes) {
                if (note.category == cat) ++count;
            }
        }
        // 分类名称后显示数量，如 "工作 (3)"
        QString displayName = count > 0
            ? QString("%1  (%2)").arg(cat).arg(count)
            : cat;

        // 每个分类用一行：[分类按钮] + [删除按钮（仅自定义分类有）]
        // 用 QWidget 容器包一层，是因为 QVBoxLayout 一个槽位只能放一个 widget
        // 我们需要横向排列两个按钮，所以用 QHBoxLayout 包裹
        QWidget* row_widget = new QWidget(category_widget_);
        QHBoxLayout* row_layout = new QHBoxLayout(row_widget);
        row_layout->setContentsMargins(0, 0, 0, 0);
        row_layout->setSpacing(0);

        QPushButton* btn = new QPushButton(displayName, row_widget);
        btn->setObjectName("categoryButton");
        btn->setFixedHeight(36);
        btn->setCheckable(true);
        btn->setChecked(cat == current_category_);



        connect(btn, &QPushButton::clicked, this, [this, cat, btn]() {
            // 取消其他按钮的选中状态
            for (QPushButton* other : category_buttons_) {
                other->setChecked(other == btn);
                other->style()->unpolish(other);
                other->style()->polish(other);

            }
            current_category_ = cat;
            emit categoryChanged(cat);
        });

        row_layout->addWidget(btn, 1);  // stretch=1，让分类按钮尽量占满宽度

        // 只有自定义分类才显示删除按钮，内置分类不允许删除
        if (!BUILTIN_CATEGORIES.contains(cat)) {
            QPushButton* del_btn = new QPushButton("×", row_widget);
            del_btn->setObjectName("deleteCategoryButton");
            del_btn->setFixedSize(24, 36);
            del_btn->setToolTip("删除分类 \"" + cat + "\"");
            // 用 lambda 捕获 cat，点击时知道要删哪个分类
            connect(del_btn, &QPushButton::clicked, this, [this, cat]() {
                onDeleteCategoryClicked(cat);
            });
            row_layout->addWidget(del_btn);
        }

        category_layout_->insertWidget(insertPos++, row_widget);
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
            margin: 0px 8px 0px 8px;

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
        QPushButton#newCategoryButton {
            color: #BDC3C7;
            background-color: transparent;
            border: none;
            border-top: 1px solid #3D5166;
            font-size: 13px;
            text-align: left;
            padding-left: 20px;
        }
        QPushButton#newCategoryButton:hover {
            background-color: #3D5166;
            color: #ECF0F1;
        }
        QPushButton#deleteCategoryButton {
            color: #7F8C8D;
            background-color: transparent;
            border: none;
            border-radius: 4px;
            font-size: 14px;
            padding: 0;
            margin-right: 8px;
        }
        QPushButton#deleteCategoryButton:hover {
            color: #E74C3C;
            background-color: #3D5166;
        }
    )");
}


void SideBar::onAddCategoryClicked() {
    bool ok;
    QString text = QInputDialog::getText(this, "新建分类", "请输入分类名: ", QLineEdit::Normal, "", &ok);
    if (ok && !text.trimmed().isEmpty()) {
        NoteManager::instance()->addCategory(text.trimmed());
    };
}
void SideBar::onDeleteCategoryClicked(const QString& categoryname) {
    NoteManager::instance()->removeCategory(categoryname);
}