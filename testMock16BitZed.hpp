#include<ueye.h>
#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include<time.h>
#include <sys/time.h>
// #include <opencv2/highgui.hpp>

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
      int cubeRows = 25;

      int nSingleFrames = 100;
      const int nRawImagesInMemory = 1000;
      double frameRate = 32.0;

      int bands;
      int nBandsBinned;
      int factorLastBands = 0;
      int nFullBinnsPerRow = 0;
      int binningFactor = 12;
      bool meanBinning = true;
      cubeFormat cubeType = Bsq; //a=bil b=bip c=bsq
      char** memSingleImageSequence = new char*[nRawImagesInMemory];
      uint16_t **hsiCube;
      uint16_t** binnedImages;

      void swTriggerCapture();
      void freeRunCapture();
      void writeCubeToFile();
      void writeSingleToFile();
      void writeBandsToSeparateFiles();
      void writeRawDataToFile(char** rawImages, int nRows, int nColumns);
      void writeRawDataToFile(uint16_t* image, int imageNumber);

      int random_partition(unsigned char* arr, int start, int end);
      int random_selection(unsigned char* arr, int start, int end, int k);
      void insertionSort(uint16_t* arr, int startPosition, int n);
      void swap(unsigned char* a, unsigned char* b);
      int partition (unsigned char arr[], int low, int high);
      void quickSort(unsigned char arr[], int low, int high);
      void bubbleSort(uint16_t arr[], int n);
      void swap(uint16_t *xp, uint16_t *yp);
};

