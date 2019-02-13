#include<ueye.h>
#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include<time.h>
#include <sys/time.h>
#include <arm_neon.h>
// #include <opencv2/highgui.hpp>
// 226x226
enum cubeFormat { Bil, Bip, Bsq };
enum cameraTriggerMode {Freerun, Swtrigger};

class HSICamera
{

    public:
      HSICamera();
      void initialize(int pixelClockMHz, int resolution, double exposureMs, int rows, int columns, int frames, cameraTriggerMode cameraMode, cubeFormat cube);
      void runCubeCapture();
      void captureSingleImage();

    private:
      HIDS hCam = 1;
      char* memSingleImage = NULL;
      int memIDSingle = 0;
      cameraTriggerMode triggerMode;

      int sensorRows;
      int sensorColumns;
      int bitDepth = 8;

      int cubeColumns;
      int cubeRows = 0;

      int nSingleFrames = 0;
      const int nRawImagesInMemory = 10;
      double frameRate = 38.0;
      double newFrameRate;

      int bands;
      int nBandsBinned;
      int factorLastBands = 0;
      int nFullBinnsPerRow = 0;
      int binningFactor = 12;
      bool meanBinning = true;
      cubeFormat cubeType = Bsq; //a=bil b=bip c=bsq
      char** memSingleImageSequence = new char*[nRawImagesInMemory];
      uint8_t **hsiCube;
      uint8_t** binnedImages;

      void swTriggerCapture();
      void freeRunCapture();
      void writeCubeToFile();
      void writeSingleToFile();
      void writeBandsToSeparateFiles();
      void writeRawDataToFile(char** rawImages, int nRows, int nColumns);
      void writeRawDataToFile(uint8_t* image, int imageNumber);

      int random_partition(unsigned char* arr, int start, int end);
      int random_selection(unsigned char* arr, int start, int end, int k);
      void insertionSort(uint8_t* arr, int startPosition, int n);
      // void swap(unsigned char* a, unsigned char* b);
      int partition (unsigned char arr[], int low, int high);
      void quickSort(unsigned char arr[], int low, int high);
      void bubbleSort(uint8_t arr[], int n);
      void swap(uint8_t *xp, uint8_t *yp);
      void bitonicMerge12(uint8_t* arr);
};

HSICamera::HSICamera(){
  is_SetErrorReport (hCam, IS_ENABLE_ERR_REP);
  INT success = is_InitCamera(&hCam, NULL);
  if(success!=IS_SUCCESS){
    printf("Failed to initialize camera!, error message: %i\n", success);
    exit (EXIT_FAILURE);
  }
}

