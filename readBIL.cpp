#include <fstream>
#include <iterator>
#include <vector>
#include <opencv2/highgui.hpp>
#include <omp.h>
#include <iostream>
// g++ readBIL.cpp `pkg-config --cflags --libs opencv`
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
    printf("NJINI\n");
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


    #pragma omp parallel for num_threads(4)
    for(int band=0; band<bands; band++){
      for(int row=0; row<rows; row++){
        for(int column=0; column<columns; column++){
          waveLengths[band][row*columns+column] = buffer[row*bands*columns+band*columns+column];
        }
      }
    }
    printf("Images stored in arrays\n");
    cv::Mat grayScaleMat = cv::Mat(rows, columns, CV_8UC1, waveLengths[25]);
    cv::Size s = grayScaleMat.size();
    printf("Height=%i - Width=%i\n",s.height, s.width);
    imwrite("grayImage.png",grayScaleMat);

    delete waveLengths;
    return 0;
}
