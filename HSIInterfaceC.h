#ifndef MAGIC_INTERFACE_H
#define MAGIC_INTERFACE_H


#ifdef __cplusplus
extern "C" {
#endif

typedef void * CameraHandle_C;
CameraHandle_C create_camera_handle();
void    initialize_camera(CameraHandle_C p, double exposureMs, int rows, int columns, int frames, double fps, int cube);
void    run_camera(CameraHandle_C p);

#ifdef __cplusplus
}
#endif

#endif
