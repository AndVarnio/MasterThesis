#include <opencv2/highgui.hpp>
#include <iostream>
//g++ opencvImages.cpp -o output `pkg-config --cflags --libs opencv`
const int SIZE = 9980928;
static unsigned char arrayOfPixels[SIZE];

int main( int argc, char** argv ) {

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
  return 0;
}
