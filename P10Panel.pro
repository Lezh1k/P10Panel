TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += /home/lezh1k/avr8-gnu-toolchain-linux_x86_64/avr/include
INCLUDEPATH += include

DEFINES += __AVR_ATtiny2313__

DISTFILES += \
    Makefile

SOURCES += \
    src/main.c \
    src/p10_screen.c
