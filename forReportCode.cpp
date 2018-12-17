#include<ueye.h>
#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include<time.h>
#include <sys/time.h>
#include <arm_neon.h>
// #include <opencv2/highgui.hpp>

class HSICamera
{
    public:
      HSICamera();
      void initialize(int pixelClockMHz, int resolution, double exposureMs, int rows, int columns, int pixelDepth);
      void runCubeCapture();
      void captureSingleImage();

    private:
      HIDS hCam = 1;
      char* memSingleImage = NULL;
      int memIDSingle = 0;

      int sensorRows;
      int sensorColumns;
      int bitDepth;

      int cubeColumns;
      int cubeRows = 25;

      const int nSingleFrames = 1735;
      const int nRawImagesInMemory = 100;
      double frameRate = 32.0;

      int bands;
      int nBandsBinned;
      int factorLastBands = 0;
      int nFullBinnsPerRow = 0;
      int binningFactor = 20;
      bool meanBinning = true;
      char cubeType = 'c'; //a=bil b=bip c=bsq
      char** memSingleImageSequence = new char*[nRawImagesInMemory];
      char **hsiCube;
      unsigned char** binnedImages;

      void writeCubeToFile();
      void writeSingleToFile();
      void writeBandsToSeparateFiles();
      void writeRawDataToFile(char** rawImages, int nRows, int nColumns);

      int random_partition(unsigned char* arr, int start, int end);
      int random_selection(unsigned char* arr, int start, int end, int k);
      void insertionSort(unsigned char arr[], int n);
      void swap(unsigned char* a, unsigned char* b);
      int partition (unsigned char arr[], int low, int high);
      void quickSort(unsigned char arr[], int low, int high);
};

HSICamera::HSICamera(){
  is_SetErrorReport (hCam, IS_ENABLE_ERR_REP);
  INT success = is_InitCamera(&hCam, NULL);
  if(success!=IS_SUCCESS){
    printf("Failed to initialize camera!\n");
  }
}

void HSICamera::initialize(int pixelClockMHz, int resolution, double exposureMs, int rows, int columns, int pixelDepth){

  printf("Initializing camera parameters\n");
  sensorRows = rows;
  sensorColumns = columns;
  bands = columns;
  nBandsBinned = bands;
  bitDepth = pixelDepth;
  UINT nPixelClock = pixelClockMHz;

  ////////////////////Binning////////////////////
  factorLastBands = bands%binningFactor;
  nFullBinnsPerRow = bands/binningFactor;
  nBandsBinned = (bands + binningFactor - 1) / binningFactor;

  if(cubeType=='a' || cubeType=='b'){

    cubeColumns = sensorRows*nBandsBinned;
    cubeRows = nSingleFrames;

  }
  else{//BSQ
    cubeColumns = sensorRows;
    cubeRows = nSingleFrames*nBandsBinned;
  }

  ////////////////////Initialize///////////////////
  int errorCode = is_PixelClock(hCam, IS_PIXELCLOCK_CMD_SET,
                        (void*)&nPixelClock,
                        sizeof(nPixelClock));
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the pixel clock, error code: %d\n", errorCode);
  };

  errorCode = is_SetDisplayMode(hCam, IS_SET_DM_DIB);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the display mode, error code: %d\n", errorCode);
  };

  UINT formatID = resolution;
  errorCode = is_ImageFormat (hCam, IMGFRMT_CMD_SET_FORMAT, &formatID, 4);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the resolution setup, error code: %d\n", errorCode);
  };

  double expTime = exposureMs;
  errorCode = is_Exposure(hCam, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*)&expTime, sizeof(expTime));
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the exposure time, error code: %d\n", errorCode);
  };

  errorCode = is_SetColorMode(hCam, IS_CM_MONO8);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the color mode, error code: %d\n", errorCode);
  };

  errorCode = is_SetGainBoost(hCam, IS_SET_GAINBOOST_ON);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the gainboost, error code: %d\n", errorCode);
  };

  errorCode = is_SetHardwareGain (hCam, 50, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the hardware gain, error code: %d\n", errorCode);
  };
  /////////////Set imagememory for triggermode//////////////////////
/*
  is_AllocImageMem(hCam, sensorColumns, sensorRows, bitDepth, &memSingleImage, &memIDSingle);
  is_SetImageMem(hCam, memSingleImage, memIDSingle);
  printf("Allocated memory for single image\n");
  is_SetExternalTrigger(hCam, IS_SET_TRIGGER_SOFTWARE);
*/

  /////////////Set imagememory for freerun mode//////////////////////
  for(int imageMemory=1; imageMemory<=nRawImagesInMemory; imageMemory++){
    // printf("Adding imagememory %i\n", imageMemory);
    is_AllocImageMem(hCam, sensorColumns, sensorRows, bitDepth, &memSingleImageSequence[imageMemory], &imageMemory);
    is_AddToSequence (hCam, memSingleImageSequence[imageMemory], imageMemory);
  }
  is_InitImageQueue (hCam, 0);

  binnedImages = new unsigned char*[nSingleFrames];
  for(int image=0; image<nSingleFrames; image++){
    binnedImages[image] = new unsigned char[nBandsBinned*sensorRows];//TODO pixeldepth
  }

  errorCode = is_SetFrameRate(hCam, frameRate, &frameRate);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with setting the framerate, error code: %d\n", errorCode);
  };

  errorCode = is_CaptureVideo (hCam, IS_WAIT);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with putting camera in freerun mode, error code: %d\n", errorCode);
  };

}

