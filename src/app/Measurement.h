#pragma once
#include <QDateTime>

struct Measurement {
	QDateTime receivedAt;
	double temperatureCelsius;
	double relativeHumidity;
};
