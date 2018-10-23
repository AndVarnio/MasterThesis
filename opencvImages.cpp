#include <opencv2/highgui.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <omp.h>
//g++ opencvImages.cpp -o output `pkg-config --cflags --libs opencv`
const int SIZE = 9980928;
static unsigned char arrayOfPixels[SIZE];

int columns = 1080;
int bands = 1148;
int rows = 480;

int cubeRows = rows;
int sensorColumns = bands;
int binningFactor = 1;
int sensorRows = columns;
int cubeColumns = bands*columns;

uchar** waveLengths = new uchar*[bands];
uchar** imagesFromCamera = new uchar*[rows];
uchar** hsiCube = new uchar*[bands];
cv::Mat allBands[1148];

int main( int argc, char** argv ) {
/*
  cv::Mat image;
  image = cv::imread("testColorImage.jpg" , CV_LOAD_IMAGE_COLOR);

  if(! image.data ) {
      std::cout <<  "Could not open or find the image" << std::endl ;
      return -1;
    }

	cv::Mat bgr[3];   //destination array
	cv::split(image,bgr);//split source

	cv::Scalar intensity = bgr[0].at<uchar>(0, 0);
	int intensityint = intensity[0];
  intensityint *= 3;

	unsigned char intensitychar = (char)intensityint;
  unsigned char intensitychar2 = (char)(intensityint>>8);
	std::cout << (int)intensitychar << std::endl;
  std::cout << (int)intensitychar2 << std::endl;
  std::cout << intensityint << std::endl;
	std::cout << intensity << std::endl;


  for(int i=0; i<2736; i++){
    for(int j=0; j<3648; j++){
      intensityint = bgr[0].at<uchar>(i, j);
      arrayOfPixels[i*3648+j] = bgr[0].at<uchar>(i, j);
      // arrayOfPixels[i] = 1;
    }
  }
  cv::Mat your_matrix = cv::Mat(2736, 3648, CV_8UC1, &arrayOfPixels);
  imwrite("your_matrix.png",your_matrix);

	imwrite("blue.png",bgr[0]); //blue channel

imwrite("green.png",bgr[1]); //green channel
imwrite("red.png",bgr[2]); //red channel

//  cv::namedWindow( "Display window", cv::WINDOW_AUTOSIZE );
 // cv::imshow( "Display window", image );

  cv::waitKey(0);
  */
  /*
  std::memcpy(bytes,image.data,size * sizeof(byte));

  cv::Mat bgr[3];
  bgr[0] = cv::imread("292.png" , CV_LOAD_IMAGE_GRAYSCALE);
  bgr[1] = cv::imread("734.png" , CV_LOAD_IMAGE_GRAYSCALE);
  bgr[2] = cv::imread("1147.png" , CV_LOAD_IMAGE_GRAYSCALE);

  std::vector<cv::Mat> channels;
  channels.push_back(bgr[1]);
  channels.push_back(bgr[0]);
  channels.push_back(bgr[2]);
  cv::Mat rgbImage;
  cv::merge(channels, rgbImage);

  imwrite("RedGreenBlue.png",rgbImage);
*/



hsiCube = new uchar*[cubeRows];
for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
  hsiCube[cubeRow] = new uchar[cubeColumns];//TODO pixeldepth
}

  for(int i=0; i<bands; i++){
    const
    std::string filename = "1539690035/" + std::to_string(i) + ".png";
    // printf("Reading filename:  %s\n", filena me.c_str());
    // char cStr[filename.size()+1];
    allBands[i] = cv::imread(filename.c_str() , CV_LOAD_IMAGE_GRAYSCALE);
  }
  printf("GOT\n");
