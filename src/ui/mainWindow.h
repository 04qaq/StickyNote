#include <QWidget.h>


namespace Ui {
	class MainWindow :public QWidget
	{
		Q_OBJECT
	public:
		explicit MainWindow(QWidget* parent = nullptr);
		~MainWindow();

	private:
		void init();

	};
}