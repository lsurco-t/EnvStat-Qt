#pragma once
#include <QWidget>
#include "app/MeasurementStore.h"

// Dependency-free history chart: temperature (left axis) and humidity (right
// axis) against receive time. Only received samples are drawn as markers, and
// lines are broken across long gaps so the chart never implies data exists
// while the device was asleep or disconnected.
class ChartWidget : public QWidget {
	Q_OBJECT

	public:
		explicit ChartWidget(const MeasurementStore *store, QWidget *parent = nullptr);

		void setThresholdCelsius(double threshold);
		void refresh();

		QSize minimumSizeHint() const override;

	protected:
		void paintEvent(QPaintEvent *event) override;

	private:
		const MeasurementStore *store_;
		double thresholdCelsius_ = 20.0;
};
