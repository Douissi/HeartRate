#include <QString>
#include <QtTest>
#include "videoProcessing.hpp"
#include "signalProcessing.hpp"

class TestVideoProcessing : public QObject
{
    Q_OBJECT

public:
    TestVideoProcessing(){}
    virtual ~TestVideoProcessing() {}

private Q_SLOTS:
    void testVideo1();
};

class TestSignalProcessing : public QObject
{
    Q_OBJECT

public:
    TestSignalProcessing(){}
    virtual ~TestSignalProcessing() {}

private Q_SLOTS:
    void testSignal1();

};



/********************************************* Testing methods *****************************************/

void TestVideoProcessing::testVideo1()
{
    QVERIFY2(true, "Failure");
}

void TestSignalProcessing::testSignal1()
{
     QVERIFY2(true, "Failure");
}















/********************************************** Main function, running unit tests ***************************/
//QTEST_APPLESS_MAIN(TestVideoProcessing)
int main(int argc, char *argv[])
{
    TestVideoProcessing tc;
    TestSignalProcessing tc2;
    QTEST_SET_MAIN_SOURCE_PATH
    int resTc1 = QTest::qExec(&tc, argc, argv);
    int resTc2 = QTest::qExec(&tc2, argc, argv);
    return resTc1 || resTc2;
}


#include "tst_qtunittesttest.moc"
