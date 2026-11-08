#include "MainWindow.h"
#include "ChartWidget.h"

#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

namespace {
	// The firmware reports about once a minute (plus Wi-Fi/boot time).
	constexpr qint64 kOverdueAfterSecs = 90;
	constexpr qint64 kStaleAfterSecs = 180;

	void setProp(QWidget *w, const char *name, const QVariant &value){
		w->setProperty(name, value);
		w->style()->unpolish(w);
		w->style()->polish(w);
	}

	QString formatAge(qint64 secs){
		if (secs < 60) return QStringLiteral("%1 s").arg(secs);
		return QStringLiteral("%1 min %2 s").arg(secs / 60).arg(secs % 60);
	}
}

MainWindow::Card MainWindow::makeCard(const QString &title){
	Card card;
	card.frame = new QFrame;
	card.frame->setObjectName("card");
	card.frame->setProperty("alert", false);

	auto *titleLabel = new QLabel(title);
	titleLabel->setObjectName("cardTitle");

	card.value = new QLabel("--");
	card.value->setObjectName("cardValue");

	card.subtitle = new QLabel;
	card.subtitle->setObjectName("cardSubtitle");

	auto *layout = new QVBoxLayout(card.frame);
	layout->setContentsMargins(18, 14, 18, 14);
	layout->setSpacing(2);
	layout->addWidget(titleLabel);
	layout->addWidget(card.value);
	layout->addWidget(card.subtitle);
	return card;
}

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),
	portCombo_(new QComboBox),
	baudCombo_(new QComboBox),
	connectButton_(new QPushButton(tr("Connect"))),
	thresholdSpin_(new QDoubleSpinBox),
	temperatureCard_(makeCard(tr("TEMPERATURE"))),
	humidityCard_(makeCard(tr("HUMIDITY"))),
	chart_(new ChartWidget(&store_)),
	console_(new QPlainTextEdit),
	connectionLabel_(new QLabel(tr("Disconnected"))),
	lastReadingLabel_(new QLabel),
	freshnessLabel_(new QLabel),
	freshnessTimer_(new QTimer(this))
{
	QFile style(":/style.qss");
	if (style.open(QIODevice::ReadOnly)){
		setStyleSheet(QString::fromUtf8(style.readAll()));
	}

	buildToolBar();

	temperatureCard_.subtitle->setText(tr("No reading yet"));
	humidityCard_.subtitle->setText(tr("Relative humidity"));

	console_->setObjectName("console");
	console_->setReadOnly(true);
	console_->setMaximumBlockCount(1000);
	console_->setPlaceholderText(tr("Device events and errors appear here"));

	auto *cards = new QGridLayout;
	cards->setSpacing(12);
	cards->addWidget(temperatureCard_.frame, 0, 0);
	cards->addWidget(humidityCard_.frame, 0, 1);

	auto *splitter = new QSplitter(Qt::Vertical);
	splitter->addWidget(chart_);
	splitter->addWidget(console_);
	splitter->setStretchFactor(0, 3);
	splitter->setStretchFactor(1, 1);
	splitter->setChildrenCollapsible(false);

	auto *layout = new QVBoxLayout;
	layout->setContentsMargins(12, 12, 12, 12);
	layout->setSpacing(12);
	layout->addLayout(cards);
	layout->addWidget(splitter, 1);

	auto *centralWidget = new QWidget;
	centralWidget->setLayout(layout);
	setCentralWidget(centralWidget);

	freshnessLabel_->setObjectName("freshness");
	statusBar()->addWidget(connectionLabel_);
	statusBar()->addWidget(lastReadingLabel_, 1);
	statusBar()->addPermanentWidget(freshnessLabel_);

	connect(freshnessTimer_, &QTimer::timeout, this, &MainWindow::updateFreshness);
	freshnessTimer_->start(1000);

	chart_->setThresholdCelsius(thresholdSpin_->value());
	updateFreshness();

	setWindowTitle("EnvStat");
	resize(820, 640);
}

void MainWindow::buildToolBar(){
	auto *bar = addToolBar(tr("Connection"));
	bar->setMovable(false);

	for (int baud : {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600}){
		baudCombo_->addItem(QString::number(baud), baud);
	}
	baudCombo_->setCurrentIndex(baudCombo_->findData(115200));

	auto *refreshButton = new QPushButton(tr("Refresh"));
	connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshPorts);
	connect(connectButton_, &QPushButton::clicked, this, &MainWindow::onConnectClicked);

	thresholdSpin_->setRange(-40.0, 80.0);
	thresholdSpin_->setValue(20.0);
	thresholdSpin_->setSingleStep(0.5);
	thresholdSpin_->setSuffix(" °C");
	connect(thresholdSpin_, &QDoubleSpinBox::valueChanged, this, [this](double v){
		chart_->setThresholdCelsius(v);
		updateThresholdState();
	});

	portCombo_->setMinimumContentsLength(22);

	bar->addWidget(new QLabel(tr("Port ")));
	bar->addWidget(portCombo_);
	bar->addWidget(refreshButton);
	bar->addWidget(new QLabel(tr(" Baud ")));
	bar->addWidget(baudCombo_);
	bar->addWidget(connectButton_);
	bar->addSeparator();
	bar->addWidget(new QLabel(tr(" Alert below ")));
	bar->addWidget(thresholdSpin_);

	refreshPorts();
}