void HSICamera::initialize(int pixelClockMHz, int resolution, double exposureMs, int rows, int columns, int frames, cameraTriggerMode cameraMode, cubeFormat cube){

  printf("Initializing camera parameters\n");
  sensorRows = rows;
  sensorColumns = columns;
  bands = columns;
  nBandsBinned = bands;
  UINT nPixelClock = pixelClockMHz;
  nSingleFrames = frames;
  triggerMode = cameraMode;
  cubeType = cube;

  ////////////////////Binning////////////////////
  factorLastBands = bands%binningFactor;
  nFullBinnsPerRow = bands/binningFactor;
  nBandsBinned = (bands + binningFactor - 1) / binningFactor;

  if(cubeType==Bil || cubeType==Bip){

    cubeColumns = sensorRows*nBandsBinned;
    cubeRows = nSingleFrames;

  }
  else{//BSQ
    cubeColumns = sensorRows;
    cubeRows = nSingleFrames*nBandsBinned;
  }

  ////////////////////Initialize///////////////////

// Get pixel clock range
  UINT nRange[3];
  ZeroMemory(nRange, sizeof(nRange));
  INT nRet = is_PixelClock(hCam, IS_PIXELCLOCK_CMD_GET_RANGE, (void*)nRange, sizeof(nRange));

  if (nRet == IS_SUCCESS){
    // UINT nMin = nRange[0];
    // UINT nMax = nRange[1];
    // UINT nInc = nRange[2];
    printf("min: %d max: %d intervall: %d\n", nRange[0], nRange[1], nRange[2]);
  }
  int errorCode = is_PixelClock(hCam, IS_PIXELCLOCK_CMD_SET,
                        (void*)&nPixelClock,
                        sizeof(nPixelClock));
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the pixel clock, error code: %d\n", errorCode);
  };

  double min, max, intervall;
  is_GetFrameTimeRange (hCam, &min, &max, &intervall);
  printf("min: %f max: %f intervall: %f\n", min, max, intervall);

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

  // errorCode = is_SetGainBoost(hCam, IS_SET_GAINBOOST_ON);
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with the gainboost, error code: %d\n", errorCode);
  // };
  //
  // errorCode = is_SetHardwareGain (hCam, 50, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with the hardware gain, error code: %d\n", errorCode);
  // };

  switch(triggerMode)
  {
    case Swtrigger  :
      is_AllocImageMem(hCam, sensorColumns, sensorRows, bitDepth, &memSingleImage, &memIDSingle);
      is_SetImageMem(hCam, memSingleImage, memIDSingle);
      printf("Allocated memory for single image\n");
      is_SetExternalTrigger(hCam, IS_SET_TRIGGER_SOFTWARE);
      break;

    case Freerun:
    for(int imageMemory=1; imageMemory<=nRawImagesInMemory; imageMemory++){
      // printf("Adding imagememory %i\n", imageMemory);
      is_AllocImageMem(hCam, sensorColumns, sensorRows, 16, &memSingleImageSequence[imageMemory], &imageMemory);
      is_AddToSequence (hCam, memSingleImageSequence[imageMemory], imageMemory);
    }
    is_InitImageQueue (hCam, 0);

    // binnedImages = new uint8_t*[322];
    // for(int image=0; image<322; image++){
    //   binnedImages[image] = new uint8_t[nBandsBinned*sensorRows];//TODO pixeldepth
    // }

    binnedImages = new uint8_t*[nSingleFrames];
    for(int image=0; image<nSingleFrames; image++){
      binnedImages[image] = new uint8_t[nBandsBinned*sensorRows];//TODO pixeldepth
    }

    errorCode = is_SetFrameRate(hCam, frameRate, &newFrameRate);
    if(errorCode!=IS_SUCCESS){
      printf("Something went wrong with setting the framerate, error code: %d\n", errorCode);
    };

    errorCode = is_CaptureVideo (hCam, IS_WAIT);
    if(errorCode!=IS_SUCCESS){
      printf("Something went wrong with putting camera in freerun mode, error code: %d\n", errorCode);
    };
    break;
  }

}

void HSICamera::runCubeCapture(){
  switch(triggerMode)
  {
    case Swtrigger  :
      swTriggerCapture();
      break;

    case Freerun:
      freeRunCapture();
      break;
  }
}

