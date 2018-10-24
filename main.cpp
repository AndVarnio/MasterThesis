#include "HSICamera.hpp"
// g++ test.cpp -lueye_api
// g++ main.cpp -lueye_api `pkg-config --cflags --libs opencv`
int main(){
  HSICamera camera;
  // camera.initialize(pixelclock, resolution, exposure time, rows, columns, bits);
  // resolution: 13=640x480/6=1920x1080
  // camera.initialize(232, 6, 50, 1080, 1920, 8); //ui336
  // camera.initialize(344, 6, 30, 1080, 1920, 8); //ui336
  // camera.initialize(237, 6, 20, 1080, 1920, 8); //ui306
  camera.initialize(344, 6, 60, 1080, 1920, 8);
  camera.runCubeCapture();
  return 0;
}
