#-------------------------------------------------
#
# Project created by QtCreator 2019-09-24T21:15:33
#
#-------------------------------------------------

QT       += testlib

QT       += gui

TARGET = tst_qtunittesttest
CONFIG   += console
CONFIG   -= app_bundle
CONFIG += c++14

TEMPLATE = app

INCLUDEPATH += .
INCLUDEPATH += /usr/local/include
INCLUDEPATH += /usr/local/include/opencv4
INCLUDEPATH += /usr/local/include/sigpack-1.2.6/sigpack
INCLUDEPATH += ../heart_rate

LIBS += -L/usr/local/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_videoio -lopencv_imgproc -lopencv_objdetect -lfftw3 -lm -larmadillo
#LIBS += -L/root/fftw/fftw-3.3.8/.libs -lfftw3 -lm


SOURCES += tst_qtunittesttest.cpp
SOURCES += ../heart_rate/signalProcessing.cpp
SOURCES += ../heart_rate/videoProcessing.cpp
HEADERS += ../heart_rate/signalProcessing.hpp
HEADERS += ../heart_rate/videoProcessing.hpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

HEADERS +=