void HSICamera::runCubeCapture(){

  /////////////////Freerun mode //////////////
  char* rawImageP;
  int imageSequenceID = 1;
  for(int imageNumber=0; imageNumber<nSingleFrames; imageNumber++){
    is_WaitForNextImage(hCam, 1000, &(rawImageP), &imageSequenceID);

    /////Binning
    #pragma omp parallel for num_threads(2)
    for(int row=0; row<sensorRows; row++){
      int rowOffset = row*sensorColumns;
      int binnedIdxOffset = row*nBandsBinned;

      int binOffset = 0;
      for(int binnIterator=0; binnIterator<nFullBinnsPerRow; binnIterator++){
        int rowAndBinOffset = rowOffset+binOffset;

        uint8x8_t vec1_16x8, vec2_16x8, vec3_16x8;
        vec1_16x8 = vld1_u8((uint8_t*)(rawImageP+rowAndBinOffset));
        vec2_16x8 = vld1_u8((uint8_t*)(rawImageP+rowAndBinOffset+8));
        vec3_16x8 = vadd_u8(vec1_16x8, vec2_16x8);

        vec1_16x8 = vld1_u8((uint8_t*)(rawImageP+16));
        vec2_16x8 = vadd_u8(vec1_16x8, vec3_16x8);

        uint8_t shiftTmp[8] = {vget_lane_u8(vec3_16x8, 4), vget_lane_u8(vec3_16x8, 5), vget_lane_u8(vec3_16x8, 6), vget_lane_u8(vec3_16x8, 7), 0, 0, 0, 0};
        vec3_16x8 = vld1_u8(shiftTmp);
        vec1_16x8 = vadd_u8(vec3_16x8, vec2_16x8);

        uint16_t totPixVal = vget_lane_u8(vec1_16x8, 0) + vget_lane_u8(vec1_16x8, 1) + vget_lane_u8(vec1_16x8, 2) + vget_lane_u8(vec1_16x8, 3);
        binOffset += binningFactor;
        binnedImages[imageNumber][binnedIdxOffset+binnIterator] = (unsigned char)(totPixVal/binningFactor);
      }
      if(factorLastBands>0){
        char totPixVal = 0;
        for(int pixelIterator=0; pixelIterator<factorLastBands; pixelIterator++){
          totPixVal = rawImageP[nFullBinnsPerRow*binningFactor+pixelIterator];
        }
        binnedImages[imageNumber][binnedIdxOffset+nFullBinnsPerRow] = rawImageP[nFullBinnsPerRow*binningFactor];
      }
    }

    is_UnlockSeqBuf (hCam, 1, rawImageP);
  }


  for(int imageMemory=1; imageMemory<=nRawImagesInMemory; imageMemory++){
    is_FreeImageMem (hCam, memSingleImageSequence[imageMemory], imageMemory);
  }

  hsiCube = new char*[cubeRows];
  for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
    hsiCube[cubeRow] = new char[cubeColumns];//TODO pixeldepth
  }

  ///Make cube
  if(cubeType=='a'){//BIL
    for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
      for(int band=0; band<nBandsBinned; band++){
        for(int pixelInCubeRow=0; pixelInCubeRow<sensorRows; pixelInCubeRow++){
          hsiCube[cubeRow][band*sensorRows+pixelInCubeRow] = binnedImages[cubeRow][nBandsBinned*(sensorRows-1)-nBandsBinned*pixelInCubeRow+band];
        }
      }
    }
  }
  else if(cubeType=='b'){//BIP
    for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
      for(int pixel=0; pixel<sensorRows; pixel++){
        for(int band=0; band<nBandsBinned; band++){
          hsiCube[cubeRow][pixel*nBandsBinned+band] = binnedImages[cubeRow][nBandsBinned*(sensorRows-1)-nBandsBinned*pixel+band];
        }
      }
    }
  }
  else{//BSQ
    for(int band=0; band<nBandsBinned; band++){
      for(int rowSpatial=0; rowSpatial<nSingleFrames; rowSpatial++){
        for(int pixelInCubeRow=0; pixelInCubeRow<cubeColumns; pixelInCubeRow++){
          hsiCube[band*nSingleFrames+rowSpatial][pixelInCubeRow] = binnedImages[rowSpatial][nBandsBinned*(sensorRows-1)-nBandsBinned*pixelInCubeRow+band];
        }
      }
    }
  }

  /////////////////////////////////Trigger mode /////////////////////
