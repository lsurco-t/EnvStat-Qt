#pragma once
#include <QDateTime>
#include <QMetaType>

struct Measurement {
	QDateTime receivedAt;
	double temperatureCelsius = 0.0;
	double relativeHumidity = 0.0;
};

Q_DECLARE_METATYPE(Measurement)
