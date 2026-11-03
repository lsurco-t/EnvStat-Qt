#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QTextEdit>

class MainWindow : public QMainWindow {
	Q_OBJECT

	public:
		explicit MainWindow(QWidget *parent = nullptr);
	
	public slots:
		void updateDisplay(float temperature, float humidity);
		void showStatusMessage(const QString &message);
	
	private:
		QLabel *temperatureLabel_;
		QLabel *humidityLabel_;
		QTextEdit *statusConsole_;
};