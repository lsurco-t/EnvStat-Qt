#include "ChartWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <algorithm>

namespace {
	constexpr qint64 kMinSpanMs = 5 * 60 * 1000;
	constexpr qint64 kGapMs = 3 * 60 * 1000;
	constexpr int kTicks = 5;

	QString timeLabel(qint64 ms){
		return QDateTime::fromMSecsSinceEpoch(ms).toString("HH:mm:ss");
	}
}

ChartWidget::ChartWidget(const MeasurementStore *store, QWidget *parent)
	: QWidget(parent), store_(store)
{
	setAutoFillBackground(false);
}

void ChartWidget::setThresholdCelsius(double threshold){
	thresholdCelsius_ = threshold;
	update();
}

void ChartWidget::refresh(){
	update();
}

QSize ChartWidget::minimumSizeHint() const {
	return {320, 180};
}

void ChartWidget::paintEvent(QPaintEvent *){
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	const QPalette &pal = palette();
	const QColor textColor = pal.color(QPalette::WindowText);
	QColor gridColor = pal.color(QPalette::Mid);
	gridColor.setAlpha(110);
	const QColor tempColor(0xd9, 0x48, 0x2b);
	const QColor humColor(0x2b, 0x7a, 0xd9);

	p.fillRect(rect(), pal.color(QPalette::Base));
	p.setPen(textColor);

	const auto &data = store_->all();
	if (data.empty()){
		p.drawText(rect(), Qt::AlignCenter,
			tr("Waiting for the first measurement…\nThe device reports about once a minute."));
		return;
	}

	const QFontMetrics fm(font());
	const QRectF plot = QRectF(rect()).adjusted(52, 30, -52, -fm.height() - 12);
	if (plot.width() < 20 || plot.height() < 20){
		return;
	}

	// Axis ranges.
	const qint64 t0 = data.front().receivedAt.toMSecsSinceEpoch();
	const qint64 span = std::max(data.back().receivedAt.toMSecsSinceEpoch() - t0, kMinSpanMs);

	double tMin = thresholdCelsius_, tMax = thresholdCelsius_;
	for (const auto &m : data){
		tMin = std::min(tMin, m.temperatureCelsius);
		tMax = std::max(tMax, m.temperatureCelsius);
	}
	tMin -= 2.0;
	tMax += 2.0;
	if (tMax - tMin < 10.0){
		const double mid = (tMax + tMin) / 2.0;
		tMin = mid - 5.0;
		tMax = mid + 5.0;
	}

	auto xOf = [&](qint64 ms){
		return plot.left() + double(ms - t0) / double(span) * plot.width();
	};
	auto yTemp = [&](double v){
		return plot.bottom() - (v - tMin) / (tMax - tMin) * plot.height();
	};
	auto yHum = [&](double v){
		return plot.bottom() - std::clamp(v, 0.0, 100.0) / 100.0 * plot.height();
	};

	// Grid and axis labels.
	p.setFont(font());
	for (int i = 0; i < kTicks; ++i){
		const double f = double(i) / (kTicks - 1);
		const double y = plot.bottom() - f * plot.height();

		p.setPen(QPen(gridColor, 1));
		p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));

		p.setPen(tempColor);
		p.drawText(QRectF(0, y - 10, plot.left() - 6, 20),
			Qt::AlignRight | Qt::AlignVCenter,
			QString::number(tMin + f * (tMax - tMin), 'f', 1));

		p.setPen(humColor);
		p.drawText(QRectF(plot.right() + 6, y - 10, 46, 20),
			Qt::AlignLeft | Qt::AlignVCenter,
			QString::number(f * 100.0, 'f', 0));
	}

	p.setPen(textColor);
	const double labelY = plot.bottom() + 4;
	p.drawText(QRectF(plot.left() - 40, labelY, 80, fm.height()), Qt::AlignCenter, timeLabel(t0));
	p.drawText(QRectF(plot.center().x() - 40, labelY, 80, fm.height()), Qt::AlignCenter, timeLabel(t0 + span / 2));
	p.drawText(QRectF(plot.right() - 40, labelY, 80, fm.height()), Qt::AlignCenter, timeLabel(t0 + span));

	// Legend.
	p.setPen(tempColor);
	p.drawText(QPointF(plot.left(), 18), tr("● Temperature (°C)"));
	p.setPen(humColor);
	p.drawText(QPointF(plot.left() + 170, 18), tr("● Humidity (%)"));

	p.save();
	p.setClipRect(plot.adjusted(-6, -6, 6, 6));

	// Threshold line.
	QPen thresholdPen(tempColor, 1, Qt::DashLine);
	p.setPen(thresholdPen);
	p.drawLine(QPointF(plot.left(), yTemp(thresholdCelsius_)),
	           QPointF(plot.right(), yTemp(thresholdCelsius_)));

	// Series: lines broken across gaps, markers for every real sample.
	auto drawSeries = [&](const QColor &color, auto yOf, auto value){
		QPainterPath path;
		qint64 prev = 0;
		bool first = true;
		for (const auto &m : data){
			const qint64 t = m.receivedAt.toMSecsSinceEpoch();
			const QPointF pt(xOf(t), yOf(value(m)));
			if (first || t - prev > kGapMs){
				path.moveTo(pt);
			} else {
				path.lineTo(pt);
			}
			first = false;
			prev = t;
		}
		p.setPen(QPen(color, 1.5));
		p.setBrush(Qt::NoBrush);
		p.drawPath(path);

		p.setPen(Qt::NoPen);
		p.setBrush(color);
		for (const auto &m : data){
			p.drawEllipse(QPointF(xOf(m.receivedAt.toMSecsSinceEpoch()), yOf(value(m))), 3.0, 3.0);
		}
	};

	drawSeries(tempColor, yTemp, [](const Measurement &m){ return m.temperatureCelsius; });
	drawSeries(humColor, yHum, [](const Measurement &m){ return m.relativeHumidity; });
	p.restore();

	p.setPen(QPen(gridColor, 1));
	p.setBrush(Qt::NoBrush);
	p.drawRect(plot);
}
