#include<stdio.h>
#include<stddef.h>
#include<ueye.h>
#include<iostream>
#include<string.h>
#include <ctime>
#include<iomanip>
#include <string.h>
#include <sstream>
#include <fstream>

const int cubeRows = 1000;
char *hsiCube[cubeRows];
const int sensorRows = 480;
const int sensorColumns = 640;
const int bitDepth = 8;

void addImageToCube();
void writeCubeToFile();

int main(){
  /////////////INIT CAMERA//////////////
  HIDS hCam = 1;
  is_SetErrorReport (hCam, IS_ENABLE_ERR_REP);
  INT something = is_InitCamera(&hCam, NULL);

  UINT nPixelClockDefault = 30;
  is_PixelClock(hCam, IS_PIXELCLOCK_CMD_SET,
                        (void*)&nPixelClockDefault,
                        sizeof(nPixelClockDefault));
  is_SetDisplayMode(hCam, IS_SET_DM_DIB);//IS_SET_DM_DIB = Captures an image in system memory (RAM)
  UINT formatID = 13;//Pixel resolution
  is_ImageFormat (hCam, IMGFRMT_CMD_SET_FORMAT, &formatID, 4);

  double expTime = 0.01;
  is_Exposure(hCam, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*)&expTime, sizeof(expTime));

  //Colormode
  // INT colorMode = IS_CM_MONO8;
  // INT colorMode = IS_CM_CBYCRY_PACKED;
  INT colorMode = IS_CM_MONO8;
  is_SetColorMode(hCam,colorMode);
  ///////////////////////////////////////

  ////////////////Alloc mem///////////////
  char* pMem = NULL;
  int memID = 0;
  is_AllocImageMem(hCam, sensorColumns, sensorRows, bitDepth, &pMem, &memID);
  is_SetImageMem(hCam, pMem, memID);
  ///////////////////////////////////////



/////capture image ///////
  for(int i=0; i<10; i++){
    is_FreezeVideo(hCam, IS_WAIT);

  }
  return 0;
}

void addImageToCube(& rowOfPixels){
  hsiCube[rowIndexCube] = rowOfPixels;
  rowIndexCube++;
}

void writeCubeToFile(){
  std::ofstream ofs;
  auto time_now = std::time(0);
  std::stringstream ss;
  ss << std::put_time(std::gmtime(&time_now), "%c %Z");
  std::string timeString = ss.str();
  timeString = "./capture/" + timeString + ".raw";

  ofs.open( timeString, std::ofstream::binary );
  ofs.write( pMem, sensorColumns*sensorRows );//TODO bitDepth
  ofs.close();
  usleep(1000*1000);
}
