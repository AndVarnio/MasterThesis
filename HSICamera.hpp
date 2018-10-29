#include<ueye.h>
#include<stdio.h>
#include<string.h>
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
      const int nSingleFrames = 640;
      double frameRate = 16.0;
      int bands;
      int nBandsBinned;
      char** memSingleImageSequence = new char*[nSingleFrames];
      int binningFactor = 1;
      // static const int cubeRows = 10;
      char **hsiCube;
      int captureInterval = 1000*10;
      int singleImageMemSize;
      void writeCubeToFile();
      void writeSingleToFile();
      void writeBandsToSeparateFiles();
      void writeRawDataToFile(char** rawImages, int nRows, int nColumns);
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
    hsiCube = new char*[cubeRows];
    for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
      hsiCube[cubeRow] = new char[cubeColumns];//TODO pixeldepth
    }
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

  is_AllocImageMem(hCam, sensorColumns, sensorRows, bitDepth, &memSingleImage, &memIDSingle);
  is_SetImageMem(hCam, memSingleImage, memIDSingle);

  is_SetExternalTrigger(hCam, IS_SET_TRIGGER_SOFTWARE);

/*
  /////////////Set imagememory for freerun mode//////////////////////
  for(int imageMemory=1; imageMemory<=nSingleFrames; imageMemory++){
    // printf("Adding imagememory %i\n", imageMemory);
    is_AllocImageMem(hCam, sensorColumns, sensorRows, bitDepth, &memSingleImageSequence[imageMemory], &imageMemory);
    is_AddToSequence (hCam, memSingleImageSequence[imageMemory], imageMemory);
  }
  is_InitImageQueue (hCam, 0);
*/
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
  ppcMem[0] = NULL;
  int imageSequenceID = 1; // = new int[nSingleFrames];
  if(1){//if this is bsq
    for(int imageNumber=0; imageNumber<nSingleFrames; imageNumber++){
      is_WaitForNextImage(hCam, 1000, &(ppcMem[imageNumber]), &imageSequenceID);
      // printf("Tick %i\n", imageNumber);
    }
    printf("Images captured\n");
    writeRawDataToFile(ppcMem, nSingleFrames, sensorRows*sensorColumns);
    printf("Raw data written to file\n");
    *(unsigned char*)(&ppcMem); //Reinterpret cast

    /////Binning
    if(binningFactor>1){
      int factorLastBands = bands%binningFactor;
      int nBinningsPerRow = bands/binningFactor;
      int nColumnsBinned = (bands + binningFactor - 1) / binningFactor;

      for(int imageNumber=0; imageNumber<nSingleFrames; imageNumber++){
        for(int row=0; row<sensorRows; row++){
          for(int binnIterator=0; binnIterator<nBinningsPerRow; binnIterator++){
            int totPixVal = 0;
            for(int pixelIterator=0; pixelIterator<binningFactor; pixelIterator++){
              // TODO Char arithmatic
              totPixVal += (int)ppcMem[imageNumber][row*sensorColumns+binnIterator*binningFactor+pixelIterator];
            }
            ppcMem[imageNumber][row*nColumnsBinned+binnIterator] = (unsigned char)(totPixVal/binningFactor);
          }
          if(factorLastBands>0){
            int totPixVal = 0;
            for(int pixelIterator=0; pixelIterator<factorLastBands; pixelIterator++){
              // TODO Char arithmatic
              totPixVal += (int)ppcMem[imageNumber][nBinningsPerRow*binningFactor+pixelIterator];
            }
            ppcMem[imageNumber][row*nColumnsBinned+nBinningsPerRow] = (unsigned char)totPixVal/factorLastBands;
          }
        }
      }
    }

///Make cube
      for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
        //TODO Binning
        // Put rows together
        // store them in new array



        for(int band=0; band<nBandsBinned; band++){
          for(int pixelInCubeRow=0; pixelInCubeRow<sensorRows; pixelInCubeRow++){
            hsiCube[cubeRow][band*sensorRows+pixelInCubeRow] = ppcMem[cubeRow][nBandsBinned*(sensorRows-1)-nBandsBinned*pixelInCubeRow+band];
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


  writeCubeToFile();
  printf("New cube written to file\n");
  writeBandsToSeparateFiles();
  printf("Grayscale images written to folder\n");
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
  strcpy(filePath, "/home/andreas/HSIProject/capture/");
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
  strcpy(filePath, "/home/andreas/HSIProject/capture/");
  strcat(filePath, timeSystemString);
  strcat(filePath, "Cube.raw");

  FILE * fp;
  fp = fopen (filePath,"wb");

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
  strcpy(filePath, "/home/andreas/HSIProject/capture/");
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