void HSICamera::swTriggerCapture(){
    for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
      is_FreezeVideo(hCam, IS_WAIT);
        //TODO Binning

      for(int band=0; band<bands; band++){
        for(int pixelInCubeRow=0; pixelInCubeRow<sensorRows; pixelInCubeRow++){
          hsiCube[cubeRow][band*sensorRows+pixelInCubeRow] = memSingleImage[sensorColumns*(sensorRows-1)-sensorColumns*pixelInCubeRow+band];
        }
      }
        // usleep(20000);
    }
}
void HSICamera::freeRunCapture(){
  double dblFPS;
  is_GetFramesPerSecond (hCam, &dblFPS);
  printf("Actual framerate: %f\n", dblFPS);

  struct timeval  tv1, tv2, tv3, tv4;
  double totTimeStore = 0;
  double totTimeBin = 0;
  char* rawImageP;
  int imageSequenceID = 0;
  int fileNumber = 0;

  int lastPixelInRowOffset = nFullBinnsPerRow*binningFactor;
  uint8_t* binnedImage = new uint8_t[nBandsBinned*sensorRows];

  for(int imageNumberBase=0; imageNumberBase<2254; imageNumberBase+=322){
  for(int imageNumberOffset=0; imageNumberOffset<322; imageNumberOffset++){
    gettimeofday(&tv1, NULL);
    int errorCode;
    // usleep(30000);
    // printf("Starting image: %d\n", imageNumberBase+imageNumberOffset);
    do{

      errorCode = is_WaitForNextImage(hCam, 1000000, &(rawImageP), &imageSequenceID);

      if(errorCode!=IS_SUCCESS){
        // is_UnlockSeqBuf (hCam, imageSequenceID, rawImageP);
        printf("Something went wrong with the is_WaitForNextImage, error code: %d, image sequence id: %d\n", errorCode, imageSequenceID);
      }
    }while(errorCode!=IS_SUCCESS);

    // uint8_t pointerToNew16BitArray[sensorRows*sensorColumns];
    //
    // for(int i=0; i<sensorRows*sensorColumns; i++){
    //   pointerToNew16BitArray[i] = uint8_t(rawImageP[i*2]) << 8 | rawImageP[i*2+1] ;
    // }

    // gettimeofday(&tv1, NULL);

    // #pragma omp parallel for num_threads(2)
    // for(int row=0; row<sensorRows; row++){
    //   int rowOffset = row*sensorColumns;
    //   int binnedIdxOffset = row*nBandsBinned;
    //
    //   int binOffset = 0;
    //
    //   for(int binnIterator=0; binnIterator<nFullBinnsPerRow; binnIterator++){
    //
    //     int rowAndBinOffset = rowOffset+binOffset;
    //     // printf("rowAndBinOffset %d\n", rowAndBinOffset);
    //
    //     bitonicMerge12(rawImageP+rowAndBinOffset);
    //     binnedImages[imageNumberBase+imageNumberOffset][binnedIdxOffset+binnIterator] = rawImageP[rowAndBinOffset+6];
    //     binOffset += binningFactor;
    //   }
    //   if(factorLastBands>0){
    //     bubbleSort(rawImageP+rowOffset+lastPixelInRowOffset, factorLastBands);
    //     //insertionSort(rawImageP, rowOffset+lastPixelInRowOffset, factorLastBands);
    //     binnedImages[imageNumberBase+imageNumberOffset][binnedIdxOffset+nFullBinnsPerRow] = rawImageP[lastPixelInRowOffset+(factorLastBands/2)];
    //   }
    //
    // }
    // printf("Unlocking image sequence id: %d\n", imageSequenceID);
    is_UnlockSeqBuf (hCam, imageSequenceID, rawImageP);
    gettimeofday(&tv2, NULL);
    // printf("%f\n", (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec));
    totTimeBin += (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
  }

  // gettimeofday(&tv1, NULL);
  // char fileName[64];
  // char imageNumberS[11];
  // sprintf(imageNumberS, "%d", fileNumber);
  // strcpy(fileName, "/home/root/rawCameraData/tmp/");
  // strcat(fileName, imageNumberS);
  // strcat(fileName, "Binned.raw");
  //
  // FILE* pFile2 = fopen ( fileName , "wb" );
  // if (pFile2==NULL) {fputs ("File error\n",stderr); exit (1);}
  //
  // for(int imageNumberOffset=0; imageNumberOffset<322; imageNumberOffset++){
  //   fwrite (binnedImages[imageNumberOffset], sizeof(uint8_t), sensorRows*nFullBinnsPerRow, pFile2);
  // }
  // fclose (pFile2);
  //
  // // usleep(9000);
  // gettimeofday(&tv2, NULL);
  // totTimeStore += (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
  fileNumber += 1;

}
  // printf("Tot time: %f\n", (double) (tv2.tv_usec - tv3.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec));
  printf("avg binning time: %f\n", totTimeBin/nSingleFrames);
  printf("avg storing time: %f\n", totTimeStore/7);

  for(int imageMemory=1; imageMemory<=nRawImagesInMemory; imageMemory++){
    // is_FreeImageMem (hCam, memSingleImageSequence[imageMemory], imageMemory);
  }

  char fileName[64];
  char imageNumberS[11];

  strcpy(fileName, "cube.raw");

  FILE * pFileCube = fopen ( fileName , "wb" );
  if (pFileCube==NULL) {fputs ("File error\n",stderr); exit (1);}

  uint8_t* cubeColumn = new uint8_t[cubeColumns];
  uint8_t* binnedFrame = new uint8_t[cubeColumns];

  uint8_t** binnedFrames = new uint8_t*[322];
  for(int image=0; image<322; image++){
    binnedFrames[image] = new uint8_t[cubeColumns];//TODO pixeldepth
  }

  gettimeofday(&tv1, NULL);

  ///Make cube
  if(cubeType==Bil){//BIL
    for(fileNumber=0; fileNumber<7; fileNumber++){
      strcpy(fileName, "/home/root/rawCameraData/tmp/");
      sprintf(imageNumberS, "%d", fileNumber);
      strcat(fileName, imageNumberS);
      strcat(fileName, "Binned.raw");
      // printf("Cubing file: %s\n", fileName);
      FILE * pFileCubeRow = fopen ( fileName , "rb" );
      if (pFileCubeRow==NULL) {fputs ("File error\n",stderr); exit (1);}
      size_t result = fread (binnedFrame, 2, nFullBinnsPerRow*sensorRows, pFileCubeRow);
      if (result != nFullBinnsPerRow*sensorRows) {fputs ("Reading error",stderr); exit (3);}
      fclose (pFileCubeRow);

      for(int frameNumber=0; frameNumber<322; frameNumber++){
        for(int band=0; band<nFullBinnsPerRow; band++){
          for(int pixelInCubeRow=0; pixelInCubeRow<sensorRows; pixelInCubeRow++){
            cubeColumn[band*sensorRows+pixelInCubeRow] = binnedFrames[frameNumber][nFullBinnsPerRow*(sensorRows-1)-nFullBinnsPerRow*pixelInCubeRow+band];
          }
        }
        fwrite (cubeColumn, sizeof(uint8_t), sensorRows*nFullBinnsPerRow, pFileCube);
      }
    }
  }
  else if(cubeType==Bip){//BIP
    for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
      strcpy(fileName, "/home/root/rawCameraData/tmp/");
      sprintf(imageNumberS, "%d", cubeRow);
      strcat(fileName, imageNumberS);
      strcat(fileName, "Binned.raw");
      // printf("Cubing file: %s\n", fileName);
      FILE * pFileCubeRow = fopen ( fileName , "rb" );
      if (pFileCubeRow==NULL) {fputs ("File error\n",stderr); exit (1);}
      size_t result = fread (binnedFrame, 2, nFullBinnsPerRow*sensorRows, pFileCubeRow);
      if (result != nFullBinnsPerRow*sensorRows) {fputs ("Reading error",stderr); exit (3);}
      fclose (pFileCubeRow);

      for(int pixel=0; pixel<sensorRows; pixel++){
        for(int band=0; band<nFullBinnsPerRow; band++){
          cubeColumn[pixel*nFullBinnsPerRow+band] = binnedFrame[nFullBinnsPerRow*(sensorRows-1)-nFullBinnsPerRow*pixel+band];
        }
      }
      fwrite (cubeColumn, sizeof(uint8_t), sensorRows*nFullBinnsPerRow, pFileCube);
    }
  }
  else{//BSQ
    for(int band=0; band<nFullBinnsPerRow; band++){
      for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
        strcpy(fileName, "/home/root/rawCameraData/tmp/");
        sprintf(imageNumberS, "%d", cubeRow);
        strcat(fileName, imageNumberS);
        strcat(fileName, "Binned.raw");
        // printf("Cubing file: %s\n", fileName);
        FILE * pFileCubeRow = fopen ( fileName , "rb" );
        if (pFileCubeRow==NULL) {fputs ("File error\n",stderr); exit (1);}
        size_t result = fread (binnedFrame, 2, nFullBinnsPerRow*sensorRows, pFileCubeRow);
        if (result != nFullBinnsPerRow*sensorRows) {fputs ("Reading error",stderr); exit (3);}

        for(int pixelInCubeRow=0; pixelInCubeRow<cubeColumns; pixelInCubeRow++){
          cubeColumn[pixelInCubeRow] = binnedFrame[nBandsBinned*(sensorRows-1)-nBandsBinned*pixelInCubeRow+band];
        }
        fwrite (cubeColumn, sizeof(uint8_t), sensorRows*nFullBinnsPerRow, pFileCube);
        fclose (pFileCube);
      }
    }
  }
  gettimeofday(&tv2, NULL);
  printf("Stored all %f\n", (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec));
  // writeCubeToFile();
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

void HSICamera::writeRawDataToFile(uint8_t* image, int imageNumber){

  struct timespec timeSystem;
  clock_gettime(CLOCK_REALTIME, &timeSystem);
  char timeSystemString[32];
  sprintf(timeSystemString, "%lli", (long long)timeSystem.tv_nsec);
  char filePath[64];

  int num = 321;
  char imageNumberS[11];
  // itoa(imageNumber, imageNumberS, 10);
  sprintf(imageNumberS, "%d", imageNumber);

  strcpy(filePath, "/home/andreas/MasterThesis/capture/");
  // strcat(filePath, timeSystemString);
  strcat(filePath, imageNumberS);
  strcat(filePath, "Single.raw");

  FILE * fp;
  fp = fopen (filePath,"wb");
  fwrite (image, sizeof(uint8_t), sensorColumns*sensorRows, fp);//TODO bitDepth
  fclose (fp);
}

void HSICamera::writeCubeToFile(){
  struct timespec timeSystem;
  clock_gettime(CLOCK_REALTIME, &timeSystem);
  char timeSystemString[32];
  sprintf(timeSystemString, "%lli", (long long)timeSystem.tv_nsec);
  char filePath[64];
  // strcpy(filePath, "/home/root/capture/");
  strcpy(filePath, "/home/andreas/MasterThesis/capture/");
  strcat(filePath, timeSystemString);
  strcat(filePath, "Cube.raw");

  FILE * fp;
  fp = fopen (filePath,"wb");

  if (fp==NULL){
    printf("\nFile cant be opened");
    return;
  }
  for(int i=0; i<cubeRows; i++){
      fwrite (hsiCube[i], sizeof(uint8_t), cubeColumns, fp);//TODO bitDepth
    }
    printf("Success\n");
    fclose (fp);
    printf("Successs\n");
}



