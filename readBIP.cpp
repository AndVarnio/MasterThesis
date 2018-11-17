#include <fstream>
#include <iterator>
#include <vector>
#include <opencv2/highgui.hpp>
#include <omp.h>
#include <iostream>
// g++ readBIL.cpp -o readBILCube `pkg-config --cflags --libs opencv`
int columns = 1080;
int bands = 96;
int rows = 1735;

char* buffer;
char** waveLengths;

int main(int argc, char* argv[])
{
  if (argc<2) {
    puts("Usage: a.out [PATH TO RAW]");
    return -1;
  }
    // buffer = malloc(columns*bands*rows);
    buffer = new char[columns*bands*rows];

    waveLengths = new char*[bands];
    for(int band=0; band<bands; band++){
      waveLengths[band] = new char[rows*columns];
    }
    std::ifstream input( argv[1], std::ios::in | std::ios::binary );
    if (!input)
   {
       std::cout << "Failed to open file\n";
       exit(1);
   }

    input.seekg (0, input.end);
    int lengthFile = input.tellg();
    input.seekg (0, input.beg);

    input.read(buffer, columns*rows*bands);

    if (input)
      printf("%i Characters read successfully\n", lengthFile);
    else
      printf("Error, read only %i out of %li characters\n", lengthFile, input.gcount());


    // #pragma omp parallel for num_threads(4)
    for(int band=0; band<bands; band++){
      for(int row=0; row<rows; row++){
        for(int column=0; column<columns; column++){
          waveLengths[band][row*columns+column] = buffer[band+row*bands*column+column*bands];
        }
      }
    }

    for(int band=0; band<bands; band++){
      for(int row=0; row<rows; row++){
        for(int pixel=0; pixel<columns; pixel++){
          waveLengths[band][row*columns+pixel] = buffer[row*bands*columns+pixel*bands+band];
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
        cv::Mat grayScaleMat = cv::Mat(rows, columns, CV_8UC1, waveLengths[band]);
        imwrite(filename,grayScaleMat);
      }

      //Make RGB image

      std::vector<cv::Mat> rgbChannels;


          rgbChannels.push_back(cv::Mat(rows, columns, CV_8UC1, waveLengths[22]));
          rgbChannels.push_back(cv::Mat(rows, columns, CV_8UC1, waveLengths[30]));
          rgbChannels.push_back(cv::Mat(rows, columns, CV_8UC1, waveLengths[45]));

          cv::Mat rgbImage;
          cv::merge(rgbChannels, rgbImage);
          std::string filename = "./" +folderName+".Images" + "/" + "color" + ".png";
          imwrite(filename,rgbImage);



    delete waveLengths;
    return 0;
}
