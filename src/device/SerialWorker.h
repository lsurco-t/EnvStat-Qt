#pragma once
#include <QString>
#include <QObject>
#include <QSerialPort>

class SerialWorker : public QObject {
	Q_OBJECT

	public:
		explicit SerialWorker(QObject *parent = nullptr);
		~SerialWorker() = default;
		void startListening();
	
	private:
		QSerialPort *SerialPort_;
		QByteArray buffer_;

	private slots:
		void readSerialData();
		void processLine(const QString &line);

	signals:
		void newDataParsed(float temperature, float humidity);
		void statusMessage(const QString &message);
};

