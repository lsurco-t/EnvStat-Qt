#pragma once
#include <QString>
#include <QObject>
#include <QSerialPort>
#include "app/Measurement.h"

class SerialWorker : public QObject {
	Q_OBJECT

	public:
		explicit SerialWorker(QObject *parent = nullptr);
		~SerialWorker() = default;

	public slots:
		void connectToPort(const QString &portName, qint32 baudRate);
		void disconnectPort();

	private:
		QSerialPort *serialport_;
		QByteArray buffer_;

	private slots:
		void readSerialData();
		void processLine(const QString &line);
		void handleError(QSerialPort::SerialPortError error);

	signals:
		void newMeasurement(const Measurement &measurement);
		void statusMessage(const QString &message);
		void connectionChanged(bool connected, const QString &portName);
};
