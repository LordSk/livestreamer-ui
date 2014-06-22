#-------------------------------------------------
#
# Project created by QtCreator 2014-06-15T13:15:53
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = livestreamer-ui
TEMPLATE = app


SOURCES += main.cpp \
    stream.cpp \
    twitchstream.cpp \
    configpath.cpp
SOURCES += mainwindow.cpp

HEADERS += mainwindow.h \
    stream.h \
    twitchstream.h \
    configpath.h

FORMS += mainwindow.ui

RESOURCES = app.qrc

RC_ICONS = icons/app-ico64.ico
