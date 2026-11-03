#include "TelemetryParser.h"
#include <QRegularExpression>
#include <QDebug>

std::optional<Telemetry> TelemetryParser::parser(const QString &line){
	Telemetry data;

	static const QRegularExpression pattern(
		R"(^DATA,temperature_c=([-+]?\d+(?:\.\d+)?),humidity_pct=([-+]?\d+(?:\.\d+)?)$)"
	);
	
	const QRegularExpressionMatch match = pattern.match(line);
	if (!match.hasMatch()){
		return std::nullopt;
	}

	data.temperature = match.captured(1).toFloat();
	data.humidity = match.captured(2).toFloat();

	return data;
}