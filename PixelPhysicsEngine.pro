QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
QMAKE_CXXFLAGS += -Wall -Wextra -pedantic -Wshadow -fsanitize-recover

QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE -= -O3
QMAKE_CXXFLAGS_RELEASE += -O0

QMAKE_CFLAGS_RELEASE -= -O1
QMAKE_CFLAGS_RELEASE -= -O2
QMAKE_CFLAGS_RELEASE -= -O3
QMAKE_CFLAGS_RELEASE += -O0

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Elements.cpp \
    Engine.cpp \
    PhysicsWindow.cpp \
    QGraphicsEngineItem.cpp \
    QGraphicsPixelItem.cpp \
    TileSet.cpp \
    main.cpp \
    MainWindow.cpp

HEADERS += \
    Elements.h \
    Engine.h \
    GeneralHashHelpers.h \
    Hashhelpers.h \
    MainWindow.h \
    PhysicsWindow.h \
    QGraphicsEngineItem.h \
    Tile.h \
    QGraphicsPixelItem.h \
    TileSet.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
