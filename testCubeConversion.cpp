#include <opencv2/highgui.hpp>
#include <iostream>

// g++ opencvImages.cpp -o output `pkg-config --cflags --libs opencv`

const int SIZE = 9980928;
static unsigned char bluePixels[SIZE];
static unsigned char greenPixels[SIZE];
static unsigned char redPixels[SIZE];

static unsigned char bluePixelsRecieve[SIZE];
static unsigned char greenPixelsRecieve[SIZE];
static unsigned char redPixelsRecieve[SIZE];

char **hsiCube;
const int imageRows = 2736;
const int imageColumns = 3648;

char **simulatedImages[imageRows];

int main(){
  cv::Mat image;
  image = cv::imread("testColorImage.jpg" , CV_LOAD_IMAGE_COLOR);

  if(! image.data ) {
      std::cout <<  "Could not open or find the image" << std::endl ;
      return -1;
    }

  cv::Mat bgr[3];   //destination array
  cv::split(image,bgr);//split source

  for(int i=0; i<2736; i++){
    for(int j=0; j<3648; j++){
      bluePixels[i*3648+j] = bgr[0].at<uchar>(i, j);
      greenPixels[i*3648+j] = bgr[1].at<uchar>(i, j);
      redPixels[i*3648+j] = bgr[2].at<uchar>(i, j);
    }
  }

  int bands = 3;

  for(int i=0; i<imageRows; i++){
    simulatedImages[i] = new char*[bands*imageColumns];
    for(int j=0; j<imageColumns; j++){
      *(simulatedImages[i][j]) = bluePixels[i*imageColumns+j];
      *(simulatedImages[i][imageColumns+j]) = bluePixels[i*imageColumns+j];
      *(simulatedImages[i][2*imageColumns+j]) = bluePixels[i*imageColumns+j];
    }

  }


  int cubeRows = imageRows;
  int cubeColumns = imageColumns*bands;

  hsiCube = new char*[cubeRows];
  for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
    hsiCube[cubeRow] = new char[cubeColumns];//TODO pixeldepth
  }

  for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
    for(int band=0; band<bands; band++){
      for(int pixelInRow=0; pixelInRow<imageColumns; pixelInRow++){
        hsiCube[cubeRow][band*imageColumns+pixelInRow] = simulatedImages[cubeRow][band*imageColumns+pixelInRow];
      }
    }
  }




    for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
      for(int imagePixelRecive=0; imagePixelRecive<imageColumns; imagePixelRecive++){
        bluePixelsRecieve[cubeRow*imageColumns+imagePixelRecive] = hsiCube[cubeRow][imagePixelRecive];
        greenPixelsRecieve[cubeRow*imageColumns+imagePixelRecive] = hsiCube[cubeRow][imagecolumns+imagePixelRecive];
        redPixelsRecieve[cubeRow*imageColumns+imagePixelRecive] = hsiCube[cubeRow][2*imagecolumns+imagePixelRecive];
      }
    }

    cv::Mat your_matrix = cv::Mat(2736, 3648, CV_8UC1, &bluePixelsRecieve);
    imwrite("your_matrix.png",your_matrix);


}
