#include <fstream>
#include <iterator>
#include <vector>
#include <omp.h>
#include <iostream>
// Converts BIL cube to BIP cube
// g++ readBIL.cpp -o readBILCube `pkg-config --cflags --libs opencv`
int columns = 1080;
int bands = 96;
int rows = 1735;

char* buffer;
char** rawImages;
char **hsiCube;

int main(int argc, char* argv[])
{
  if (argc<2) {
    puts("Usage: a.out [PATH TO RAW]");
    return -1;
  }

  buffer = new char[columns*bands*rows];

  std::ifstream input( argv[1], std::ios::in | std::ios::binary );
  if (!input){
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

  int cubeColumns = columns*bands;

  hsiCube = new char*[rows];
  for(int cubeRow=0; cubeRow<rows; cubeRow++){
    hsiCube[cubeRow] = new char[cubeColumns];//TODO pixeldepth
  }


  for(int cubeRow=0; cubeRow<rows; cubeRow++){
    for(int pixel=0; pixel<columns; pixel++){
      for(int band=0; band<bands; band++){
        hsiCube[cubeRow][pixel*bands+band] = rawImages[cubeRow][bands*(columns-1)-bands*pixel+band];
      }
    }
  }


  FILE * fp;
  fp = fopen ("testBIP","wb");

  if (fp==NULL){
    printf("\nFile cant be opened");
    return 0;
  }
  for(int i=0; i<rows; i++){
    fwrite (hsiCube[i], sizeof(char), cubeColumns, fp);//TODO bitDepth
  }
  fclose (fp);


  return 0;
}
