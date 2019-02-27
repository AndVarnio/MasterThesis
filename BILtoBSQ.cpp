#include <fstream>
#include <iterator>
#include <vector>
#include <opencv2/highgui.hpp>
#include <omp.h>
#include <iostream>
// g++ readBIL.cpp -o readBILCube `pkg-config --cflags --libs opencv`
int columns = 1080;
int bands = 160;
int rows = 2254;

int nSingleFrames = rows;
int nBandsBinned = bands;
int sensorRows = columns;
int cubeColumns = columns;

uint16_t* buffer;
uint16_t** waveLengths;
uint16_t** rawImages;
uint16_t **hsiCube;

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
      if (result != SIZEINPUTFILE) {fputs ("Reading error\n",stderr);
      printf("result=%ld SIZEINPUTFILE=%d\n",result, SIZEINPUTFILE);
      exit (3);}


    #pragma omp parallel for num_threads(4)
    for(int band=0; band<bands; band++){
      for(int row=0; row<rows; row++){
        for(int column=0; column<columns; column++){
          waveLengths[band][row*columns+column] = buffer[row*bands*columns+band*columns+column];
        }
      }
    }

    rawImages = new uint16_t*[rows];
    for(int x=0; x<rows; x++){
      rawImages[x] = new uint16_t[bands*columns];
    }

    int rawImageLength = bands*columns;
    for(int imageNr=0; imageNr<rows; imageNr++){
      for(int band=0; band<bands; band++){
        for(int pixel=0; pixel<columns; pixel++){
          rawImages[imageNr][(columns-1)*bands-pixel*bands+band] = buffer[imageNr*rawImageLength+band*columns+pixel];
        }
      }

    }

    int cubeRows = nSingleFrames*nBandsBinned;

    hsiCube = new uint16_t*[cubeRows];
    for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
      hsiCube[cubeRow] = new uint16_t[cubeColumns];//TODO pixeldepth
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
        fwrite (hsiCube[i], sizeof(uint16_t), cubeColumns, fp);//TODO bitDepth
      }
      fclose (fp);


    delete waveLengths;
    return 0;
}