void MainWindow::refreshPorts(){
	const QString previous = portCombo_->currentData().toString();
	portCombo_->clear();

	for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()){
		const QString label = info.description().isEmpty()
			? info.systemLocation()
			: QStringLiteral("%1 — %2").arg(info.systemLocation(), info.description());
		portCombo_->addItem(label, info.systemLocation());
	}

	if (portCombo_->count() == 0){
		portCombo_->addItem(tr("No serial ports found"), QString());
	} else if (const int i = portCombo_->findData(previous); i >= 0){
		portCombo_->setCurrentIndex(i);
	}
}

void MainWindow::onConnectClicked(){
	if (connected_){
		emit disconnectRequested();
		return;
	}

	const QString port = portCombo_->currentData().toString();
	if (port.isEmpty()){
		showStatusMessage(tr("Failed to connect: no serial port selected"));
		return;
	}
	emit connectRequested(port, baudCombo_->currentData().toInt());
}

void MainWindow::setConnected(bool connected, const QString &portName){
	connected_ = connected;
	connectButton_->setText(connected ? tr("Disconnect") : tr("Connect"));
	portCombo_->setEnabled(!connected);
	baudCombo_->setEnabled(!connected);
	connectionLabel_->setText(
		connected ? tr("Connected: %1").arg(portName) : tr("Disconnected"));
	updateFreshness();
}

void MainWindow::updateDisplay(const Measurement &measurement){
	store_.add(measurement);

	temperatureCard_.value->setText(
		QStringLiteral("%1 °C").arg(measurement.temperatureCelsius, 0, 'f', 1));
	humidityCard_.value->setText(
		QStringLiteral("%1 %").arg(measurement.relativeHumidity, 0, 'f', 1));

	updateThresholdState();
	updateFreshness();
	chart_->refresh();
}

void MainWindow::updateThresholdState(){
	const auto latest = store_.latest();
	if (!latest){
		return;
	}

	const bool below = latest->temperatureCelsius < thresholdSpin_->value();
	setProp(temperatureCard_.frame, "alert", below);
	temperatureCard_.subtitle->setText(
		below ? tr("Below %1 °C threshold").arg(thresholdSpin_->value(), 0, 'f', 1)
		      : tr("Within normal range"));
}

void MainWindow::updateFreshness(){
	const auto latest = store_.latest();

	if (!latest){
		lastReadingLabel_->setText(tr("  Last reading: none yet"));
		freshnessLabel_->setText(connected_ ? tr("Waiting for first reading") : tr("Not connected"));
		setProp(freshnessLabel_, "state", "idle");
		return;
	}

	const qint64 age = qMax<qint64>(0, latest->receivedAt.secsTo(QDateTime::currentDateTime()));
	lastReadingLabel_->setText(
		tr("  Last reading: %1 (%2 ago)")
			.arg(latest->receivedAt.toString("HH:mm:ss"), formatAge(age)));

	if (age <= kOverdueAfterSecs){
		freshnessLabel_->setText(tr("Data up to date"));
		setProp(freshnessLabel_, "state", "ok");
	} else if (age <= kStaleAfterSecs){
		freshnessLabel_->setText(connected_
			? tr("Reading overdue (device may be sleeping)")
			: tr("Disconnected, data aging"));
		setProp(freshnessLabel_, "state", "overdue");
	} else {
		freshnessLabel_->setText(connected_ ? tr("Stale: no data for %1").arg(formatAge(age))
		                                    : tr("Stale (disconnected)"));
		setProp(freshnessLabel_, "state", "stale");
	}
}

void MainWindow::showStatusMessage(const QString &message){
	const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
	QString line = QStringLiteral("[%1] %2").arg(timestamp, message).toHtmlEscaped();

	if (message.contains("fail", Qt::CaseInsensitive) ||
	    message.contains("error", Qt::CaseInsensitive)){
		line = QStringLiteral("<span style=\"color:#d9482b\">%1</span>").arg(line);
	}

	console_->appendHtml(line);
}
