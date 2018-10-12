// #include <iostream>
// #include <uEye.h>
//
// int main(){
//  std::cout<<"HElo\n";
// }


#include<stdio.h>
#include<stddef.h>
#include<ueye.h>


HIDS hCam = 1;

int main() {
  is_SetErrorReport (hCam, IS_ENABLE_ERR_REP);
  printf("Success-Code: %d\n",IS_SUCCESS);
  //Init camera
  INT nRet = is_InitCamera (&hCam, NULL);
  printf("Status Init %d\n",nRet);

  UINT nCaps = 0;
  double expTime = 0.018;
  nRet = is_Exposure(hCam, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*)&expTime, sizeof(expTime));
  // nRet = is_Exposure(hCam, IS_EXPOSURE_CMD_GET_EXPOSURE, (void*)&nCaps, sizeof(nCaps));
  printf("Exposure time %d\n",nRet);

  INT nGamma = 100;
  nRet = is_Gamma(hCam, IS_GAMMA_CMD_SET, (void*) &nGamma, sizeof(nGamma));


  nRet = is_Gamma(hCam, IS_GAMMA_CMD_GET, (void*) &nGamma, sizeof(nGamma));
  printf("Gamma %d\n",nGamma);


  // nRet = is_SetSaturation (hCam, 100, 100);
  // printf("nRet saturation  %d\n",nRet);

  nRet = is_SetGainBoost (hCam, IS_SET_GAINBOOST_ON);
  printf("Gain boost %d\n",nRet);
  //Pixel-Clock set
  UINT nPixelClockDefault = 30;
  nRet = is_PixelClock(hCam, IS_PIXELCLOCK_CMD_SET,
                        (void*)&nPixelClockDefault,
                        sizeof(nPixelClockDefault));

  printf("Status is_PixelClock %d\n",nRet);
  
  //Colormode
  // INT colorMode = IS_CM_MONO8;
  // INT colorMode = IS_CM_CBYCRY_PACKED;
  INT colorMode = IS_CM_BGR8_PACKED;

  nRet = is_SetColorMode(hCam,colorMode);
  printf("Status SetColorMode %d\n",nRet);

  UINT formatID = 6;
  //Bildgröße einstellen -> 2592x1944
  nRet = is_ImageFormat(hCam, IMGFRMT_CMD_SET_FORMAT, &formatID, 4);
  printf("Status ImageFormat %d\n",nRet);

  //Speicher für Bild alloziieren
  char* pMem = NULL;
  int memID = 0;
  nRet = is_AllocImageMem(hCam, 1920, 1080, 16, &pMem, &memID);
  printf("Status AllocImage %d\n",nRet);

  //diesen Speicher aktiv setzen
  nRet = is_SetImageMem(hCam, pMem, memID);
  printf("Status SetImageMem %d\n",nRet);

  //Bilder im Kameraspeicher belassen
  INT displayMode = IS_SET_DM_DIB;
  nRet = is_SetDisplayMode (hCam, displayMode);
  printf("Status displayMode %d\n",nRet);

  //Bild aufnehmen
  nRet = is_FreezeVideo(hCam, IS_WAIT);
  printf("Status is_FreezeVideo %d\n",nRet);

  //Bild aus dem Speicher auslesen und als Datei speichern
  IMAGE_FILE_PARAMS ImageFileParams;
  ImageFileParams.pwchFileName = L"./snap_BGR8.png";
  ImageFileParams.pnImageID = NULL;
  ImageFileParams.ppcImageMem = NULL;
  ImageFileParams.nQuality = 0;
  ImageFileParams.nFileType = IS_IMG_PNG;

  nRet = is_ImageFile(hCam, IS_IMAGE_FILE_CMD_SAVE, (void*) &ImageFileParams, sizeof(ImageFileParams));
  printf("Status is_ImageFile %d\n",nRet);

  ImageFileParams.pwchFileName = L"./snap_BGR8.bmp";
  ImageFileParams.pnImageID = NULL;
  ImageFileParams.ppcImageMem = NULL;
  ImageFileParams.nQuality = 0;
  ImageFileParams.nFileType = IS_IMG_BMP;

  nRet = is_ImageFile(hCam, IS_IMAGE_FILE_CMD_SAVE, (void*) &ImageFileParams, sizeof(ImageFileParams));
  printf("Status is_ImageFile %d\n",nRet);

  ImageFileParams.pwchFileName = L"./snap_BGR8.jpg";
  ImageFileParams.pnImageID = NULL;
  ImageFileParams.ppcImageMem = NULL;
  ImageFileParams.nQuality = 0;
  ImageFileParams.nFileType = IS_IMG_JPG;

  nRet = is_ImageFile(hCam, IS_IMAGE_FILE_CMD_SAVE, (void*) &ImageFileParams, sizeof(ImageFileParams));
  printf("Status is_ImageFile %d\n",nRet);

  //Kamera wieder freigeben
  is_ExitCamera(hCam);

  return 0;
}
