#include <QtTest>
#include "device/TelemetryParser.h"

class TelemetryParserTest : public QObject {
	Q_OBJECT

	private slots:
		void parsesValidMeasurement();
		void rejectsInvalidRecord();
		void rejectsStatusRecord();
};

void TelemetryParserTest::parsesValidMeasurement(){
	const auto result = TelemetryParser::parser(
		QStringLiteral("DATA,temperature_c=22.2,humidity_pct=38.4")
	);

	QVERIFY(result.has_value());
	QCOMPARE(result->temperature, 22.2f);
	QCOMPARE(result->humidity, 38.4f);
}

void TelemetryParserTest::rejectsInvalidRecord(){
	const auto result = TelemetryParser::parser(
		QStringLiteral("DATA,temperature_c=invalid,humidity_pct=38.4")
	);

	QVERIFY(!result.has_value());
}

void TelemetryParserTest::rejectsStatusRecord(){
	const auto result = TelemetryParser::parser(
		QStringLiteral("STATUS,wifi_connected")
	);

	QVERIFY(!result.has_value());
}

QTEST_MAIN(TelemetryParserTest)

#include "tst_TelemetryParser.moc"