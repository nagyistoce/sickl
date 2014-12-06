#pragma once

#ifdef _WIN32
    #define WINDOWS 1
#else
    #define LINUX 1
#endif

#include <vector>
#include <sstream>
#include <stdint.h>
#include <string.h>
#if WINDOWS
    #include <intrin.h>
    #define DEBUGBREAK() __debugbreak()
#elif LINUX
    #define DEBUGBREAK() __builtin_trap()
#endif

#define COMPUTE_ASSERT(X) if(!(X)) {printf("Assert Failed: %s line %i on \"%s\"\n", __FILE__, __LINE__, #X);  DEBUGBREAK();}

#include "Enums.h"
#include "AST.h"
#include "Source.h"
#include "Compiler.h"
#include "Program.h"
#include "Common.h"

// Backends
#include "Backends/OpenGL.h"

