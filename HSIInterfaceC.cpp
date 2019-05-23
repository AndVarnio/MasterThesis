#include "HSIInterfaceC.h"
#include "HSICamera.hpp"

extern "C"
{
    MHandle create_magic() { return new HSICamera(); }
    void  initialize_magic(MHandle p, double exposureMs, int rows, int columns, int frames, double fps, cubeFormat cube) { return ((HSICamera *)p)->initialize(exposureMs, rows, columns, frames, fps, cube); }
    void run_magic(MHandle p) {return ((HSICamera *)p)->runCubeCapture();}
}
