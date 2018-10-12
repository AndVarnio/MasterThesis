#include<stdio.h>
#include<stddef.h>
#include<ueye.h>
#include<iostream>
#include<string.h>
#include <ctime>
#include<iomanip>
#include <string.h>
#include <sstream>
// g++ test.cpp -lueye_api

int main() {
HIDS hCam = 1;
INT something = is_InitCamera(&hCam, NULL);
is_SetErrorReport (hCam, IS_ENABLE_ERR_REP);
INT nRet;
UINT nPixelClockDefault = 38;

// 0.018
nRet = is_PixelClock(hCam, IS_PIXELCLOCK_CMD_SET,
                      (void*)&nPixelClockDefault,
                      sizeof(nPixelClockDefault));
printf("Pixelclock %d\n",nRet);
//////////////////////////////Get last error message from camera////////////////////////////////////////
const unsigned short bufferLen = 128;
char myErrorBuffer[bufferLen]; // You can also allocate the string dynamically
char* lastErrorString;
int lastError;
nRet = is_GetError(hCam, &lastError, &lastErrorString);
memset(myErrorBuffer, 0, bufferLen);
strncpy(myErrorBuffer, lastErrorString, bufferLen);

// char **foo = (char *[]){"we", "dd"};
// is_GetError (1, &something, foo);
// printf ("%s", &(*foo)[0]);

std::cout << myErrorBuffer << std::endl;
std::cout << lastErrorString << std::endl;
///////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////Get camera info////////////////////////////////////////////
SENSORINFO infStructure;

something = is_GetSensorInfo (hCam, &infStructure);
std::cout << "SensorID: " << infStructure.SensorID << std::endl;
std::cout << "strSensorName: " << infStructure.strSensorName << std::endl;
std::cout << "nColorMode: " << infStructure.nColorMode << std::endl;
if(infStructure.nColorMode == IS_COLORMODE_MONOCHROME){
    std::cout << "Color mode is monochrome." << std::endl;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////Capture image/////////////////////////////////

double expTime = 100;
int errorCode = is_Exposure(hCam, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*)&expTime, sizeof(expTime));
if(errorCode!=IS_SUCCESS){
  printf("Something went wrong with the exposure time, error code: %d\n", errorCode);
};

//  IS_CM_MONO8
is_SetColorMode(hCam, IS_CM_MONO8);
UINT formatID = 6;
is_ImageFormat (hCam, IMGFRMT_CMD_SET_FORMAT, &formatID, 4);

char* pMem = NULL;
int memID = 0;
is_AllocImageMem(hCam, 1920, 1080, 8, &pMem, &memID);
is_SetImageMem(hCam, pMem, memID);
is_SetDisplayMode(hCam, IS_SET_DM_DIB);
is_FreezeVideo(hCam, IS_WAIT);

auto time_now = std::time(0);
std::stringstream ss;
ss << std::put_time(std::gmtime(&time_now), "%c %Z");
std::string timeString = ss.str();
timeString = "./capture/" + timeString + ".bmp";

wchar_t* wide_string = new wchar_t[ timeString.length() + 1 ];
std::copy( timeString.begin(), timeString.end(), wide_string );
wide_string[ timeString.length() ] = 0;

IMAGE_FILE_PARAMS ImageFileParams;
ImageFileParams.pwchFileName = wide_string;
ImageFileParams.pnImageID = NULL;
ImageFileParams.ppcImageMem = NULL;
ImageFileParams.nQuality = 0;
ImageFileParams.nFileType = IS_IMG_BMP;

nRet = is_ImageFile(hCam, IS_IMAGE_FILE_CMD_SAVE, (void*) &ImageFileParams, sizeof(ImageFileParams));

printf("Status is_ImageFile %d\n",nRet);
//////////////////////////////////////////////////////////////////////////////////////

if (something == IS_NO_SUCCESS){
  std::cout << "No success!\n";
}
else{
  std::cout << "Success???\n";
}

// GetCameraID();
return 0;
}
