#include <QApplication>
#include "MainWindow.h"
#include "SerialWorker.h"

int main(int argc, char **argv){

	QApplication app(argc, argv);

	SerialWorker serialWorker;
	MainWindow mainWindow;
	
	QObject::connect(&serialWorker, &SerialWorker::newDataParsed, &mainWindow, &MainWindow::updateDisplay);

	QObject::connect(&serialWorker, &SerialWorker::statusMessage, &mainWindow, &MainWindow::showStatusMessage);

	mainWindow.show();

	serialWorker.startListening();

	return app.exec();
}