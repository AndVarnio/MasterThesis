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
char** rawImages;
char **hsiCube;

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

    rawImages = new char*[rows];
    for(int x=0; x<rows; x++){
      rawImages[x] = new char[bands*columns];
    }

    int rawImageLength = bands*columns;
    for(int imageNr=0; imageNr<rows; imageNr++){
      for(int band=0; band<bands; band++){
        for(int pixel=0; pixel<columns; pixel++){
          rawImages[imageNr][(columns-1)*bands-pixel*bands+band] = buffer[imageNr*rawImageLength+band*columns+pixel];
        }
      }

    }

    int nSingleFrames = 1735;
    int nBandsBinned = bands;
    int sensorRows = 1080;
    int cubeColumns = sensorRows;

    int cubeRows = nSingleFrames*nBandsBinned;

    hsiCube = new char*[cubeRows];
    for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
      hsiCube[cubeRow] = new char[cubeColumns];//TODO pixeldepth
    }


    for(int band=0; band<nBandsBinned; band++){
      for(int rowSpatial=0; rowSpatial<nSingleFrames; rowSpatial++){
        for(int pixelInCubeRow=0; pixelInCubeRow<cubeColumns; pixelInCubeRow++){
          hsiCube[band*nSingleFrames+rowSpatial][pixelInCubeRow] = rawImages[rowSpatial][nBandsBinned*(sensorRows-1)-nBandsBinned*pixelInCubeRow+band];
        }
      }
    }


    FILE * fp;
    fp = fopen ("testBIP","wb");

    if (fp==NULL){
      printf("\nFile cant be opened");
      return 0;
    }
    for(int i=0; i<cubeRows; i++){
        fwrite (hsiCube[i], sizeof(char), cubeColumns, fp);//TODO bitDepth
      }
      fclose (fp);


    delete waveLengths;
    return 0;
}
