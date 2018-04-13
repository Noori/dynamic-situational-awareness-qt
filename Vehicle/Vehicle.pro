#-------------------------------------------------
#  Copyright 2018 ESRI
#
#  All rights reserved under the copyright laws of the United States
#  and applicable international laws, treaties, and conventions.
#
#  You may freely redistribute and use this sample code, with or
#  without modification, provided you include the original copyright
#  notice and use restrictions.
#
#  See the Sample code usage restrictions document for further information.
#-------------------------------------------------

TARGET = DSA_Vehicle_Qt
TEMPLATE = app

QT += core gui opengl network positioning sensors qml quick xml concurrent
CONFIG += c++11

ARCGIS_RUNTIME_VERSION = 100.2.1
include($$PWD/../Shared/build/arcgisruntime.pri)
include($$PWD/../Shared/build/arcgisruntimecpptoolkit.pri)

INCLUDEPATH += $$PWD/../Shared/ \
    $$PWD/../Shared/alerts \
    $$PWD/../Shared/analysis \
    $$PWD/../Shared/messages \
    $$PWD/../Shared/utilities \
    $$PWD/../Shared/markup

HEADERS += \
    AppInfo.h \
    Vehicle.h \
    VehicleStyles.h \
    $$PWD/../Shared/*.h \
    $$PWD/../Shared/alerts/*.h \
    $$PWD/../Shared/analysis/*.h \
    $$PWD/../Shared/messages/*.h \
    $$PWD/../Shared/utilities/*.h \
    $$PWD/../Shared/markup/*.h

SOURCES += \
    main.cpp \
    Vehicle.cpp \
    VehicleStyles.cpp \
    $$PWD/../Shared/*.cpp \
    $$PWD/../Shared/alerts/*.cpp \
    $$PWD/../Shared/analysis/*.cpp \
    $$PWD/../Shared/messages/*.cpp \
    $$PWD/../Shared/utilities/*.cpp \
    $$PWD/../Shared/markup/*.cpp

RESOURCES += \
    qml/qml.qrc \
    ../Shared/Resources/Resources.qrc \
    ../Shared/Resources/application.qrc \
    ../Shared/qml/shared_qml.qrc

QML_IMPORT_PATH += $$PWD/../Shared/qml

!android {
  PRECOMPILED_HEADER = $$PWD/../Shared/pch.hpp
  CONFIG += precompile_header
}

#-------------------------------------------------------------------------------

win32 {
    include (Win/Win.pri)
}

macx {
    include (Mac/Mac.pri)
}

ios {
    include (iOS/iOS.pri)
}

android {
    include (Android/Android.pri)
}

DISTFILES += \
    Resources/qtquickcontrols2.conf