HSICamera::HSICamera(){
  // is_SetErrorReport (hCam, IS_ENABLE_ERR_REP);
  // INT success = is_InitCamera(&hCam, NULL);
  // if(success!=IS_SUCCESS){
  //   printf("Failed to initialize camera!\n");
  //   exit (EXIT_FAILURE);
  // }
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

  // ////////////////////Initialize///////////////////
  // int errorCode = is_PixelClock(hCam, IS_PIXELCLOCK_CMD_SET,
  //                       (void*)&nPixelClock,
  //                       sizeof(nPixelClock));
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with the pixel clock, error code: %d\n", errorCode);
  // };
  //
  // double min, max, intervall;
  // is_GetFrameTimeRange (hCam, &min, &max, &intervall);
  // printf("min: %f max: %f intervall: %f", min, max, intervall);
  //
  // errorCode = is_SetDisplayMode(hCam, IS_SET_DM_DIB);
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with the display mode, error code: %d\n", errorCode);
  // };
  //
  // UINT formatID = resolution;
  // errorCode = is_ImageFormat (hCam, IMGFRMT_CMD_SET_FORMAT, &formatID, 4);
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with the resolution setup, error code: %d\n", errorCode);
  // };
  //
  // double expTime = exposureMs;
  // errorCode = is_Exposure(hCam, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*)&expTime, sizeof(expTime));
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with the exposure time, error code: %d\n", errorCode);
  // };
  //
  // errorCode = is_SetColorMode(hCam, IS_CM_MONO12);
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with the color mode, error code: %d\n", errorCode);
  // };
  //
  // errorCode = is_SetGainBoost(hCam, IS_SET_GAINBOOST_ON);
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with the gainboost, error code: %d\n", errorCode);
  // };
  //
  // errorCode = is_SetHardwareGain (hCam, 50, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with the hardware gain, error code: %d\n", errorCode);
  // };
  //
  // switch(triggerMode)
  // {
  //   case Swtrigger  :
  //     is_AllocImageMem(hCam, sensorColumns, sensorRows, bitDepth, &memSingleImage, &memIDSingle);
  //     is_SetImageMem(hCam, memSingleImage, memIDSingle);
  //     printf("Allocated memory for single image\n");
  //     is_SetExternalTrigger(hCam, IS_SET_TRIGGER_SOFTWARE);
  //     break;
  //
  //   case Freerun:
  //   for(int imageMemory=1; imageMemory<=nRawImagesInMemory; imageMemory++){
  //     // printf("Adding imagememory %i\n", imageMemory);
  //     is_AllocImageMem(hCam, sensorColumns, sensorRows, 16, &memSingleImageSequence[imageMemory], &imageMemory);
  //     is_AddToSequence (hCam, memSingleImageSequence[imageMemory], imageMemory);
  //   }
  //   is_InitImageQueue (hCam, 0);
  //
    binnedImages = new uint16_t*[nSingleFrames];
    for(int image=0; image<nSingleFrames; image++){
      binnedImages[image] = new uint16_t[nBandsBinned*sensorRows];//TODO pixeldepth
    }
  //
  //   errorCode = is_SetFrameRate(hCam, frameRate, &frameRate);
  //   if(errorCode!=IS_SUCCESS){
  //     printf("Something went wrong with setting the framerate, error code: %d\n", errorCode);
  //   };
  //
  //   errorCode = is_CaptureVideo (hCam, IS_WAIT);
  //   if(errorCode!=IS_SUCCESS){
  //     printf("Something went wrong with putting camera in freerun mode, error code: %d\n", errorCode);
  //   };
  //   break;
  // }

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
    // for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
    //   is_FreezeVideo(hCam, IS_WAIT);
    //     //TODO Binning
    //
    //   for(int band=0; band<bands; band++){
    //     for(int pixelInCubeRow=0; pixelInCubeRow<sensorRows; pixelInCubeRow++){
    //       hsiCube[cubeRow][band*sensorRows+pixelInCubeRow] = memSingleImage[sensorColumns*(sensorRows-1)-sensorColumns*pixelInCubeRow+band];
    //     }
    //   }
    //     // usleep(20000);
    // }
}
void HSICamera::freeRunCapture(){
  struct timeval  tv1, tv2, tv3;

  char* rawImageP;
  int imageSequenceID = 1;
  int lastPixelInRowOffset = nFullBinnsPerRow*binningFactor;
  for(int imageNumber=0; imageNumber<nSingleFrames; imageNumber++){
    // printf("Wait for a image\n");
    gettimeofday(&tv1, NULL);

// TODO Read next file XSingle.raw
    char fileName[64];

    char imageNumberS[11];
    sprintf(imageNumberS, "%d", imageNumber);

    strcpy(fileName, imageNumberS);
    strcat(fileName, "Single.raw");
    printf("Reading file: %s\n", fileName);
    FILE * pFile = fopen ( fileName , "rb" );
    if (pFile==NULL) {fputs ("File error\n",stderr); exit (1);}

    uint16_t pointerToNew16BitArray[sensorRows*sensorColumns];

    size_t result = fread (pointerToNew16BitArray, 2, 1920*1080, pFile);
    if (result != 1920*1080) {fputs ("Reading error",stderr); exit (3);}
    fclose (pFile);

    #pragma omp parallel for num_threads(2)
    for(int row=0; row<sensorRows; row++){
      int rowOffset = row*sensorColumns;
      int binnedIdxOffset = row*nBandsBinned;

      int binOffset = 0;
      for(int binnIterator=0; binnIterator<nFullBinnsPerRow; binnIterator++){
        int totPixVal = 0;

        totPixVal += (int)pointerToNew16BitArray[rowOffset+binOffset+0];
        totPixVal += (int)pointerToNew16BitArray[rowOffset+binOffset+1];
        totPixVal += (int)pointerToNew16BitArray[rowOffset+binOffset+2];
        totPixVal += (int)pointerToNew16BitArray[rowOffset+binOffset+3];
        totPixVal += (int)pointerToNew16BitArray[rowOffset+binOffset+4];
        totPixVal += (int)pointerToNew16BitArray[rowOffset+binOffset+5];
        totPixVal += (int)pointerToNew16BitArray[rowOffset+binOffset+6];
        totPixVal += (int)pointerToNew16BitArray[rowOffset+binOffset+7];
        totPixVal += (int)pointerToNew16BitArray[rowOffset+binOffset+8];
        totPixVal += (int)pointerToNew16BitArray[rowOffset+binOffset+9];
        totPixVal += (int)pointerToNew16BitArray[rowOffset+binOffset+10];
        totPixVal += (int)pointerToNew16BitArray[rowOffset+binOffset+11];

        binOffset += binningFactor;
        //   // TODO Char arithmatic

        binnedImages[imageNumber][binnedIdxOffset+binnIterator] = (uint16_t)(totPixVal/binningFactor);
        // printf("row=%i binnIterator=%i\n",row, binnIterator);
        // printf("GOTOTOT\n" );

      }

      if(factorLastBands>0){
        char totPixVal = 0;
        for(int pixelIterator=0; pixelIterator<factorLastBands; pixelIterator++){
          totPixVal = pointerToNew16BitArray[nFullBinnsPerRow*binningFactor+pixelIterator];
        }
        binnedImages[imageNumber][binnedIdxOffset+nFullBinnsPerRow] = pointerToNew16BitArray[nFullBinnsPerRow*binningFactor];
      }

    }
    // usleep(9000);
    gettimeofday(&tv2, NULL);
    // printf("%d %f\n", imageNumber, (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec));
    // is_UnlockSeqBuf (hCam, 1, rawImageP);
    // exit(1);

  }


  for(int imageMemory=1; imageMemory<=nRawImagesInMemory; imageMemory++){
    // is_FreeImageMem (hCam, memSingleImageSequence[imageMemory], imageMemory);
  }

  hsiCube = new uint16_t*[cubeRows];
  for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
    hsiCube[cubeRow] = new uint16_t[cubeColumns];//TODO pixeldepth
  }

  ///Make cube
  if(cubeType==Bil){//BIL
    for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
      for(int band=0; band<nFullBinnsPerRow; band++){
        for(int pixelInCubeRow=0; pixelInCubeRow<sensorRows; pixelInCubeRow++){
          hsiCube[cubeRow][band*sensorRows+pixelInCubeRow] = binnedImages[cubeRow][nFullBinnsPerRow*(sensorRows-1)-nFullBinnsPerRow*pixelInCubeRow+band];
        }
      }
    }
  }
  else if(cubeType==Bip){//BIP
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

void HSICamera::writeRawDataToFile(uint16_t* image, int imageNumber){

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
  fwrite (image, sizeof(uint16_t), sensorColumns*sensorRows, fp);//TODO bitDepth
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
      fwrite (hsiCube[i], sizeof(uint16_t), cubeColumns, fp);//TODO bitDepth
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

void HSICamera::swap(uint16_t *xp, uint16_t *yp)
{
    uint16_t temp = *xp;
    *xp = *yp;
    *yp = temp;
}

void HSICamera::insertionSort(uint16_t arr[], int startPosition, int n)
{
  uint16_t key;
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


void HSICamera::bubbleSort(uint16_t arr[], int n)
{
   int i, j;
   for (i = 0; i < n-1; i++)

       // Last i elements are already in place
       for (j = 0; j < n-i-1; j++)
           if (arr[j] > arr[j+1])
              swap(&arr[j], &arr[j+1]);
}
