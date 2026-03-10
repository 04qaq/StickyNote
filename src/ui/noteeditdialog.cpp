#include "noteeditdialog.h"
#include "components/colorselector.h" // 颜色选择器组件
#include "core/notemanager.h"

#include <QCloseEvent>
#include <QMessageBox>
#include <QShortcut>
#include <QVBoxLayout>

#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFrame>

NoteEditDialog::NoteEditDialog(const NoteData& data, QWidget* parent)
	: QDialog(parent)
{
	initUI();
	connectSignals();
	loadData(data);
}

void NoteEditDialog::saveData() {
	data_.title = title_edit_->text().trimmed();
	data_.content = content_edit_->toHtml().trimmed();
	data_.category = category_combo_->currentText();
	data_.color = color_selector_->currentColor(); // 保存用户选择的颜色
	original_data_ = data_;
}

NoteData NoteEditDialog::result()
{
	return data_;

}

void NoteEditDialog::onSaveClicked()
{
	if (title_edit_->text().trimmed().isEmpty()) {
		QMessageBox::warning(this, "提示", "标题不能为空，请输入便签标题。");
		title_edit_->setFocus();
	}
	else {
		saveData();
		accept();
	}
}


void NoteEditDialog::onCancelClicked()
{
	reject();
}

void NoteEditDialog::initUI()
{
	setWindowTitle("编辑便签");
	setMinimumWidth(420);
	setModal(true);

	// ── 整体垂直布局 ──────────────────────────────────────────
	auto* root_layout = new QVBoxLayout(this);
	root_layout->setContentsMargins(24, 20, 24, 20);
	root_layout->setSpacing(16);

	// ── 标题输入 ──────────────────────────────────────────────
	auto* title_label = new QLabel("标题", this);
	title_label->setObjectName("fieldLabel");

	title_edit_ = new QLineEdit(this);
	title_edit_->setPlaceholderText("请输入便签标题…");
	title_edit_->setObjectName("titleEdit");

	root_layout->addWidget(title_label);
	root_layout->addWidget(title_edit_);

	// ── 富文本工具栏（加粗 / 斜体）────────────────────────────
	auto* toolbar_layout = new QHBoxLayout;
	toolbar_layout->setSpacing(6);

	bold_btn_ = new QPushButton("B", this);
	bold_btn_->setObjectName("formatBtn");
	bold_btn_->setCheckable(true);
	bold_btn_->setFixedSize(28, 28);
	bold_btn_->setToolTip("加粗 (Ctrl+B)");

	italic_btn_ = new QPushButton("I", this);
	italic_btn_->setObjectName("formatBtn");
	italic_btn_->setCheckable(true);
	italic_btn_->setFixedSize(28, 28);
	italic_btn_->setToolTip("斜体 (Ctrl+I)");

	toolbar_layout->addWidget(bold_btn_);
	toolbar_layout->addWidget(italic_btn_);
	toolbar_layout->addStretch();

	// ── 内容输入 ──────────────────────────────────────────────
	auto* content_label = new QLabel("内容", this);
	content_label->setObjectName("fieldLabel");

	content_edit_ = new QTextEdit(this);
	content_edit_->setPlaceholderText("记录你的想法…");
	content_edit_->setObjectName("contentEdit");
	content_edit_->setMinimumHeight(120);

	root_layout->addWidget(content_label);
	root_layout->addLayout(toolbar_layout);
	root_layout->addWidget(content_edit_);

	word_count_label_ = new QLabel("0 字", this);
	word_count_label_->setObjectName("wordCountLabel");
	word_count_label_->setAlignment(Qt::AlignRight);  // 右对齐，更美观
	root_layout->addWidget(word_count_label_);
	root_layout->setSpacing(8);  // 统一间距，避免负 margin 遮挡



	// ── 分类选择 ──────────────────────────────────────────────
	auto* category_label = new QLabel("分类", this);
	category_label->setObjectName("fieldLabel");

	category_combo_ = new QComboBox(this);
	category_combo_->setObjectName("categoryCombo");
	// 从 NoteManager 读取已有分类填充下拉框
	for (const QString& cat : NoteManager::instance()->categories()) {
		category_combo_->addItem(cat);
	}

	root_layout->addWidget(category_label);
	root_layout->addWidget(category_combo_);

	// ── 颜色选择器 ────────────────────────────────────────────
	// ColorSelector 是自定义 Widget，绘制圆形色块，点击选色
	auto* color_label = new QLabel("便签颜色", this);
	color_label->setObjectName("fieldLabel");

	color_selector_ = new ColorSelector(this);

	root_layout->addWidget(color_label);
	root_layout->addWidget(color_selector_);

	// ── 分割线 ────────────────────────────────────────────────

	auto* line = new QFrame(this);
	line->setFrameShape(QFrame::HLine);
	line->setObjectName("divider");
	root_layout->addWidget(line);

	// ── 底部按钮（取消 / 保存）────────────────────────────────
	auto* btn_layout = new QHBoxLayout;
	btn_layout->setSpacing(10);

	cancel_btn_ = new QPushButton("取消", this);
	cancel_btn_->setObjectName("cancelBtn");
	cancel_btn_->setFixedHeight(36);

	save_btn_ = new QPushButton("保存", this);
	save_btn_->setObjectName("saveBtn");
	save_btn_->setFixedHeight(36);
	save_btn_->setDefault(true);  // 回车键触发保存

	btn_layout->addStretch();
	btn_layout->addWidget(cancel_btn_);
	btn_layout->addWidget(save_btn_);

	root_layout->addLayout(btn_layout);


	// ── 样式表 ────────────────────────────────────────────────
	setStyleSheet(R"(
		NoteEditDialog {
			background-color: #FFFFFF;
		}
		QLabel#fieldLabel {
			font-size: 13px;
			font-weight: 600;
			color: #333333;
		}
		QLineEdit#titleEdit {
			border: 1px solid #D9D9D9;
			border-radius: 6px;
			padding: 6px 10px;
			font-size: 14px;
			color: #333333;
			background: #FAFAFA;
		}
		QLineEdit#titleEdit:focus {
			border-color: #1890FF;
			background: #FFFFFF;
		}
		QTextEdit#contentEdit {
			border: 1px solid #D9D9D9;
			border-radius: 6px;
			padding: 8px 10px;
			font-size: 13px;
			color: #333333;
			background: #FAFAFA;
		}
		QTextEdit#contentEdit:focus {
			border-color: #1890FF;
			background: #FFFFFF;
		}
		QComboBox#categoryCombo {
			border: 1px solid #D9D9D9;
			border-radius: 6px;
			padding: 5px 10px;
			font-size: 13px;
			color: #333333;
			background: #FAFAFA;
		}
		QComboBox#categoryCombo:focus {
			border-color: #1890FF;
		}
		QComboBox#categoryCombo QAbstractItemView {
			background-color: #FFFFFF;
			border: 1px solid #D9D9D9;
			border-radius: 4px;
			color: #333333;
			selection-background-color: #E6F7FF;
			selection-color: #1890FF;
			outline: none;
		}

		QPushButton#formatBtn {
			border: 1px solid #D9D9D9;
			border-radius: 4px;
			background: #FAFAFA;
			font-size: 13px;
			font-weight: bold;
			color: #555555;
		}
		QPushButton#formatBtn:hover {
			background: #E6F7FF;
			border-color: #1890FF;
			color: #1890FF;
		}
		QPushButton#formatBtn:checked {
			background: #1890FF;
			border-color: #1890FF;
			color: #FFFFFF;
		}
		QFrame#divider {
			color: #F0F0F0;
		}
		QLabel#wordCountLabel {
			font-size: 12px;
			color: #AAAAAA;
			margin-top: 0px;
		}

		QPushButton#cancelBtn {
			border: 1px solid #D9D9D9;
			border-radius: 6px;
			padding: 0 20px;
			font-size: 13px;
			background: #FFFFFF;
			color: #555555;
		}
		QPushButton#cancelBtn:hover {
			border-color: #1890FF;
			color: #1890FF;
		}
		QPushButton#saveBtn {
			border: none;
			border-radius: 6px;
			padding: 0 20px;
			font-size: 13px;
			background: #1890FF;
			color: #FFFFFF;
		}
		QPushButton#saveBtn:hover {
			background: #40A9FF;
		}
		QPushButton#saveBtn:pressed {
			background: #096DD9;
		}
	)");
}

