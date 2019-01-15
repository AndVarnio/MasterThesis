#include<ueye.h>
#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include<time.h>
#include <sys/time.h>
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
      const int nRawImagesInMemory = 1735;
      double frameRate = 32.0;

      int bands;
      int nBandsBinned;
      int factorLastBands = 0;
      int nFullBinnsPerRow = 0;
      int binningFactor = 20;
      bool meanBinning = false;

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
// TODO Destructor

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

  if(1){//TODO cubeformat enum, now bil and bip

    cubeColumns = sensorRows*nBandsBinned;
    cubeRows = nSingleFrames;

  }
  else{//BSQ
    cubeColumns = sensorRows;
    cubeRows = nSingleFrames*nBandsBinned;
  }

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

  // errorCode = is_SetGainBoost(hCam, IS_SET_GAINBOOST_ON);
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with the gainboost, error code: %d\n", errorCode);
  // };

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
  printf("Starting image capture\n");
  /////////////////Freerun mode //////////////

  char* rawImageP;
  clock_t time_spent = 0;
  int imnr = 1;
  double timtot = 0.0;
  struct timeval  tv1, tv2, tv3;

  int imageSequenceID = 1; // = new int[nSingleFrames];
  for(int imageNumber=0; imageNumber<nSingleFrames; imageNumber++){
    is_WaitForNextImage(hCam, 1000, &(rawImageP), &imageSequenceID);
    // printf("Image nr: %i\n", imageNumber);
    /////Binning
    gettimeofday(&tv1, NULL);
    if(meanBinning){

      #pragma omp parallel for num_threads(2)
      for(int row=0; row<sensorRows; row++){
  int rowOffset = row*sensorColumns;
  int binnedIdxOffset = row*nBandsBinned;

  int binOffset = 0;
  for(int binnIterator=0; binnIterator<nFullBinnsPerRow; binnIterator++){

    int totPixVal = 0;

    totPixVal += (int)rawImageP[rowOffset+binOffset+0];
    totPixVal += (int)rawImageP[rowOffset+binOffset+1];
    totPixVal += (int)rawImageP[rowOffset+binOffset+2];
    totPixVal += (int)rawImageP[rowOffset+binOffset+3];
    totPixVal += (int)rawImageP[rowOffset+binOffset+4];
    totPixVal += (int)rawImageP[rowOffset+binOffset+5];
    totPixVal += (int)rawImageP[rowOffset+binOffset+6];
    totPixVal += (int)rawImageP[rowOffset+binOffset+7];
    totPixVal += (int)rawImageP[rowOffset+binOffset+8];
    totPixVal += (int)rawImageP[rowOffset+binOffset+9];
    totPixVal += (int)rawImageP[rowOffset+binOffset+10];
    totPixVal += (int)rawImageP[rowOffset+binOffset+11];
    totPixVal += (int)rawImageP[rowOffset+binOffset+12];
    totPixVal += (int)rawImageP[rowOffset+binOffset+13];
    totPixVal += (int)rawImageP[rowOffset+binOffset+14];
    totPixVal += (int)rawImageP[rowOffset+binOffset+15];
    totPixVal += (int)rawImageP[rowOffset+binOffset+16];
    totPixVal += (int)rawImageP[rowOffset+binOffset+17];
    totPixVal += (int)rawImageP[rowOffset+binOffset+18];
    totPixVal += (int)rawImageP[rowOffset+binOffset+19];

    binOffset += binningFactor;
    //   // TODO Char arithmatic

    binnedImages[imageNumber][binnedIdxOffset+binnIterator] = (unsigned char)(totPixVal/binningFactor);
    // printf("row=%i binnIterator=%i\n",row, binnIterator);
  }
  if(factorLastBands>0){
    char totPixVal = 0;
    for(int pixelIterator=0; pixelIterator<factorLastBands; pixelIterator++){
      totPixVal = rawImageP[nFullBinnsPerRow*binningFactor+pixelIterator];
    }
    binnedImages[imageNumber][binnedIdxOffset+nFullBinnsPerRow] = rawImageP[nFullBinnsPerRow*binningFactor];
  }
}
    }

    else{
      // printf("imagenumber=%i\n",imageNumber);
      for(int row=0; row<sensorRows; row++){
        int rowOffset = row*sensorColumns;
        int binnedIdxOffset = row*nBandsBinned;
        // printf("row=%i\n",row);
        // #pragma omp parallel for num_threads(2)
        for(int binnIterator=0; binnIterator<nFullBinnsPerRow; binnIterator++){
          int binOffset = binnIterator*binningFactor;
          // printf("row=%i binOffset=%i\n",row, binOffset);
          // insertionSort((unsigned char*)(&rawImageP[rowOffset+binOffset]), binningFactor);
          // binnedImages[imageNumber][binnedIdxOffset+binnIterator] = rawImageP[rowOffset+binOffset+10];
          binnedImages[imageNumber][binnedIdxOffset+binnIterator] = random_selection((unsigned char*)&rawImageP[rowOffset+binOffset], 0, 20, 10);
          // random_selection((unsigned char*)(&rawImageP), rowOffset+binOffset, rowOffset+binOffset+20, 10);
          // quickSort((unsigned char*)(&rawImageP), binnedIdxOffset+binnIterator, binnedIdxOffset+binnIterator+20);
          // binnedImages[imageNumber][binnedIdxOffset+binnIterator] = rawImageP[rowOffset+binOffset+10];
        }
      }
    }
    gettimeofday(&tv2, NULL);
    timtot +=  (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
    // printf("%f\n", (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec));
    is_UnlockSeqBuf (hCam, 1, rawImageP);
  }

  printf("%f\n", timtot);
  // printf("%li\n", (((double)time_spent/nSingleFrames)/CLOCKS_PER_SEC));

  for(int imageMemory=1; imageMemory<=nRawImagesInMemory; imageMemory++){
    is_FreeImageMem (hCam, memSingleImageSequence[imageMemory], imageMemory);
  }

  printf("Images captured\n");


gettimeofday(&tv1, NULL);


hsiCube = new char*[cubeRows];
for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
  hsiCube[cubeRow] = new char[cubeColumns];//TODO pixeldepth
}

