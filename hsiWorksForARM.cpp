#include<ueye.h>
#include<stdio.h>
#include<string.h>
#include <stdlib.h>

// #include <fstream>
#include<time.h>
// #include <sstream>
// #include<iomanip>
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
      int cubeRows = 25; //1735;
      const int nSingleFrames = 1735;
      const int nRawImagesInMemory = 100;
      double frameRate = 16.0;
      int bands;
      int nBandsBinned;
      char** memSingleImageSequence = new char*[nRawImagesInMemory];
      int binningFactor = 20;
      // static const int cubeRows = 10;
      char **hsiCube;
      unsigned char** binnedImages;
      int captureInterval = 1000*10;
      int singleImageMemSize;
      bool meanBinning = false;
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
  if(binningFactor>1){
    if(sensorColumns%binningFactor==0){
      nBandsBinned = sensorColumns/binningFactor;
    }
    else{
      nBandsBinned = sensorColumns/binningFactor + 1;
    }
  }


  if(1){//TODO cubeformat enum, now bil
    singleImageMemSize = sensorRows*sensorColumns;
    cubeColumns = sensorRows*nBandsBinned;
    cubeRows = nSingleFrames;

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

  binnedImages = new unsigned char*[nSingleFrames+1];
  for(int image=0; image<nSingleFrames+1; image++){
    binnedImages[image] = new unsigned char[nBandsBinned*sensorRows];//TODO pixeldepth
  }

  //////////////Temperature/////////////////////
  /*
  double fTemperature = 0;
  is_DeviceFeature(hCam, IS_DEVICE_FEATURE_CMD_GET_TEMPERATURE,
                      (void*)&fTemperature, sizeof(fTemperature));
  printf("Internal camera temperature: %f", fTemperature);

  is_DeviceFeature(hCam, IS_DEVICE_FEATURE_CMD_GET_SENSOR_TEMPERATURE_NUMERICAL_VALUE,
                      (void*)&fTemperature, sizeof(fTemperature));
  printf("Internal temperature: %f", fTemperature);

  INT nTemperatureStatus = 0;
  is_DeviceFeature(hCam, IS_DEVICE_FEATURE_CMD_GET_TEMPERATURE_STATUS, &nTemperatureStatus, sizeof(nTemperatureStatus));
  printf("Temperature status: %i", nTemperatureStatus);
*/
  // is_DeviceFeature (hCam, IS_DEVICE_FEATURE_CMD_GET_SENSOR_TEMPERATURE_NUMERICAL_VALUE, void* pParam, UINT cbSizeOfParam)
}

void HSICamera::runCubeCapture(){
  printf("Starting image capture\n");
/////////////////Freerun mode //////////////

  double fps = frameRate;
  int errorCode;
  errorCode = is_SetFrameRate(hCam, fps, &fps);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with setting the framerate, error code: %d\n", errorCode);
  };


  errorCode = is_CaptureVideo (hCam, IS_WAIT);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with putting camera in freerun mode, error code: %d\n", errorCode);
  };

  char** ppcMem = new char*[nSingleFrames];
  char* rawImageP;
  ppcMem[0] = NULL;
  int imageSequenceID = 1; // = new int[nSingleFrames];
  if(1){//if this is bsq
    printf("RUN\n");
    for(int imageNumber=0; imageNumber<nSingleFrames; imageNumber++){
      is_WaitForNextImage(hCam, 1000, &(rawImageP), &imageSequenceID);
      // printf("Image nr: %i\n", imageNumber);
      /////Binning
      if(binningFactor>1){
        if(meanBinning){
        int factorLastBands = bands%binningFactor;
        int nBinningsPerRow = bands/binningFactor;
        int nColumnsBinned = (bands + binningFactor - 1) / binningFactor;


          for(int row=0; row<sensorRows; row++){
            int rowOffset = row*sensorColumns;
            int binnedIdxOffset = row*nColumnsBinned;
            #pragma omp parallel for num_threads(2)
            for(int binnIterator=0; binnIterator<nBinningsPerRow; binnIterator++){
              int binOffset = binnIterator*binningFactor;
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
              totPixVal += (int)rawImageP[rowOffset+binOffset+20];

              // for(int pixelIterator=0; pixelIterator<binningFactor; pixelIterator++){
              //   // TODO Char arithmatic
              //
              //
              //   totPixVal += rawImageP[rowOffset+binOffset+pixelIterator];
              //   // totPixVal += (int)rawImageP[1];
              // }
              // binnedImages[imageNumber][binnedIdxOffset+binnIterator] = rawImageP[rowOffset+binOffset];
              binnedImages[imageNumber][binnedIdxOffset+binnIterator] = (unsigned char)(totPixVal/binningFactor);
              // printf("row=%i binnIterator=%i\n",row, binnIterator);
            }
            if(factorLastBands>0){
              char totPixVal = 0;
              for(int pixelIterator=0; pixelIterator<factorLastBands; pixelIterator++){
                // TODO Char arithmatic
                totPixVal = rawImageP[nBinningsPerRow*binningFactor+pixelIterator];
              }
              binnedImages[imageNumber][binnedIdxOffset+nBinningsPerRow] = rawImageP[nBinningsPerRow*binningFactor];
              // binnedImages[imageNumber][binnedIdxOffset+nBinningsPerRow] = (unsigned char)totPixVal/factorLastBands;
            }
          }
        }

        else{
          int factorLastBands = bands%binningFactor;
          int nBinningsPerRow = bands/binningFactor;
          int nColumnsBinned = (bands + binningFactor - 1) / binningFactor;


            for(int row=0; row<sensorRows; row++){
              int rowOffset = row*sensorColumns;
              int binnedIdxOffset = row*nColumnsBinned;
              // printf("row=%i\n",row);
              // #pragma omp parallel for num_threads(2)
              for(int binnIterator=0; binnIterator<nBinningsPerRow; binnIterator++){
                int binOffset = binnIterator*binningFactor;
                printf("row=%i binOffset=%i\n",row, binOffset);
                // insertionSort((unsigned char*)(&rawImageP[rowOffset+binOffset]), binningFactor);
                // binnedImages[imageNumber][binnedIdxOffset+binnIterator] = rawImageP[rowOffset+binOffset+10];
                binnedImages[imageNumber][binnedIdxOffset+binnIterator] = rawImageP[rowOffset+binOffset+10];
                // random_selection((unsigned char*)(&rawImageP), rowOffset+binOffset, rowOffset+binOffset+20, 10);

                // quickSort((unsigned char*)(&rawImageP), binnedIdxOffset+binnIterator, binnedIdxOffset+binnIterator+20);
                //
                // binnedImages[imageNumber][binnedIdxOffset+binnIterator] = rawImageP[rowOffset+binOffset+10];

              }

            }
        }
      }
      is_UnlockSeqBuf (hCam, 1, rawImageP);
      // printf("Tick %i\n", imageNumber);
    }
    for(int imageMemory=1; imageMemory<=nRawImagesInMemory; imageMemory++){
      is_FreeImageMem (hCam, memSingleImageSequence[imageMemory], imageMemory);

    }

    // Get pointer to new image
    // Bin image
    //  Loop
    printf("Images captured\n");
    // writeRawDataToFile(ppcMem, nSingleFrames, sensorRows*sensorColumns);
    // printf("Raw data written to file\n");
    // *(unsigned char*)(&ppcMem); //Reinterpret cast


///Make cube
hsiCube = new char*[cubeRows];
for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
  hsiCube[cubeRow] = new char[cubeColumns];//TODO pixeldepth
}
printf("Allocated mem for cube\n");
      for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
        // printf("Cube row: %d\n",cubeRow);
        //TODO Binning
        // Put rows together
        // store them in new array



        for(int band=0; band<nBandsBinned; band++){
          for(int pixelInCubeRow=0; pixelInCubeRow<sensorRows; pixelInCubeRow++){
            hsiCube[cubeRow][band*sensorRows+pixelInCubeRow] = binnedImages[cubeRow][nBandsBinned*(sensorRows-1)-nBandsBinned*pixelInCubeRow+band];
          }
        }