void HSICamera::captureSingleImage(){
  // is_FreezeVideo(hCam, IS_WAIT);
  // writeSingleToFile();
}

void HSICamera::writeSingleToFile(){

  struct timespec timeSystem;
  clock_gettime(CLOCK_REALTIME, &timeSystem);
  char timeSystemString[32];
  sprintf(timeSystemString, "%lli", (long long)timeSystem.tv_nsec);
  char filePath[64];
  strcpy(filePath, "/home/");
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

void HSICamera::swap(uint8_t *xp, uint8_t *yp)
{
    uint8_t temp = *xp;
    *xp = *yp;
    *yp = temp;
}

void HSICamera::insertionSort(uint8_t arr[], int startPosition, int n)
{
  uint8_t key;
   int i, j;
   for (i = startPosition+1; i < startPosition+n; i++)
   {
      // printf("v - %u\n", *(arr+i));
      // *(arr+i) = 2;
      // printf("w - %u\n", *(arr+i));
       // key = arr[i];
       j = i-1;

       /* Move elements of arr[0..i-1], that are
          greater than key, to one position ahead
          of their current position */
       while (j >= 0 && arr[j] > key)
       {
           arr[j+1] = arr[j];
           j = j-1;
       }
       arr[j+1] = key;

   }
   // printf("arr end %p\n", arr);
}


void HSICamera::bubbleSort(uint8_t arr[], int n)
{
   int i, j;
   for (i = 0; i < n-1; i++)

       // Last i elements are already in place
       for (j = 0; j < n-i-1; j++)
           if (arr[j] > arr[j+1])
              swap(&arr[j], &arr[j+1]);
}


void HSICamera::bitonicMerge12(uint8_t* arr){

  // // Load vectors
  // uint16x8_t vec1_16x8, vec2_16x8;
  // uint16x4_t vec1_16x4, vec2_16x4, vec3_16x4, vec4_16x4;
  // uint8_t zeroPadding[4] = {0, 0, 0, 0};
  //
  // vec1_16x4 = vld1_u16(zeroPadding);
  // vec2_16x4 = vld1_u16(arr);
  // vec3_16x4 = vld1_u16(arr+4);
  // vec4_16x4 = vld1_u16(arr+8);
  //
  // // Sorting across registers
  // uint16x4_t max_vec_sort = vmax_u16(vec4_16x4, vec3_16x4);
  // uint16x4_t min_vec_sort = vmin_u16(vec4_16x4, vec3_16x4);
  //
  // vec3_16x4 = vmin_u16(max_vec_sort, vec2_16x4);
  // vec4_16x4 = vmax_u16(max_vec_sort, vec2_16x4);
  //
  // vec2_16x4 = vmin_u16(vec3_16x4, min_vec_sort);
  // vec3_16x4 = vmax_u16(vec3_16x4, min_vec_sort);
  //
  // // Transpose vectors
  // vec1_16x8 = vcombine_u16(vec1_16x4, vec2_16x4);
  // vec2_16x8 = vcombine_u16(vec3_16x4, vec4_16x4);
  //
  // uint16x8x2_t interleavedVector = vzipq_u16(vec1_16x8, vec2_16x8);
  // interleavedVector = vzipq_u16(interleavedVector.val[0], interleavedVector.val[1]);
  //
  // uint16x8_t reversed_vector = vrev64q_u16(interleavedVector.val[1]);
  //
  // // L1
  // uint16x8_t max_vec = vmaxq_u16(reversed_vector, interleavedVector.val[0]);
  // uint16x8_t min_vec = vminq_u16(reversed_vector, interleavedVector.val[0]);
  //
  // uint16x8x2_t shuffleTmp = vtrnq_u16(min_vec, max_vec);
  // uint16x8x2_t interleavedTmp = vzipq_u16(shuffleTmp.val[0], shuffleTmp.val[1]);
  //
  // // Shuffle lines into right vectors
  // uint16x4_t line1 = vget_high_u16(interleavedTmp.val[0]);
  // uint16x4_t line3 = vget_low_u16(interleavedTmp.val[0]);
  // uint16x4_t line2 = vget_high_u16(interleavedTmp.val[1]);
  // uint16x4_t line4 = vget_low_u16(interleavedTmp.val[1]);
  // uint16x8_t  vec1_L2 = vcombine_u16(line1, line2);
  // uint16x8_t  vec2_L2 = vcombine_u16(line3, line4);
  //
  // // L2
  // max_vec = vmaxq_u16(vec1_L2, vec2_L2);
  // min_vec = vminq_u16(vec1_L2, vec2_L2);
  //
  // uint16x8x2_t vec1_L3 = vtrnq_u16(min_vec, max_vec);
  //
  // // L3
  // max_vec = vmaxq_u16(vec1_L3.val[0], vec1_L3.val[1]);
  // min_vec = vminq_u16(vec1_L3.val[0], vec1_L3.val[1]);
  //
  // uint16x8x2_t zippedResult = vzipq_u16(min_vec, max_vec);
  //
  // // Bitonic merge 8x2
  // reversed_vector = vrev64q_u16(zippedResult.val[0]);
  // uint16x4_t reversed_vector_high = vget_high_u16(reversed_vector);
  // uint16x4_t reversed_vector_low = vget_low_u16(reversed_vector);
  // reversed_vector = vcombine_u16(reversed_vector_high, reversed_vector_low);
  //
  // max_vec = vmaxq_u16(reversed_vector, zippedResult.val[1]);
  // min_vec = vminq_u16(reversed_vector, zippedResult.val[1]);
  //
  // // Shuffle lines into right vectors
  // uint16x4_t max_vec_high = vget_high_u16(max_vec);
  // uint16x4_t max_vec_low = vget_low_u16(max_vec);
  // uint16x4_t min_vec_high = vget_high_u16(min_vec);
  // uint16x4_t min_vec_low = vget_low_u16(min_vec);
  // uint16x8_t shuffled_vec1 = vcombine_u16(min_vec_low, max_vec_low);
  // uint16x8_t shuffled_vec2 = vcombine_u16(min_vec_high, max_vec_high);
  //
  // uint16x8_t input_bitonic4x2_max = vmaxq_u16(shuffled_vec1, shuffled_vec2);
  // uint16x8_t input_bitonic4x2_min = vminq_u16(shuffled_vec1, shuffled_vec2);
  //
  //
  // // Bitonic merge 4x2
  // // L1
  // max_vec = vmaxq_u16(input_bitonic4x2_min, input_bitonic4x2_max);
  // min_vec = vminq_u16(input_bitonic4x2_min, input_bitonic4x2_max);
  //
  // shuffleTmp = vtrnq_u16(min_vec, max_vec);
  // interleavedTmp = vzipq_u16(shuffleTmp.val[0], shuffleTmp.val[1]);
  //
  // // Shuffle lines into right vectors
  // line1 = vget_high_u16(interleavedTmp.val[0]);
  // line3 = vget_low_u16(interleavedTmp.val[0]);
  // line2 = vget_high_u16(interleavedTmp.val[1]);
  // line4 = vget_low_u16(interleavedTmp.val[1]);
  // vec1_L2 = vcombine_u16(line1, line2);
  // vec2_L2 = vcombine_u16(line3, line4);
  //
  // // L2
  // max_vec = vmaxq_u16(vec1_L2, vec2_L2);
  // min_vec = vminq_u16(vec1_L2, vec2_L2);
  //
  // vec1_L3 = vtrnq_u16(min_vec, max_vec);
  //
  // // L3
  // max_vec = vmaxq_u16(vec1_L3.val[0], vec1_L3.val[1]);
  // min_vec = vminq_u16(vec1_L3.val[0], vec1_L3.val[1]);
  //
  // zippedResult = vzipq_u16(min_vec, max_vec);
  // uint16x4_t lowestValues = vget_high_u16(zippedResult.val[0]);
  // vst1_u16(arr, lowestValues);
  // vst1q_u16(arr+4, zippedResult.val[1]);
}
