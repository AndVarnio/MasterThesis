#include "HSICamera.hpp"
// g++ test.cpp -lueye_api
int main(){
  HSICamera camera;
  // camera.initialize(pixelclock, resolution, exposure time, rows, columns, bits);
  // resolution: 13=640x480/6=1920x1080
  camera.initialize(344, 6, 50, 1080, 1920, 8);
  camera.runCubeCapture();
  return 0;
}
