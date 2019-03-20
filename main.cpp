// #include "HSICamera.hpp"
#include "HSICameraPC.hpp"

#include <stdlib.h>
// g++ test.cpp -lueye_api
// g++ main.cpp -lueye_api `pkg-config --cflags --libs opencv`
int main(int argc, char** argv){
  HSICamera camera;
  // camera.initialize(pixelclock, resolution, exposure time, rows, columns, frames, fps, cameramode, cubeformat);
  // resolution: 13=640x480/6=1920x1080/36=1936x1216
  // camera.initialize(232, 6, 50, 1080, 1920, 8); //ui336
  // camera.initialize(344, 6, 30, 1080, 1920, 8); //ui336
  // camera.initialize(237, 6, 20, 1080, 1920, 8); //ui306
  // camera.initialize(118, 36, 30, 1216, 1936, 2254, Freerun, Bil); //GOAL parmeters
  camera.initialize(232, 28, 30, 1024, 1024, 1500, 32, Freerun, Bil);
  // camera.initialize(118, 6, 5, 1080, 1920, 2254, Freerun, Bil);

  camera.runCubeCapture();
  printf("Successss\n");
  // camera.captureSingleImage();
  return 0;
}
