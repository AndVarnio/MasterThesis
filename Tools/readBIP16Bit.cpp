#include <fstream>
#include <iterator>
#include <vector>
#include <opencv2/highgui.hpp>
#include <omp.h>
#include <iostream>
// Reads 16-bit BIP cube and stores the bands as grayscale images
// g++ readBIL.cpp -o readBILCube `pkg-config --cflags --libs opencv`
int columns = 720;
int bands = 107;
int rows = 500;

uint16_t* buffer;
uint16_t** waveLengths;

int main(int argc, char* argv[])
{
  if (argc<2) {
    puts("Usage: a.out [PATH TO RAW]");
    return -1;
  }
  const int SIZEINPUTFILE = columns*bands*rows;
  buffer = new uint16_t[SIZEINPUTFILE];

  waveLengths = new uint16_t*[bands];
  for(int band=0; band<bands; band++){
    waveLengths[band] = new uint16_t[rows*columns];
  }

  FILE * pFile = fopen ( argv[1] , "rb" );
  if (pFile==NULL) {fputs ("File error\n",stderr); exit (1);}

  size_t result = fread (buffer, 2, SIZEINPUTFILE, pFile);
  if (result != SIZEINPUTFILE){
    printf("Read %d, SIZEINPUTFILE=%d\n", result, SIZEINPUTFILE);
    fputs ("Reading error\n",stderr);
    exit (3);
  }


  for(int band=0; band<bands; band++){
    for(int row=0; row<rows; row++){
      for(int column=0; column<columns; column++){
        waveLengths[band][row*columns+column] = buffer[band+row*bands*columns+column*bands];
      }
    }
  }

  std::stringstream ss;
  ss << argv[1];
  std::string folderName = ss.str();
  std::string newDirectory = "mkdir -p ./" +folderName+".Images";
  system(newDirectory.c_str());



  for(int band=0; band<bands; band++){

    std::string filename = "./" +folderName+".Images" + "/" + std::to_string(band) + ".png";
    cv::Mat grayScaleMat = cv::Mat(rows, columns, CV_16UC1, waveLengths[band]);
    imwrite(filename,grayScaleMat);
  }

  //Make RGB image

  std::vector<cv::Mat> rgbChannels;


  rgbChannels.push_back(cv::Mat(rows, columns, CV_16UC1, waveLengths[22]));
  rgbChannels.push_back(cv::Mat(rows, columns, CV_16UC1, waveLengths[30]));
  rgbChannels.push_back(cv::Mat(rows, columns, CV_16UC1, waveLengths[45]));

  cv::Mat rgbImage;
  cv::merge(rgbChannels, rgbImage);
  std::string filename = "./" +folderName+".Images" + "/" + "color" + ".png";
  imwrite(filename,rgbImage);



  delete waveLengths;
  return 0;
}
