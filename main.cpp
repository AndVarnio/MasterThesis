// #include "HSIInterfaceC.h"
#include "HSICamera.hpp"
#include <stdlib.h>
#include <stdio.h>

// /home/andreas/gcc-linaro-7.3.1-2018.05-i686_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++ -O3 main.cpp HSICamera.cpp CubeDMADriver.cpp -o cubeCapture -fopenmp -mfpu=neon -I usr/include usr/lib/libueye_api.so.4.90
int main(int argc, char** argv){

  HSICamera camera;

  camera.initialize(10, 1024, 1024, 500, 20, None);

  camera.runCubeCapture();
  printf("Successss\n");

  return 0;
}
