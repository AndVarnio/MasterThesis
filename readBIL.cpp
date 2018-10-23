#include <fstream>
#include <iterator>
#include <vector>
#include <opencv2/highgui.hpp>
#include <omp.h>

int columns = 1080;
int bands = 1920;
int rows = 640;

char* buffer;
char** waveLengths;

int main()
{
    buffer = new char[columns*bands*rows];
    waveLengths = new char*[bands];
    for(int band=0; band<bands; band++){
      waveLengths[band] = new char[rows*columns];
    }
    std::ifstream input( "toDecode.raw", std::ios::binary );

    input.seekg (0, input.end);
    int lengthFile = input.tellg();
    input.seekg (0, input.beg);

    input.read(buffer, columns*bands*rows);

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
    cv::Mat grayScaleMat = cv::Mat(rows, columns, CV_8UC1, waveLengths[500]);
    cv::Size s = grayScaleMat.size();
    printf("Height=%i - Width=%i\n",s.height, s.width);
    imwrite("grayImage.png",grayScaleMat);

    delete waveLengths;
    return 0;
}
