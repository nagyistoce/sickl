QT += opengl

TEMPLATE = app

CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = Mandelbrot

unix {
    QMAKE_CXXFLAGS += -std=c++11
}

# includes

INCLUDEPATH += \
    ../SiCKL/include \
    ../../../extern/EasyBMP_1.06

# sources

SOURCES += \
    Main.cpp \
    ../../../extern/EasyBMP_1.06/EasyBMP.cpp

# linking

Debug:LIBS += -L../../../bin/Debug -lSiCKLD
Release:LIBS += -L../../../bin/Release -lSiCKL

win32
{
    LIBS += -L../../../extern/glew-1.9.0/lib -lglew32s
    LIBS += -L../../../extern/freeglut-2.8.0/lib -lfreeglut_static
}

unix {
    LIBS += -L/usr/lib -lglut
    LIBS += -lGLEW
    LIBS += -lGLU
    LIBS += -lGL
    LIBS += -lX11
    LIBS += -ldl
    LIBS += -lXext
    LIBS += -lXxf86vm
    LIBS += -lXi
}

# DEPENDPATH += $$PWD/../SiCKL/include

# output directories

Release:DESTDIR = $$PWD/../../../bin/Release
Debug:DESTDIR = $$PWD/../../../bin/Debug

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui
