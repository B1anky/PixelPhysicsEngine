QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
QMAKE_CXXFLAGS += -Wall -Wextra -pedantic -Wshadow
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Elements.cpp \
    Engine.cpp \
    PhysicsWindow.cpp \
    QGraphicsPixelItem.cpp \
    main.cpp \
    MainWindow.cpp

HEADERS += \
    Elements.h \
    Engine.h \
    Hashhelpers.h \
    MainWindow.h \
    PhysicsWindow.h \
    Tile.h \
    QGraphicsPixelItem.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
