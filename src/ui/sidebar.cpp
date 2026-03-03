#include "sidebar.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

Sidebar::Sidebar(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(140);
    initUI();
    applyStyle();
}

void Sidebar::initUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 8);
    layout->setSpacing(2);

    // 分类标题
    QLabel* header = new QLabel("分类", this);
    header->setObjectName("sidebarHeader");
    header->setContentsMargins(16, 4, 8, 4);
    layout->addWidget(header);

    // 内置分类列表
    const QStringList categories = {"全部", "工作", "生活", "学习"};
    for (const QString& cat : categories) {
        QPushButton* btn = new QPushButton(cat, this);
        btn->setObjectName("categoryButton");
        btn->setCheckable(true);
        btn->setFlat(true);
        connect(btn, &QPushButton::clicked, this, [this, cat]() {
            emit categorySelected(cat);
        });
        layout->addWidget(btn);
    }

    layout->addStretch();

    // 分隔线
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setObjectName("sidebarDivider");
    layout->addWidget(line);

    // 自定义分类按钮（占位）
    QPushButton* addBtn = new QPushButton("+ 自定义", this);
    addBtn->setObjectName("addCategoryButton");
    addBtn->setFlat(true);
    layout->addWidget(addBtn);
}

void Sidebar::applyStyle()
{
    setStyleSheet(R"(
        Sidebar {
            background-color: #EAECF0;
            border-right: 1px solid #D0D3D9;
        }

        QLabel#sidebarHeader {
            color: #888;
            font-size: 11px;
            font-weight: bold;
            letter-spacing: 1px;
        }

        QPushButton#categoryButton {
            text-align: left;
            padding: 8px 16px;
            color: #333;
            font-size: 13px;
            border: none;
            border-radius: 0;
            background: transparent;
        }
        QPushButton#categoryButton:hover {
            background-color: #D8DCE5;
        }
        QPushButton#categoryButton:checked {
            background-color: #1890FF;
            color: #ffffff;
            font-weight: bold;
        }

        QFrame#sidebarDivider {
            color: #D0D3D9;
            margin: 0 8px;
        }

        QPushButton#addCategoryButton {
            text-align: left;
            padding: 8px 16px;
            color: #1890FF;
            font-size: 13px;
            border: none;
            background: transparent;
        }
        QPushButton#addCategoryButton:hover {
            background-color: #D8DCE5;
        }
    )");
}
