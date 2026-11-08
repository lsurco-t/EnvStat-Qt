#pragma once
#include <QMainWindow>
#include "app/Measurement.h"
#include "app/MeasurementStore.h"

class QComboBox;
class QDoubleSpinBox;
class QFrame;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTimer;
class ChartWidget;

class MainWindow : public QMainWindow {
	Q_OBJECT

	public:
		explicit MainWindow(QWidget *parent = nullptr);

	public slots:
		void updateDisplay(const Measurement &measurement);
		void showStatusMessage(const QString &message);
		void setConnected(bool connected, const QString &portName);

	signals:
		void connectRequested(const QString &portName, qint32 baudRate);
		void disconnectRequested();

	private:
		struct Card {
			QFrame *frame = nullptr;
			QLabel *value = nullptr;
			QLabel *subtitle = nullptr;
		};

		static Card makeCard(const QString &title);
		void buildToolBar();
		void refreshPorts();
		void onConnectClicked();
		void updateThresholdState();
		void updateFreshness();

		MeasurementStore store_;
		bool connected_ = false;

		QComboBox *portCombo_;
		QComboBox *baudCombo_;
		QPushButton *connectButton_;
		QDoubleSpinBox *thresholdSpin_;

		Card temperatureCard_;
		Card humidityCard_;
		ChartWidget *chart_;
		QPlainTextEdit *console_;

		QLabel *connectionLabel_;
		QLabel *lastReadingLabel_;
		QLabel *freshnessLabel_;
		QTimer *freshnessTimer_;
};
