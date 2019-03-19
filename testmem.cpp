#include<stdio.h>
#include <stdlib.h>
#include <unistd.h>

char** waveLengths;

int main(int argc, char** argv){

  waveLengths = new char*[1000];
  int sum = 400000;
  for(int band=0; band<1000; band++){
    waveLengths[band] = new char[sum];
  }

  for(int band=0; band<1000; band++){
    for(int i=0; i<sum; i++){
      waveLengths[band][i] = i+sum;
    }
  }

  usleep(10000000);
  return 0;
}
