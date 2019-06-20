HEADERS       = glwidget.h \
    texturemanager.h \
                window.h \
                mainwindow.h \
    cmesh.h \
    player.h \
    gameobject.h \
    cube.h \
    bullet.h
SOURCES       = glwidget.cpp \
                main.cpp \
    texturemanager.cpp \
                window.cpp \
                mainwindow.cpp \
    cmesh.cpp \
    player.cpp \
    gameobject.cpp \
    cube.cpp \
    bullet.cpp

QT           += widgets

# install
target.path = ./qtglgame
INSTALLS += target

DESTDIR = builds
OBJECTS_DIR = builds/objects

DISTFILES += \
    builds/resources/shader.fs \
    builds/resources/shader.vs
