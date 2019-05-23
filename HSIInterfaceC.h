#ifndef MAGIC_INTERFACE_H
#define MAGIC_INTERFACE_H

#include"HSICamera.hpp"
typedef enum cubeFormat cubeFormatC;
//struct HSICamera;

#ifdef __cplusplus
extern "C" {
#endif



typedef void * MHandle;
MHandle create_magic();
void    initialize_magic(MHandle p, double exposureMs, int rows, int columns, int frames, double fps, cubeFormatC cube);
void    run_magic(MHandle p);

#ifdef __cplusplus
}
#endif

#endif
