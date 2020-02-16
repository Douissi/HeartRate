#-------------------------------------------------
#
# Project created by QtCreator 2019-01-21T20:25:34
#
#-------------------------------------------------

QT       += core gui

CONFIG += c++14

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = heart_rate
TEMPLATE = app

INCLUDEPATH += .
INCLUDEPATH += /usr/local/include
INCLUDEPATH += /usr/local/include/opencv4
INCLUDEPATH += /usr/local/include/sigpack-1.2.6/sigpack


LIBS += -L/usr/local/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_videoio -lopencv_imgproc -lopencv_objdetect -lfftw3 -lm -larmadillo
#LIBS += -L/root/fftw/fftw-3.3.8/.libs -lfftw3 -lm

SOURCES += main.cpp\
        mainwindow.cpp \
    qcvwidget.cpp

HEADERS  += mainwindow.h \
    videoProcessing.hpp \
    signalProcessing.hpp \
    qcvwidget.h

FORMS    += mainwindow.ui \
    qcvwidget.ui


DISTFILES += \
    ../README.md