///Make cube
  if(1){//BIL



    printf("Allocated mem for cube\n");
    for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
      for(int band=0; band<nBandsBinned; band++){
        for(int pixelInCubeRow=0; pixelInCubeRow<sensorRows; pixelInCubeRow++){
          hsiCube[cubeRow][band*sensorRows+pixelInCubeRow] = binnedImages[cubeRow][nBandsBinned*(sensorRows-1)-nBandsBinned*pixelInCubeRow+band];
        }
      }
    }
  }
  else if(1){//BIP


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
  /* stuff to do! */
  gettimeofday(&tv2, NULL);
  printf("%f\n", (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec));
/*
  /////////////////////////////////Trigger mode /////////////////////

  if(1){//if this is bsq
    // while(1){
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
    // }
  }
*/

  printf("Writing cube to file\n");
  writeCubeToFile();
  printf("New cube written to file\n");
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
  // strcpy(filePath, "/home/root/capture/");
  strcpy(filePath, "/home/andreas/HSIProject/capture/");
  strcat(filePath, timeSystemString);
  strcat(filePath, "Cube.raw");

  printf("Trying to open: %s\n", filePath);
  struct timeval  tv1, tv2;
  gettimeofday(&tv1, NULL);

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

    gettimeofday(&tv2, NULL);
    printf("%f\n", (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec));
}



void HSICamera::captureSingleImage(){
  is_FreezeVideo(hCam, IS_WAIT);
  printf("Took a single picture\n");
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
  printf("Writing picture to file\n" );
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

////////////////////////////Sorting algorithms/////////////////////////////////
int HSICamera::random_partition(unsigned char* arr, int start, int end)
{
    // srand(time(NULL));
    // int pivotIdx = start + rand() % (end-start+1);

    int pivotIdx = end;
    unsigned char pivot = arr[pivotIdx];

    unsigned char tmp = arr[pivotIdx];
    arr[pivotIdx] = arr[end];
    arr[end] = tmp;

    // swap(arr[pivotIdx], arr[end]); // move pivot element to the end

    pivotIdx = end;
    int i = start -1;

    for(int j=start; j<=end-1; j++)
    {
        if(arr[j] <= pivot)
        {
            i = i+1;

            tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;

            // swap(arr[i], arr[j]);
        }
    }

    tmp = arr[i+1];
    arr[i+1] = arr[pivotIdx];
    arr[pivotIdx] = tmp;

    // swap(arr[i+1], arr[pivotIdx]);

    return i+1;
}

int HSICamera::random_selection(unsigned char* arr, int start, int end, int k)
{
    if(start == end)
        return arr[start];

    if(k ==0) return -1;

    if(start < end)
    {

    // int mid = end;
    int mid = partition(arr, start, end);
    int i = mid - start + 1;
    if(i == k)
        return arr[mid];
    else if(k < i)
        return random_selection(arr, start, mid-1, k);
    else
        return random_selection(arr, mid+1, end, k-i);
    }

}

void HSICamera::insertionSort(unsigned char arr[], int n)
{
    unsigned char key;
   int i, j;
   for (i = 1; i < n; i++)
   {
       key = arr[i];
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
}

void HSICamera::swap(unsigned char* a, unsigned char* b)
{
    unsigned char t = *a;
    *a = *b;
    *b = t;
}

int HSICamera::partition (unsigned char arr[], int low, int high)
{
    int pivot = arr[high];    // pivot
    int i = (low - 1);  // Index of smaller element

    for (int j = low; j <= high- 1; j++)
    {
        // If current element is smaller than or
        // equal to pivot
        if (arr[j] <= pivot)
        {
            i++;    // increment index of smaller element
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

/* The main function that implements QuickSort
 arr[] --> Array to be sorted,
  low  --> Starting index,
  high  --> Ending index */
void HSICamera::quickSort(unsigned char arr[], int low, int high)
{
    if (low < high)
    {
        /* pi is partitioning index, arr[p] is now
           at right place */
        int pi = partition(arr, low, high);

        // Separately sort elements before
        // partition and after partition
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}
