#include <QString>
#include <QtTest>

class QtUnitTestTest : public QObject
{
    Q_OBJECT

public:
    QtUnitTestTest();

private Q_SLOTS:
    void testCase1();
};

QtUnitTestTest::QtUnitTestTest()
{
}

void QtUnitTestTest::testCase1()
{
    QVERIFY2(true, "Failure");
}

QTEST_APPLESS_MAIN(QtUnitTestTest)

#include "tst_qtunittesttest.moc"
