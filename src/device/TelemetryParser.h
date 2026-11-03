#pragma once
#include <QString>
#include <optional>

struct Telemetry {
	float temperature;
	float humidity;
};

class TelemetryParser {
	public:
		static std::optional<Telemetry> parser(const QString &line);
};