#include "MainWindow.h"
#include <QDateTime>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	temperatureLabel_(new QLabel("Temperature: --")),
	humidityLabel_(new QLabel("Humidity: --")),
	statusConsole_(new QTextEdit)
{
	statusConsole_->setReadOnly(true);

	auto *layout = new QVBoxLayout;
	layout->addWidget(temperatureLabel_);
	layout->addWidget(humidityLabel_);
	layout->addWidget(statusConsole_);

	auto *centralWidget = new QWidget;
	centralWidget->setLayout(layout);
	setCentralWidget(centralWidget);

	setWindowTitle("EnvStat");
	resize(500, 300);
}

void MainWindow::updateDisplay(float temperature, float humidity){
	temperatureLabel_->setText(
		QString("Temperature: %1 °C").arg(temperature));

	humidityLabel_->setText(
		QString("Humidity: %1 %").arg(humidity));
}

void MainWindow::showStatusMessage(const QString &message){
	const QString timestamp = 
		QDateTime::currentDateTime().toString("HH:mm:ss");
	
	statusConsole_->append(
		QString("[%1] %2").arg(timestamp, message)
	);
}

