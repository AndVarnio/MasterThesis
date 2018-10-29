#include <fstream>
#include <iterator>
#include <vector>
#include <stdio.h>
// #include <opencv2/highgui.hpp>
// #include <omp.h>

int columns = 1080;
int bands = 1920;
int rows = 640;

char* buffer;
char** waveLengths;

int width = 1080;
int imageCount = 1920;
int height = 640;

int main()
{
  struct timespec start;
  double elapsed;

  clock_gettime(CLOCK_REALTIME, &start);
  char name[32];

  // itoa(name, (long long)start.tv_nsec, 10);
  sprintf(name,"%lli", (long long)start.tv_nsec);
  printf("%s\n", name);
printf("%li\n", start.tv_nsec);
    int pixelsInImage = width*height;
    buffer = new char[width*imageCount*height];
    waveLengths = new char*[imageCount];
    for(int band=0; band<imageCount; band++){
      waveLengths[band] = new char[width*height];
    }
    std::ifstream input( "1540369631SensorData.raw", std::ios::binary );

    input.seekg (0, input.end);
    int lengthFile = input.tellg();
    input.seekg (0, input.beg);

    input.read(buffer, width*imageCount*height);

    if (input)
      printf("%i Characters read successfully\n", lengthFile);
    else
      printf("Error, read only %li out of %i characters\n", input.gcount(), lengthFile);

    for(int i=0; i<imageCount; i++){
      for(int j=0; j<pixelsInImage; j++){
        waveLengths[i][j] = buffer[imageCount*pixelsInImage+j];
      }
    }
    std::string numbers = "34243423";
    std::string timeString = "/home/andreas/HSIProject/" + numbers + ".raw";
  FILE * fp;
   /* open the file for writing*/
   fp = fopen (timeString.c_str(),"wb");

   for(int i=0; i<imageCount; i++){
     // fprintf (fp, waveLengths[i]);

     // printf("Written %li bytes\n", fwrite (waveLengths[i], sizeof(char), pixelsInImage, fp));
   }
   /* close the file*/
   fclose (fp);


    delete waveLengths;
    return 0;
}




















/*
#include <string.h>
#include <stdio.h>
#include<iomanip>
#include <iostream>
const char *byte_to_binary(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i=size-1;i>=0;i--)
    {
        for (j=7;j>=0;j--)
        {
            byte = b[i] & (1<<j);
            byte >>= j;
            printf("%u", byte);
        }
    }
    puts("");
}

void test1(){
  char **hsiCube;
  hsiCube = new char*[3];
  hsiCube[0] = new char[4];
  hsiCube[1] = new char[4];
  hsiCube[2] = new char[4];
  hsiCube[0][0] = 'a';
  hsiCube[0][1] = 'b';
  hsiCube[0][2] = 'c';
  hsiCube[0][3] = 'd';
  hsiCube[1][0] = 'e';
  hsiCube[1][1] = 'f';
  hsiCube[1][2] = 'g';
  hsiCube[1][3] = 'h';
  hsiCube[2][0] = 'i';
  hsiCube[2][1] = 'j';
  hsiCube[2][2] = 'k';
  hsiCube[2][3] = 'j';

  printf("Row1: |%s|%s|%s|%s|\n", byte_to_binary(hsiCube[0][0]), byte_to_binary(hsiCube[0][1]), byte_to_binary(hsiCube[0][2]), byte_to_binary(hsiCube[0][3]));
  printf("Row2: |%s|%s|%s|%s|\n", byte_to_binary(hsiCube[1][0]), byte_to_binary(hsiCube[1][1]), byte_to_binary(hsiCube[1][2]), byte_to_binary(hsiCube[1][3]));
  printf("Row3: |%s|%s|%s|%s|\n", byte_to_binary(hsiCube[2][0]), byte_to_binary(hsiCube[2][1]), byte_to_binary(hsiCube[2][2]), byte_to_binary(hsiCube[2][3]));

}

void test2(){
  char **hsiCube;
  hsiCube = new char*[2*3];
  hsiCube[0] = new char[2*4];
  hsiCube[1] = new char[2*4];
  hsiCube[2] = new char[2*4];

  hsiCube[0][0] = 'a';
  hsiCube[0][1] = 'b';
  hsiCube[0][2] = 'c';
  hsiCube[0][3] = 'd';
  hsiCube[1][0] = 'e';
  hsiCube[1][1] = 'f';
  hsiCube[1][2] = 'g';
  hsiCube[1][3] = 'h';
  hsiCube[2][0] = 'i';
  hsiCube[2][1] = 'j';
  hsiCube[2][2] = 'k';
  hsiCube[2][3] = 'j';

  // printBits(sizeof(hsiCube[0][0]), &hsiCube[0][0]);
  printf("Row1: |%s|%s|%s|%s|\n", byte_to_binary(hsiCube[0][0]), byte_to_binary(hsiCube[0][1]), byte_to_binary(hsiCube[0][2]), byte_to_binary(hsiCube[0][3],
  byte_to_binary(hsiCube[0][4]), byte_to_binary(hsiCube[0][5]), byte_to_binary(hsiCube[0][6]), byte_to_binary(hsiCube[0][7]));
  printf("Row2: |%s|%s|%s|%s|\n", byte_to_binary(hsiCube[1][0]), byte_to_binary(hsiCube[1][1]), byte_to_binary(hsiCube[1][2]), byte_to_binary(hsiCube[1][3],
  byte_to_binary(hsiCube[1][4]), byte_to_binary(hsiCube[1][5]), byte_to_binary(hsiCube[1][6]), byte_to_binary(hsiCube[1][7]));
  printf("Row3: |%s|%s|%s|%s|\n", byte_to_binary(hsiCube[2][0]), byte_to_binary(hsiCube[2][1]), byte_to_binary(hsiCube[2][2]), byte_to_binary(hsiCube[2][3],
  byte_to_binary(hsiCube[2][0]), byte_to_binary(hsiCube[2][1]), byte_to_binary(hsiCube[2][2]), byte_to_binary(hsiCube[2][3]));

}

int main(){
  test1();
  test2();
  return 0;
}
*/
