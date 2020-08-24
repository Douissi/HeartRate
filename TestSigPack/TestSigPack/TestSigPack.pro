TEMPLATE = app
CONFIG += console
CONFIG += c++14
CONFIG -= app_bundle
CONFIG -= qt

#INCLUDEPATH += /root/sigpack/sigpack-1.2.6/sigpack
INCLUDEPATH += /usr/local/include/sigpack-1.2.6/sigpack
LIBS+=  -lm -larmadillo

SOURCES += main.cpp
