#include "HSIInterfaceC.h"
#include "HSICamera.hpp"

extern "C"
{
    CameraHandle_C create_camera_handle() {
      return new HSICamera();
    }

    void  initialize_camera(CameraHandle_C p, double exposureMs, int rows, int columns, int frames, double fps, int cube) {
      return ((HSICamera *)p)->initialize(exposureMs, rows, columns, frames, fps, cube);
    }

    void run_camera(CameraHandle_C p) {
      return ((HSICamera *)p)->runCubeCapture();
    }
}
