// #include "HSIInterfaceC.h"
#include "HSICamera.hpp"
// #include "HSICameraPC.hpp"
// #include "HSI8bit.hpp"
#include <stdlib.h>
#include <stdio.h>
// g++ test.cpp -lueye_api
// g++ main.cpp -lueye_api `pkg-config --cflags --libs opencv`
// /home/andreas/gcc-linaro-7.3.1-2018.05-i686_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++ -O3 main.cpp HSICamera.cpp CubeDMADriver.cpp -o cubeCapture -fopenmp -mfpu=neon -I usr/include usr/lib/libueye_api.so.4.90
int main(int argc, char** argv){

  // MHandle h = create_magic();
  // initialize_magic(h, 10, 1936, 1216, 500, 6, Raw);
  // run_magic(h);

  HSICamera camera;
  // camera.initialize(pixelclock, exposure time, rows, columns, frames, fps, cameramode, cubeformat);
  // resolution: 13=640x480/6=1920x1080/36=1936x1216
  // camera.initialize(232, 6, 50, 1080, 1920, 8); //ui336
  // camera.initialize(344, 6, 30, 1080, 1920, 8); //ui336
  // camera.initialize(237, 6, 20, 1080, 1920, 8); //ui306
  // camera.initialize(118, 36, 30, 1216, 1936, 2254, Freerun, Bil); //GOAL parmeters

  camera.initialize(10, 1024, 1024, 500, 20, None);
  // // camera.initialize(118, 6, 5, 1080, 1920, 2254, 25, Freerun, Bil);
  //
  camera.runCubeCapture();
  printf("Successss\n");
  // camera.captureSingleImage();
  return 0;
}

// Image packing configureations -> Root file system type | SD card