/*
        for(int band=0; band<bands; band++){
          for(int pixelInCubeRow=0; pixelInCubeRow<sensorRows; pixelInCubeRow++){
            hsiCube[cubeRow][band*sensorRows+pixelInCubeRow] = ppcMem[cubeRow][sensorColumns*(sensorRows-1)-sensorColumns*pixelInCubeRow+band];
          }
        }
*/
      }




  }


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
  // writeBandsToSeparateFiles();
  // printf("Grayscale images written to folder\n");
}

void HSICamera::writeRawDataToFile(char** rawImages, int nRows, int nColumns){
  /*
  std::ofstream ofs;
  auto time_now = std::time(0);
  std::stringstream ss;
  ss << time_now;
  std::string timeString = ss.str();
  timeString = "./capture/" + timeString + "SensorData.raw";
  */
  /*
  printf("Trying to open: %s\n", timeString.c_str());
  ofs.open( timeString, std::ofstream::binary|std::ios_base::app );
  if (!ofs.is_open())
  {
    printf("ofs not open\n");
  }
  // ofs.write( pMem, sensorColumns*sensorRows );//TODO bitDepth
  const char linebreak = '\n';
  for(int i=0; i<nRows; i++){
    ofs.write( rawImages[i], nColumns );
    //ofs.write( &linebreak, 1 );
    // ofs << hsiCube[i];
    // ofs << "\n";
  }
  ofs.close();
  */

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
  /*
  std::ofstream ofs;
  auto time_now = std::time(0);
  std::stringstream ss;
  ss << std::put_time(std::gmtime(&time_now), "%c %Z");
  std::string timeString = ss.str();
  timeString = "/home/andreas/HSIProject/capture/" + timeString + ".raw";
*/
  /*
  printf("Trying to open: %s\n", timeString.c_str());
  ofs.open( timeString, std::ofstream::binary|std::ios_base::app );
  if (!ofs.is_open())
  {
    printf("ofs not open\n");
  }
  // ofs.write( pMem, sensorColumns*sensorRows );//TODO bitDepth
  const char linebreak = '\n';
  for(int i=0; i<cubeRows; i++){
    ofs.write( hsiCube[i], cubeColumns );
    //ofs.write( &linebreak, 1 );
    // ofs << hsiCube[i];
    // ofs << "\n";
  }
  ofs.close();
*/
  struct timespec timeSystem;
  clock_gettime(CLOCK_REALTIME, &timeSystem);
  char timeSystemString[32];
  sprintf(timeSystemString, "%lli", (long long)timeSystem.tv_nsec);
  char filePath[64];
  strcpy(filePath, "/home/root/capture/");
  // strcpy(filePath, "/home/andreas/HSIProject/capture/");
  strcat(filePath, timeSystemString);
  strcat(filePath, "Cube.raw");

  printf("Trying to open: %s\n", filePath);

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
  printf("Took a picture\n");
  writeSingleToFile();
}

void HSICamera::writeSingleToFile(){
/*
  auto time_now = std::time(0);
  std::stringstream ss;
  ss << time_now;
  std::string timeString = ss.str();
  timeString = "/home/andreas/HSIProject/capture/" + timeString + ".raw";
  */
  /*
  std::ofstream ofs;
  ofs.open( timeString, std::ofstream::binary );
  ofs.write( memSingleImage, sensorColumns*sensorRows );//TODO bitDepth
  ofs.close();*/

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
