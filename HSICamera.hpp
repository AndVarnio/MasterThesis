#include<ueye.h>
#include<stdio.h>
#include <fstream>
#include <sstream>
#include<iomanip>
#include <opencv2/highgui.hpp>

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
      int nSingleFrames = 100;
      double frameRate = 32.0;
      int bands;
      // static const int cubeRows = 10;
      char **hsiCube;
      int captureInterval = 1000*10;
      void writeCubeToFile();
      void writeSingleToFile();
      void writeBandsToSeparateFiles();
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
  sensorRows = rows;
  sensorColumns = columns;
  bands = columns;
  bitDepth = pixelDepth;
  UINT nPixelClock = pixelClockMHz;

  if(1){//TODO cubeformat enum, now bil
    cubeColumns = sensorRows*bands;
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

  is_AllocImageMem(hCam, sensorColumns, sensorRows, bitDepth, &memSingleImage, &memIDSingle);
  is_SetImageMem(hCam, memSingleImage, memIDSingle);

  is_SetExternalTrigger(hCam, IS_SET_TRIGGER_SOFTWARE);

  // errorCode = is_CaptureVideo (hCam, IS_WAIT);
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with putting camera in freerun mode, error code: %d\n", errorCode);
  // };
  //
  // double fps = frameRate;
  // errorCode = is_SetFrameRate(hCam, fps, &fps);
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with setting the framerate, error code: %d\n", errorCode);
  // };

}

void HSICamera::runCubeCapture(){
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
      printf("Images captured\n");
      writeCubeToFile();
      printf("New cube written to file\n");
      writeBandsToSeparateFiles();
      printf("Grayscale images written to folder\n");
    // }
  }

}

void HSICamera::writeCubeToFile(){
  std::ofstream ofs;
  auto time_now = std::time(0);
  std::stringstream ss;
  ss << std::put_time(std::gmtime(&time_now), "%c %Z");
  std::string timeString = ss.str();
  timeString = "./capture/" + timeString + ".raw";
  printf("Trying to open: %s\n", timeString.c_str());
  ofs.open( timeString, std::ofstream::binary|std::ios_base::app );
  if (!ofs.is_open())
  {
    printf("ofs not open\n");
  }
  // ofs.write( pMem, sensorColumns*sensorRows );//TODO bitDepth
  const char linebreak = '\n';
  for(int i=0; i<cubeRows; i++){
    ofs.write( hsiCube[i], cubeColumns*cubeRows );
    //ofs.write( &linebreak, 1 );
    // ofs << hsiCube[i];
    // ofs << "\n";
  }
  ofs.close();
}

void HSICamera::captureSingleImage(){
  is_FreezeVideo(hCam, IS_WAIT);
  writeSingleToFile();
}

void HSICamera::writeSingleToFile(){
  std::ofstream ofs;
  auto time_now = std::time(0);
  std::stringstream ss;
  ss << std::put_time(std::gmtime(&time_now), "%c %Z");
  std::string timeString = ss.str();
  timeString = "./capture/" + timeString + ".raw";

  ofs.open( timeString, std::ofstream::binary );
  ofs.write( memSingleImage, sensorColumns*sensorRows );//TODO bitDepth
  ofs.close();
}

void HSICamera::writeBandsToSeparateFiles(){

  auto time_now = std::time(0);
  std::stringstream ss;
  ss << time_now;
  std::string timeString = ss.str();
  std::string newDirectory = "mkdir -p ./capture/"+timeString;
  system(newDirectory.c_str());

  char grayScaleImage[nSingleFrames*sensorRows];
  if(1){ //This is bsq
    for(int band=0; band<bands; band++){
      for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
        for(int sensorRow=0; sensorRow<sensorRows; sensorRow++){
          grayScaleImage[cubeRow*sensorRows+sensorRow] = hsiCube[cubeRow][band*sensorRows+sensorRow];
        }
      }
      std::string filename = "./capture/" + timeString + "/" + std::to_string(band) + ".png";
      cv::Mat grayScaleMat = cv::Mat(nSingleFrames, sensorRows, CV_8UC1, &grayScaleImage);
      imwrite(filename,grayScaleMat);
    }
  }
}
