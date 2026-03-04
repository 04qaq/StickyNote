#pragma once
#include <QWidget>
#include <QList>

class QLabel;
class QPushButton;
class QVBoxLayout;

class SideBar : public QWidget {
	Q_OBJECT
public:
	explicit SideBar(QWidget *parent = nullptr);

	// 从 NoteManager 重新加载分类列表，刷新按钮
	void refreshCategories();

signals:
	// 用户点击某个分类时发出，传递分类名称（"全部" 表示不过滤）
	void categoryChanged(const QString& category);

private:
	void initUI();
	void applyStyle();

	QLabel      *name_label_      = nullptr;
	QWidget     *category_widget_ = nullptr;
	QVBoxLayout *category_layout_ = nullptr;
	QPushButton *settings_button_ = nullptr;

	QList<QPushButton*> category_buttons_;  // 动态生成的分类按钮
	QString             current_category_;  // 当前选中的分类
};
