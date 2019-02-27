// #include "hsiWorksForARM_SIMD.hpp"
// #include "HSICameraARMGigE8Bit.hpp"
// #include "HSICameraARMGigE.hpp"
// #include "HSICameraGigEHWTrigger.hpp"
#include "HSICameraARMSubsampling.hpp"
// #include "testMock16BitPC.hpp"
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
  // TODO Different pixelclock USB 2.0
  // camera.initialize(118, 36, 30, 1216, 1936, 2254, Freerun, Bil); //GOAL parmeters
  // camera.initialize(50, 6, 5, 1080, 1920, 2254, Freerun, Bil);
  camera.initialize(118, 36, 5, 1080, 1920, 1000, 23, Freerun, Bsq);
  // camera.initialize(118, 6, 5, 1080, 1920, 2254, Freerun, Bil);
  // camera.initialize(118, 6, 5, 1080, 1920, 8); //ui306
  camera.runCubeCapture();
  printf("Successss\n");
  // camera.captureSingleImage();
  return 0;
}


// The task is to set up a iDS camera on a Zynq-7000 SoC for capturing of hyperspectral images. With the goal of capturing 2254 12-bit images at a resolution of 1216x1936 with a framerate of 32fps. Aslo investigate how to implement other previous work into the pipeline. Subtasks for the project in prioritized order: 1 - Test the new camera and with new parameters for; resolution, number of frames and 12-bit. 2 - Memory management 3 - Implement and test HW triggering 4 - Median binning of images 5 - CCSDS 123 compression on L1A data
