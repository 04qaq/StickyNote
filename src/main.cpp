#include <QApplication>
#include "ui/mainwindow.h"

int main(int argc, char* argv[])
{
    // 创建 Qt 应用程序对象
    QApplication app(argc, argv);

    // 创建主窗口对象
    MainWindow window;

    window.show();
    // 进入应用程序主循环
    return app.exec();
}