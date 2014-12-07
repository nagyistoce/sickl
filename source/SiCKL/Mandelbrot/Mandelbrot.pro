QT -= core gui

TEMPLATE = app

CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

# fix config to ONLY contain our build type
CONFIG(debug, debug|release) {
    CONFIG -= release
    CONFIG += debug
} else {
    CONFIG -= debug
    CONFIG += release
}

# binary name

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

debug:LIBS += -L$$PWD/../../../bin/Debug -lSiCKLD
release:LIBS += -L$$PWD/../../../bin/Release -lSiCKL

win32 {
    LIBS += -L$$PWD/../../../extern/glew-1.9.0/lib -lglew32s
    LIBS += -L$$PWD/../../../extern/glfw-3.0.4/lib -lglfw3
    LIBS += -lopengl32
    LIBS += -luser32
    LIBS += -lkernel32
    LIBS += -lgdi32
} else:macx {
    QMAKE_MAC_SDK = macosx10.10
    LIBS += -L/usr/local/lib -lGLEW
    LIBS += -L/usr/local/lib -lglfw3
    LIBS += -framework Cocoa
    LIBS += -framework OpenGL
    LIBS += -framework IOKit
    LIBS += -framework CoreVideo
} else:unix {
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

release:DESTDIR = $$PWD/../../../bin/Release
debug:DESTDIR = $$PWD/../../../bin/Debug

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui

macx: LIBS += -lglfw3
