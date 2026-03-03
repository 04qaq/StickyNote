#include "mainWindow.h"
MainWindow::MainWindow(QWidget* parent = nullptr) {
	init();
}

MainWindow::~MainWindow() {

}

MainWindow::init() {
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint);//设置窗口为顶级窗口且无边框
	setAttribute(Qt::WA_TranslucentBackground, false);//设置背景不透明
}