/*
  if(1){//if this is bsq
    for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
      is_FreezeVideo(hCam, IS_WAIT);
        //TODO Binning

      for(int band=0; band<bands; band++){
        for(int pixelInCubeRow=0; pixelInCubeRow<sensorRows; pixelInCubeRow++){
          hsiCube[cubeRow][band*sensorRows+pixelInCubeRow] = memSingleImage[sensorColumns*(sensorRows-1)-sensorColumns*pixelInCubeRow+band];
        }
      }
        // usleep(captureInterval);
    }
  }
*/
  writeCubeToFile();
}

void HSICamera::writeRawDataToFile(char** rawImages, int nRows, int nColumns){

  struct timespec timeSystem;
  clock_gettime(CLOCK_REALTIME, &timeSystem);
  char timeSystemString[32];
  sprintf(timeSystemString, "%lli", (long long)timeSystem.tv_nsec);
  char filePath[64];
  strcpy(filePath, "/home/capture/");
  // strcpy(filePath, "/home/andreas/HSIProject/capture/");
  strcat(filePath, timeSystemString);
  strcat(filePath, "SensorData.raw");

  FILE * fp;
  fp = fopen (filePath,"wb");

  for(int i=0; i<nRows; i++){
    fwrite (rawImages[i], sizeof(char), nColumns, fp);//TODO bitDepth
  }
  fclose (fp);

}

void HSICamera::writeCubeToFile(){
  struct timespec timeSystem;
  clock_gettime(CLOCK_REALTIME, &timeSystem);
  char timeSystemString[32];
  sprintf(timeSystemString, "%lli", (long long)timeSystem.tv_nsec);
  char filePath[64];
  strcpy(filePath, "/home/root/capture/");
  // strcpy(filePath, "/home/andreas/HSIProject/capture/");
  strcat(filePath, timeSystemString);
  strcat(filePath, "Cube.raw");

  FILE * fp;
  fp = fopen (filePath,"wb");

  if (fp==NULL){
    printf("\nFile cant be opened");
    return;
  }
  for(int i=0; i<cubeRows; i++){
      fwrite (hsiCube[i], sizeof(char), cubeColumns, fp);//TODO bitDepth
    }
    fclose (fp);
}



void HSICamera::captureSingleImage(){
  is_FreezeVideo(hCam, IS_WAIT);
  writeSingleToFile();
}

void HSICamera::writeSingleToFile(){

  struct timespec timeSystem;
  clock_gettime(CLOCK_REALTIME, &timeSystem);
  char timeSystemString[32];
  sprintf(timeSystemString, "%lli", (long long)timeSystem.tv_nsec);
  char filePath[64];
  strcpy(filePath, "/home/capture/");
  // strcpy(filePath, "/home/andreas/HSIProject/capture/");
  strcat(filePath, timeSystemString);
  strcat(filePath, "Single.raw");

  FILE * fp;
  fp = fopen (filePath,"wb");
  fwrite (memSingleImage, sizeof(char), sensorColumns*sensorRows, fp);//TODO bitDepth
  fclose (fp);
}

void HSICamera::writeBandsToSeparateFiles(){

/*
  auto time_now = std::time(0);
  std::stringstream ss;
  ss << time_now;
  std::string timeString = ss.str();
  std::string newDirectory = "mkdir -p ./capture/"+timeString;
  system(newDirectory.c_str());

  std::vector<cv::Mat> rgbChannels;


  char grayScaleImage[nSingleFrames*sensorRows];
  char rgbImage[3][nSingleFrames*sensorRows];
  int bandIterator = 0;
  if(1){ //This is bsq
    for(int band=0; band<nBandsBinned; band++){
      for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
        for(int sensorRow=0; sensorRow<sensorRows; sensorRow++){
          grayScaleImage[cubeRow*sensorRows+sensorRow] = hsiCube[cubeRow][band*sensorRows+sensorRow];
        }
      }
      std::string filename = "./capture/" + timeString + "/" + std::to_string(band) + ".png";
      cv::Mat grayScaleMat = cv::Mat(nSingleFrames, sensorRows, CV_8UC1, &grayScaleImage);
      imwrite(filename,grayScaleMat);
      */
      //Make RGB image
/*
      if(band==292||band==734||band==1147){
        for(int i=0; i<nSingleFrames*sensorRows; i++){
          rgbImage[bandIterator][i] = grayScaleImage[i];
        }

        bandIterator++;

        if(band==1147){
          rgbChannels.push_back(cv::Mat(nSingleFrames, sensorRows, CV_8UC1, &rgbImage[0]));
          rgbChannels.push_back(cv::Mat(nSingleFrames, sensorRows, CV_8UC1, &rgbImage[1]));
          rgbChannels.push_back(cv::Mat(nSingleFrames, sensorRows, CV_8UC1, &rgbImage[2]));

          cv::Mat rgbImage;
          cv::merge(rgbChannels, rgbImage);
          filename = "./capture/" + timeString + "/RedGreenBlue" + ".png";
          imwrite(filename,rgbImage);
        }
      }
*/
    // }
  // }
}
