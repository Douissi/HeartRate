#-------------------------------------------------
#
# Project created by QtCreator 2019-01-21T20:25:34
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = heart_rate
TEMPLATE = app

INCLUDEPATH += .
INCLUDEPATH += /usr/local/include
INCLUDEPATH += /usr/local/include/opencv4
INCLUDEPATH += /root/sigpack/sigpack-1.2.6/sigpack
#INCLUDEPATH += /root/fftw/fftw-3.3.8
#INCLUDEPATH += /usr/local/include/opencv4/opencv2
#INCLUDEPATH += /usr/local/include/opencv4/opencv2/core
#INCLUDEPATH += /usr/local/include/opencv4/opencv2/highgui
#INCLUDEPATH += /usr/local/include/opencv4/opencv2/imgproc
#INCLUDEPATH += /usr/local/include/opencv4/opencv2/flann
#INCLUDEPATH += /usr/local/include/opencv4/opencv2/photo
#INCLUDEPATH += /usr/local/include/opencv4/opencv2/video
#INCLUDEPATH += /usr/local/include/opencv4/opencv2/features2d
#INCLUDEPATH += /usr/local/include/opencv4/opencv2/objdetect
#INCLUDEPATH += /usr/local/include/opencv4/opencv2/calib3d
#INCLUDEPATH += /usr/local/include/opencv4/opencv2/ml
#INCLUDEPATH += /usr/local/include/opencv4/opencv2/contrib


LIBS += -L/usr/local/lib -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_videoio -lopencv_imgproc -lopencv_objdetect -lfftw3 -lm -larmadillo
#LIBS += -L/root/fftw/fftw-3.3.8/.libs -lfftw3 -lm

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h \
    videoProcessing.hpp \
    signalProcessing.hpp

FORMS    += mainwindow.ui


DISTFILES += \
    ../README.md