void NoteEditDialog::connectSignals()
{
	connect(save_btn_, &QPushButton::clicked, this, &NoteEditDialog::onSaveClicked);
	connect(cancel_btn_, &QPushButton::clicked, this, &NoteEditDialog::onCancelClicked);

	// Ctrl+S 快捷键保存
	new QShortcut(QKeySequence::Save, this, [this]() { saveData(); });


	connect(content_edit_, &QTextEdit::textChanged, this, [this]() {
		word_count_label_->setText(QString::number(content_edit_->toPlainText().length()) + " 字");
		});

	connect(bold_btn_, &QPushButton::toggled, this, [this](bool on) {
		content_edit_->setFontWeight(on ? QFont::Bold : QFont::Normal);
		content_edit_->setFocus();
		});


	connect(italic_btn_, &QPushButton::toggled, this, [this](bool on) {

		content_edit_->setFontItalic(on);
		content_edit_->setFocus();
		});
}

void NoteEditDialog::loadData(const NoteData& data)
{
	data_ = data;
	original_data_ = data;
	title_edit_->setText(data_.title);
	content_edit_->setHtml(data.content);
	int idx = category_combo_->findText(data.category);
	if (idx >= 0) category_combo_->setCurrentIndex(idx);
	// 回显颜色：编辑已有便签时，把保存的颜色显示为选中状态
	color_selector_->setCurrentColor(data.color);
	word_count_label_->setText(QString::number(content_edit_->toPlainText().length()) + " 字");

}

bool NoteEditDialog::isModified() {
	QTextEdit tmp;
	tmp.setHtml(original_data_.content);
	QString original_content = tmp.toPlainText();
	return title_edit_->text().trimmed() != original_data_.title ||
		content_edit_->toPlainText() != original_content ||
		category_combo_->currentText() != original_data_.category ||
		color_selector_->currentColor() != original_data_.color;
}
void NoteEditDialog::closeEvent(QCloseEvent* event) {
	if (!isModified()) {
		QDialog::closeEvent(event);  // 无修改，直接关闭
		return;
	}

	QMessageBox mesgBox(this);
	mesgBox.setWindowTitle("未保存的修改");
	mesgBox.setText("你有未保存的修改，是否保存？");
	mesgBox.setIcon(QMessageBox::Warning);

	QPushButton* save_btn    = mesgBox.addButton("保存",   QMessageBox::AcceptRole);
	QPushButton* discard_btn = mesgBox.addButton("不保存", QMessageBox::DestructiveRole);

	mesgBox.exec();

	if (mesgBox.clickedButton() == save_btn) {
		// onSaveClicked 内部调用 accept()，对话框会自行关闭
		// 这里阻止 closeEvent 继续，避免重复关闭
		event->ignore();
		onSaveClicked();
	}
	else if (mesgBox.clickedButton() == discard_btn) {
		QDialog::closeEvent(event);  // 不保存，直接关闭
	}
	else {
		event->ignore();  // 取消：阻止关闭，回到编辑状态
	}
}
