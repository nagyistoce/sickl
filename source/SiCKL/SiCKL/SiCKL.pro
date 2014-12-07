#-------------------------------------------------
#
# Project created by QtCreator 2014-07-18T22:13:26
#
#-------------------------------------------------

QT -= core gui

TEMPLATE = lib
CONFIG += staticlib

Debug:TARGET = SiCKLD
Release:TARGET = SiCKL

unix {
   QMAKE_CXXFLAGS += -std=c++11
   QMAKE_CXXFLAGS += -Wno-invalid-offsetof
}

INCLUDEPATH += include

win32 {
    DEFINES += FREEGLUT_LIB_PRAGMAS=0
    DEFINES += FREEGLUT_STATIC=1
    DEFINES += GLEW_STATIC=1

    INCLUDEPATH += \
        ../../../extern/glew-1.9.0/include \
        ../../../extern/freeglut-2.8.0/include
}

# includes

HEADERS += \
    include/Types.h \
    include/Source.h \
    include/SiCKL.h \
    include/Program.h \
    include/Interfaces.h \
    include/Functions.h \
    include/Friends.h \
    include/Enums.h \
    include/Defines.h \
    include/Declares.h \
    include/Compiler.h \
    include/Buffers.h \
    include/AST.h \
    include/Backends/OpenGL.h \
    include/Common.h

# sources

SOURCES += \
    source/Backends/OpenGL/OpenGL.Runtime.cpp \
    source/Backends/OpenGL/OpenGL.Program.cpp \
    source/Backends/OpenGL/OpenGL.Compiler.cpp \
    source/Types.cpp \
    source/Source.cpp \
    source/Functions.cpp \
    source/AST.cpp \
    source/Common.cpp



unix {
    target.path = /usr/lib
    INSTALLS += target
}

# output directories

Release:DESTDIR = $$PWD/../../../bin/Release
Debug:DESTDIR = $$PWD/../../../bin/Debug

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui
