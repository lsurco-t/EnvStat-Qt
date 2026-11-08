#include <QtTest>
#include "app/MeasurementStore.h"

class MeasurementStoreTest : public QObject {
	Q_OBJECT

	private slots:
		void startsEmpty();
		void storesLatest();
		void dropsOldestWhenFull();
		void clearEmptiesStore();
};

static Measurement make(double temperature){
	return Measurement{QDateTime::currentDateTime(), temperature, 50.0};
}

void MeasurementStoreTest::startsEmpty(){
	MeasurementStore store;
	QVERIFY(store.empty());
	QVERIFY(!store.latest().has_value());
}

void MeasurementStoreTest::storesLatest(){
	MeasurementStore store;
	store.add(make(20.0));
	store.add(make(21.5));

	QCOMPARE(store.size(), std::size_t{2});
	QCOMPARE(store.latest()->temperatureCelsius, 21.5);
}

void MeasurementStoreTest::dropsOldestWhenFull(){
	MeasurementStore store(3);
	for (int i = 0; i < 5; ++i){
		store.add(make(i));
	}

	QCOMPARE(store.size(), std::size_t{3});
	QCOMPARE(store.all().front().temperatureCelsius, 2.0);
	QCOMPARE(store.all().back().temperatureCelsius, 4.0);
}

void MeasurementStoreTest::clearEmptiesStore(){
	MeasurementStore store;
	store.add(make(20.0));
	store.clear();
	QVERIFY(store.empty());
}

QTEST_MAIN(MeasurementStoreTest)

#include "tst_MeasurementStore.moc"
