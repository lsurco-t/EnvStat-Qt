#include "SerialWorker.h"
#include "TelemetryParser.h"
#include <QDateTime>
#include <QHash>
#include <optional>

SerialWorker::SerialWorker(QObject *parent) : QObject(parent){
	serialport_ = new QSerialPort(this);

	connect(serialport_, &QSerialPort::readyRead, this, &SerialWorker::readSerialData);
	connect(serialport_, &QSerialPort::errorOccurred, this, &SerialWorker::handleError);
}

void SerialWorker::connectToPort(const QString &portName, qint32 baudRate){
	if (serialport_->isOpen()){
		disconnectPort();
	}

	buffer_.clear();
	serialport_->setPortName(portName);
	serialport_->setBaudRate(baudRate);

	if (serialport_->open(QIODevice::ReadOnly)){
		emit statusMessage(
			QStringLiteral("Connected to %1 at %2 baud").arg(portName).arg(baudRate)
		);
		emit connectionChanged(true, portName);
	} else {
		emit statusMessage(
			QStringLiteral("Failed to connect to %1: %2")
				.arg(portName, serialport_->errorString())
		);
		emit connectionChanged(false, portName);
	}
}

void SerialWorker::disconnectPort(){
	if (!serialport_->isOpen()){
		return;
	}

	const QString portName = serialport_->portName();
	serialport_->close();
	buffer_.clear();

	emit statusMessage(QStringLiteral("Disconnected from %1").arg(portName));
	emit connectionChanged(false, portName);
}

void SerialWorker::handleError(QSerialPort::SerialPortError error){
	if (error == QSerialPort::NoError || !serialport_->isOpen()){
		return;
	}

	emit statusMessage(
		QStringLiteral("Serial port error: %1").arg(serialport_->errorString())
	);

	// A removed device cannot recover on its own; close so the UI can reconnect.
	if (error == QSerialPort::ResourceError ||
		error == QSerialPort::DeviceNotFoundError ||
		error == QSerialPort::PermissionError){
		const QString portName = serialport_->portName();
		serialport_->close();
		buffer_.clear();
		emit connectionChanged(false, portName);
	}
}

void SerialWorker::readSerialData(){
	buffer_ += serialport_->readAll();

	while(buffer_.contains('\n')){
		const int endIndex = buffer_.indexOf('\n');
		const QByteArray rawLine = buffer_.left(endIndex);
		buffer_.remove(0, endIndex + 1);

		processLine(QString::fromUtf8(rawLine).trimmed());
	}
}

static const QHash<QString, QString> statusMessages{
	{QStringLiteral("wifi_connecting"),
	 QStringLiteral("Wi-Fi connection in progress")},
	{QStringLiteral("wifi_connected"),
	 QStringLiteral("Wi-Fi connected")},
	{QStringLiteral("telegram_alert_sent"),
	 QStringLiteral("Telegram temperature alert sent")},
	{QStringLiteral("telegram_all_clear_sent"),
	 QStringLiteral("Telegram all-clear notification sent")},
	{QStringLiteral("wifi_connection_failed"),
	 QStringLiteral("Wi-Fi connection failed")},
	{QStringLiteral("dht_read_failed"),
	 QStringLiteral("DHT sensor read failed")},
	{QStringLiteral("telegram_alert_failed"),
	 QStringLiteral("Telegram temperature alert failed")},
	{QStringLiteral("telegram_all_clear_failed"),
	 QStringLiteral("Telegram all-clear notification failed")}
};

static std::optional<QString> statusParser(const QString &line){
	const QStringList fields = line.split(',');

	if (fields.size() == 3 &&
		fields[0] == QStringLiteral("STATUS") &&
		fields[1] == QStringLiteral("sleeping") &&
		fields[2].startsWith(QStringLiteral("seconds="))) {
		const QString seconds = fields[2].mid(QStringLiteral("seconds=").size());

		bool validSeconds = false;
		seconds.toULongLong(&validSeconds);

		if (validSeconds) {
			return QStringLiteral("Device sleeping for %1 seconds").arg(seconds);
		}
	}

	if (fields.size() != 2 ||
		(fields[0] != QStringLiteral("STATUS") &&
		 fields[0] != QStringLiteral("ERROR"))) {
		return std::nullopt;
	}

	const auto message = statusMessages.constFind(fields[1]);

	if (message == statusMessages.cend()) {
		return QStringLiteral("Unknown device event: %1").arg(fields[1]);
	}

	return *message;
}

void SerialWorker::processLine(const QString &line){

	const auto telemetry = TelemetryParser::parser(line);

	if (telemetry){
		emit newMeasurement(Measurement{
			QDateTime::currentDateTime(),
			telemetry->temperature,
			telemetry->humidity
		});
		return;
	}

	const auto status = statusParser(line);

	if (status){
		emit statusMessage(*status);
	}
}