#include <QApplication>
#include "MainWindow.h"
#include "SerialWorker.h"

int main(int argc, char **argv){

	QApplication app(argc, argv);
	qRegisterMetaType<Measurement>();

	SerialWorker serialWorker;
	MainWindow mainWindow;

	QObject::connect(&serialWorker, &SerialWorker::newMeasurement, &mainWindow, &MainWindow::updateDisplay);
	QObject::connect(&serialWorker, &SerialWorker::statusMessage, &mainWindow, &MainWindow::showStatusMessage);
	QObject::connect(&serialWorker, &SerialWorker::connectionChanged, &mainWindow, &MainWindow::setConnected);

	QObject::connect(&mainWindow, &MainWindow::connectRequested, &serialWorker, &SerialWorker::connectToPort);
	QObject::connect(&mainWindow, &MainWindow::disconnectRequested, &serialWorker, &SerialWorker::disconnectPort);

	mainWindow.show();

	return app.exec();
}
