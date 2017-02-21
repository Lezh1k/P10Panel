#-------------------------------------------------
#
# Project created by QtCreator 2015-12-09T21:57:17
#
#-------------------------------------------------

QT       += core gui multimedia serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BoulderingTimer
TEMPLATE = app


SOURCES += src/main.cpp\
        src/mainwindow.cpp \
    src/p10_controller.cpp

HEADERS  += include/mainwindow.h\
        include/screen.h \
    include/p10_controller.h

FORMS    += forms/mainwindow.ui
