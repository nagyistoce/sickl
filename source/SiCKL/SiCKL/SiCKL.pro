QT -= core gui

TEMPLATE = lib
CONFIG += staticlib

# fix config to ONLY contain our build type
CONFIG(debug, debug|release) {
    CONFIG -= release
    CONFIG += debug
} else {
    CONFIG -= debug
    CONFIG += release
}

# binary name

debug:TARGET = SiCKLD
release:TARGET = SiCKL

# compiler flags

unix {
    QMAKE_CXXFLAGS += -std=c++11
    QMAKE_CXXFLAGS += -Wno-invalid-offsetof
}

macx {
    QMAKE_MAC_SDK = macosx10.10
}

# include paths

INCLUDEPATH += include

win32 {
    DEFINES += GLEW_STATIC=1

    INCLUDEPATH += \
        $$PWD/../../../extern/glew-1.9.0/include \
        $$PWD/../../../extern/glfw-3.0.4/include
} else:macx {
    INCLUDEPATH +=\
        /usr/local/include
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
    include/Common.h \
    include/Backends/OpenCL.h

# sources

SOURCES += \
    source/Backends/OpenGL/OpenGL.Runtime.cpp \
    source/Backends/OpenGL/OpenGL.Program.cpp \
    source/Backends/OpenGL/OpenGL.Compiler.cpp \
    source/Types.cpp \
    source/Source.cpp \
    source/Functions.cpp \
    source/AST.cpp \
    source/Backends/OpenCL/OpenCL.Runtime.cpp \
    source/Backends/OpenCL/OpenCL.Compiler.cpp

# unix {
#   target.path = /usr/lib
#    INSTALLS += target
# }

# output binary directories

debug:DESTDIR = $$PWD/../../../bin/Debug
release:DESTDIR = $$PWD/../../../bin/Release

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui


