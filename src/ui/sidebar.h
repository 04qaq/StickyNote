#pragma once
#include <QWidget>

class QLabel;
class QPushButton;

class SideBar : public QWidget {
	Q_OBJECT
public:
	explicit SideBar(QWidget *parent = nullptr);

private:
	void initUI();
	void applyStyle();

	QLabel      *name_label_      = nullptr;
	QWidget     *category_widget_ = nullptr;
	QPushButton *settings_button_ = nullptr;
};