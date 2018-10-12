#include<stdio.h>
#include<stddef.h>
#include<ueye.h>


HIDS hCam = 1;

int main() {
  printf("Success-Code: %d\n",IS_SUCCESS);
  //Kamera öffnen
  INT nRet = is_InitCamera (&hCam, NULL);
  printf("Status Init %d\n",nRet);

  //Pixel-Clock setzen
  UINT nPixelClockDefault = 9;
  nRet = is_PixelClock(hCam, IS_PIXELCLOCK_CMD_SET,
                        (void*)&nPixelClockDefault,
                        sizeof(nPixelClockDefault));

  printf("Status is_PixelClock %d\n",nRet);

  //Farbmodus der Kamera setzen
  //INT colorMode = IS_CM_CBYCRY_PACKED;
  INT colorMode = IS_CM_BGR8_PACKED;

  nRet = is_SetColorMode(hCam,colorMode);
  printf("Status SetColorMode %d\n",nRet);

  UINT formatID = 4;
  //Bildgröße einstellen -> 2592x1944
  nRet = is_ImageFormat(hCam, IMGFRMT_CMD_SET_FORMAT, &formatID, 4);
  printf("Status ImageFormat %d\n",nRet);

  //Speicher für Bild alloziieren
  char* pMem = NULL;
  int memID = 0;
  nRet = is_AllocImageMem(hCam, 2592, 1944, 16, &pMem, &memID);
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
