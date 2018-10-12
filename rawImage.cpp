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
// https://github.com/StevenPuttemans/opencv_tryout_code/blob/master/camera_interfacing/IDS_uEye_camera.cpp
int main(){
  HIDS hCam = 1;
  is_SetErrorReport (hCam, IS_ENABLE_ERR_REP);
  INT something = is_InitCamera(&hCam, NULL);
  printf("Status Init %d\n",something);

  UINT nPixelClockDefault = 30;
  is_PixelClock(hCam, IS_PIXELCLOCK_CMD_SET,
                        (void*)&nPixelClockDefault,
                        sizeof(nPixelClockDefault));
  is_SetDisplayMode(hCam, IS_SET_DM_DIB);
  UINT formatID = 13;
  is_ImageFormat (hCam, IMGFRMT_CMD_SET_FORMAT, &formatID, 4);
  double expTime = 0.01;
  is_Exposure(hCam, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*)&expTime, sizeof(expTime));
  INT colorMode = IS_CM_MONO8;
  is_SetColorMode(hCam,colorMode);

  char* pMem = NULL;
  int memID = 0;
  is_AllocImageMem(hCam, 640, 480, 8, &pMem, &memID);
  is_SetImageMem(hCam, pMem, memID);
  is_FreezeVideo(hCam, IS_WAIT);


  std::ofstream ofs;

  ofs.open( "timeChar.raw", std::ofstream::binary );
  ofs.write( pMem, 640*480 );
  ofs.close();
  return 0;
}
