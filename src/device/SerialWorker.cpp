#include "SerialWorker.h"
#include "TelemetryParser.h"
#include <QHash>
#include <optional>

SerialWorker::SerialWorker(QObject *parent) : QObject(parent){
	SerialPort_ = new QSerialPort(this);

	SerialPort_->setPortName("/dev/ttyUSB0");
	SerialPort_->setBaudRate(QSerialPort::Baud115200);

	connect(SerialPort_, &QSerialPort::readyRead, this, &SerialWorker::readSerialData);
}

void SerialWorker::startListening(){
	if (SerialPort_->open(QIODevice::ReadOnly)){
		emit statusMessage(
			QStringLiteral("Connected to ESP32 Successfully")
		);
	} else {
		emit statusMessage(
			QStringLiteral("Failed to connect to ESP32: %1").arg(SerialPort_->errorString())
		);
	}
}

void SerialWorker::readSerialData(){
	buffer_ += SerialPort_->readAll();

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
		emit newDataParsed(telemetry->temperature, telemetry->humidity);
		return;
	}

	const auto status = statusParser(line);

	if (status){
		emit statusMessage(*status);
	}
}