// allBands[1147] = cv::imread("1147.png" , CV_LOAD_IMAGE_GRAYSCALE);
cv::Size s = allBands[114].size();
printf("Height=%i - Width=%i\n",s.height, s.width);
imwrite("AA.png",allBands[1147]);
  for(int band=0; band<bands; band++){
    waveLengths[band] = new uchar(rows*columns);
  }

  printf("GOTHERES %li\n", sizeof(uchar));

  for(int i=0; i<bands; i++){
    // printf("%i\n", i);
    uchar* charPTR = (uchar*)allBands[i].data;
    waveLengths[i] = charPTR;
    // std::memcpy(waveLengths[i][0], charPTR, 1);
    // waveLengths[i] = (uchar*)(allBands[i].data);
  }

  for(int i=0; i<rows*columns; i++){
    // printf("%i - %d\n",i, waveLengths[1147][0]);
  }
  unsigned char* grayScaleImage;
  // std::copy(std::begin(&waveLengths[1147]), std::end(&waveLengths[1147]), std::begin(grayScaleImage));

  cv::Mat oneImage = cv::Mat(rows, columns, CV_8UC1, waveLengths[1147]);
  s = oneImage.size();
  printf("Height=%i - Width=%i\n",s.height, s.width);

  imwrite("./AAout.png",oneImage);
  // std::memcpy(oneImage.data, waveLengths[1147], rows*690);

  for(int i=0; i<cubeRows; i++){
    // printf("%i\n", i);

    imagesFromCamera[i] = new uchar[bands*columns];

  }
  // Take one row for each image, place them in new array.
  // Image 1 goes in row 1
  #pragma omp parallel for num_threads(4)
  for(int band=0; band<bands; band++){
    for(int spatialImage=0; spatialImage<cubeRows; spatialImage++){
      for(int pixel=0; pixel<columns; pixel++){
        imagesFromCamera[spatialImage][band*columns+pixel] = waveLengths[band][spatialImage*columns+pixel];
      }
    }

  }


  for(int cubeRow=0; cubeRow<cubeRows; cubeRow++){
    //TODO Binning
    // Put rows together
    // store them in new array
    int nBandsBinned;
    if(sensorColumns%binningFactor==0){
      nBandsBinned = sensorColumns/binningFactor;
    }
    else{
      nBandsBinned = sensorColumns/binningFactor + 1;
    }
    nBandsBinned=1;
    // printf("Not yet!\n");
    for(int band=0; band<bands; band++){
      for(int pixelInCubeRow=0; pixelInCubeRow<sensorRows; pixelInCubeRow++){
        hsiCube[cubeRow][band*sensorRows+pixelInCubeRow] = imagesFromCamera[cubeRow][nBandsBinned*(sensorRows-1)-nBandsBinned*pixelInCubeRow+band];
      }
    }


    /*
    for(int band=0; band<bands; band++){
      for(int pixelInCubeRow=0; pixelInCubeRow<sensorRows; pixelInCubeRow++){
        hsiCube[cubeRow][band*sensorRows+pixelInCubeRow] = ppcMem[cubeRow][sensorColumns*(sensorRows-1)-sensorColumns*pixelInCubeRow+band];
      }
    }*/
    // usleep(captureInterval);
  }

  #pragma omp parallel for num_threads(4)
  for(int band=0; band<bands; band++){
    for(int row=0; row<rows; row++){
      for(int column=0; column<columns; column++){
        // printf("Tick: %i, %i, %i\n", band, row, column);
        waveLengths[band][row*columns+column] = hsiCube[row][band*columns+column];
      }
    }
  }

  printf("Tick\n");

  cv::Mat grayScaleMat = cv::Mat(rows, columns, CV_8UC1, waveLengths[500]);
  s = grayScaleMat.size();
  printf("Height=%i - Width=%i\n",s.height, s.width);
  imwrite("decubed.png",grayScaleMat);

  std::ofstream ofs;

  ofs.open( "timeString.raw", std::ofstream::out|std::ofstream::binary|std::ios_base::app );
  if (!ofs.is_open())
  {
    printf("ofs not open\n");
  }
  // ofs.write( pMem, sensorColumns*sensorRows );//TODO bitDepth
  printf("Size: %i - %iMB\n", (cubeRows*cubeColumns), ((cubeRows*cubeColumns)/(1000*1000)));
  for(int i=0; i<cubeRows; i++){
    ofs.write((char*)(&hsiCube[i]), cubeColumns );
    // ofs << hsiCube[i];
    //ofs.write( &linebreak, 1 );
    // ofs << hsiCube[i];
    // ofs << "\n";
  }
  ofs.close();
  return 0;
}
