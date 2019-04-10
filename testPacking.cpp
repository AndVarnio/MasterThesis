#include <fstream>
#include <iterator>
#include <vector>
#include<time.h>
#include <sys/time.h>
// #include <opencv2/highgui.hpp>
// #include <opencv2/core/core.hpp>
#include <omp.h>
#include <iostream>
// g++ readBIL.cpp -o readBILCube `pkg-config --cflags --libs opencv`
int columns = 1024;
int bands = 85;
int rows = 10;

//
// #pragma pack(push, 1)
// typedef struct
// {
//     uint32_t value0_12x1:12;
//     uint32_t value1_12x1:12;
// } uint24_t;
// #pragma pack(pop)


// #define SCALE12_16  16 // 65536/4096
void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i=size-1;i>=0;i--)
    {
        for (j=7;j>=0;j--)
        {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    // puts("");
}

#pragma pack(push, 1)
typedef struct {
  uint16_t value0_12x1:12;
  // uint32_t value1_12x1:12;
} uint24_t;
#pragma pack(pop)


// #pragma pack(push, 1)
// typedef struct {
//         union {
//                 struct {
//                         uint32_t value0_24x1:24;
//                 };
//                 struct {
//                         uint32_t value0_12x1:12;
//                         uint32_t value1_12x1:12;
//                 };
//                 struct {
//                         uint32_t value0_8x1:8;
//                         uint32_t value1_8x1:8;
//                         uint32_t value3_8x1:8;
//                 };
//         };
// } uint24_t;
// #pragma pack(pop)

// #pragma pack(push, 1)
// typedef struct {
//
//
//                         uint32_t value0_12x1:12;
//                         uint32_t value1_12x1:12;
//
//
// } uint24_t;
// #pragma pack(pop)


uint16_t** waveLengths;
uint16_t* buffer_in;
uint24_t* buffer_out;
uint16_t* control_buffer;
// #pragma pack(push, 1)
// typedef union
// {
//   struct
//   {
//     uint32_t value0_12x1:12;
//     uint32_t value1_12x1:12;
//   } nibble;
//   uint32_t abyte;
// } tyvalue;
// #pragma pack(pop)
//
// union
// {
//   struct
//   {
//     unsigned char x:4;
//     unsigned char y:4;
//   } nibble;
//   unsigned char abyte;
// } value;
//
// typedef union
// {
//   struct
//   {
//     unsigned char x:4;
//     unsigned char y:4;
//   } nibble;
//   unsigned char abyte;
// } tyvalue;


static inline uint32_t __attribute__((always_inline)) saturate(uint32_t value)
{
  register uint32_t result;

  asm volatile("usat %0, %2, %1 \n\t"                     \
  : [result] "=r" (result)                        \
  : [value] "r" (value), [saturate] "I" (12)      \
  :                                               \
);

return result;
}

void compact_12bit(const uint16_t *input, uint24_t *output, uint32_t elements)
{
  printf("Size buffer out: %d\n", sizeof(buffer_out));
  printf("elements: %d\n", elements);

  for (uint32_t i = 0; i < elements; i++) {
    // if(i<20){
    //   printf("%d input = %u, output = %u\n", i, input[i], output[i].value0_12x1);
    // }
    // output[i].value0_12x1 = input[i];
    // if(i<20){
    //   printf("%d input = %u, output = %u\n", i, input[i], output[i].value0_12x1);
    // }
    (output++)->value0_12x1 = *input++;
  }
  // printf("\n" );
  // for(int i =0; i<20; i++){
  //   printf("%d input = %u, output = %u\n", i, input[i], output[i].value0_12x1);
  // }
  // #if 0
  /* More readable, but slower */
  // for (uint32_t i = 0; i < elements; i++) {
  //   output->value0_12x1 = *input++;
  //   (output++)->value1_12x1 = *input++;
  // }

  // for (uint32_t i = 0; i < elements; ++i, input += 2)
  //   (output++)->value0_24x1 = *input | ((uint32_t)*(input+1)) << 12;

  // #else
  // /* Alternative - less readable but faster */
  // for (uint32_t i = 0; i < elements; ++i, input += 2)
  // (output++)->value0_24x1 = saturate(*input) | ((uint32_t)saturate(*(input+1))) << 12;
  // #endif
}

void __attribute__((noinline, used)) compact(const uint16_t *input, uint24_t *output, uint32_t elements)
{
  printf("elements: %d\n", elements);
// #if 0
        /* More readable, but slower */
        // for (uint32_t i = 0; i < elements; ++i) {
        //         output->value0_12x1 = saturate(*input++);
        //         (output++)->value1_12x1 = saturate(*input++);
        // }
// #else
//         /* Alternative - less readable but faster */
//         for (uint32_t i = 0; i < elements; ++i, input += 2)
//                 (output++)->value0_24x1 = saturate(*input) | ((uint32_t)saturate(*(input+1))) << 12;
// #endif
}

void unpack_12bit(uint32_t elements)
{

  /* More readable, but slower */
  for (uint32_t i = 0; i < elements; i++) {
    control_buffer[i] = buffer_out[i].value0_12x1;
    // control_buffer[i*2] = buffer_out[i].value0_12x1;
    // control_buffer[(i*2)+1] = buffer_out[i].value1_12x1;
  }
}


int main(int argc, char* argv[])
{


  if (argc<2) {
    puts("Usage: a.out [PATH TO RAW]");
    return -1;
  }
    // buffer_in = malloc(columns*bands*rows);
    const int SIZEINPUTFILE = columns*bands*rows;
    buffer_in = new uint16_t[SIZEINPUTFILE];

    waveLengths = new uint16_t*[bands];
    for(int band=0; band<bands; band++){
      waveLengths[band] = new uint16_t[rows*columns];
    }

    FILE * pFile = fopen ( argv[1] , "rb" );
    if (pFile==NULL) {fputs ("File error\n",stderr); exit (1);}

    size_t result = fread (buffer_in, 2, SIZEINPUTFILE, pFile);
    printf("Read: %d, SIZEINPUTFILE: %d\n", result, SIZEINPUTFILE);
    if (result != SIZEINPUTFILE){

      fputs ("Reading error \n",stderr);
      exit (3);
    }

    for(int i=0; i<SIZEINPUTFILE; i++){
      buffer_in[i] = saturate(buffer_in[i]);
    }

    const int SIZEOUTPUTBUFFER = (SIZEINPUTFILE*3)/4;
    printf("Allocating out buffer %d\n", SIZEOUTPUTBUFFER);
    buffer_out = new uint24_t[SIZEINPUTFILE];

    printf("Starting to compact\n");
    /* Dividing by 2 because we process two input values in a single loop inside compact() */
    struct timeval  tv1, tv2, tv3, tv4;
    gettimeofday(&tv1, NULL);

    compact_12bit(buffer_in, buffer_out, SIZEINPUTFILE);
    gettimeofday(&tv2, NULL);
    double timess = (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
    printf("Packing time: %f ms\n", timess);
    // compact(buffer_in, buffer_out, 10);

    for(int i=0; i<20; i++){

      printf("in: %d ", buffer_in[i]);
      printBits(sizeof(buffer_in[i]), &buffer_in[i]);
      // printf("\n");
      printf(" out: %d ", buffer_out[i].value0_12x1);
      printBits(sizeof(buffer_out[i]), &buffer_out[i]);
      printf("\n");


      // sizeof(deviceMem[i]), &deviceMem[i]
      // printBits(sizeof(buffer_in[(i*2)]), &buffer_in[i*2]);
      // printf("\n");
      // printBits(sizeof(buffer_in[(i*2)+1]), &buffer_in[(i*2)+1]);
      // printf("\n");
      // printf("i: %d \n", buffer_out[i].value0_12x1);
      // printBits(sizeof(buffer_out[i]), &buffer_out[i]);
      // printf("\n");
    }
    control_buffer = new uint16_t[SIZEINPUTFILE];
    unpack_12bit(SIZEINPUTFILE);

    uint32_t matches = 0;
    uint32_t nPrinted = 0;
    for(int i=0; i<SIZEINPUTFILE; i++){

        if (buffer_in[i] == control_buffer[i]) {
          matches++;
        }



      else {
        if (nPrinted<20) {
          nPrinted++;
          // printf("%u: %4u %4u\n\r", i, source[i], destin[i]);
          // printf("%u: %4u %4u %u %u\n\r", i, buffer_in[i], control_buffer[i], buffer_out[i].value0_12x1, buffer_out[i].value1_12x1);
          printf("%u: %4u %4u %u\n\r", i, buffer_in[i], control_buffer[i], buffer_out[i].value0_12x1);

        }
      }
    }

    if (matches != SIZEINPUTFILE) {
  		fprintf(stderr, "ERROR: Only %f%% of the data matches\n\r", \
  				(double)matches*100/SIZEINPUTFILE);

  	}
  	else {
  		printf("Transfer success!\n\r");
  	}
//     #pragma omp parallel for num_threads(4)
//     for(int band=0; band<bands; band++){
//       for(int row=0; row<rows; row++){
//         for(int column=0; column<columns; column++){
//           waveLengths[band][row*columns+column] = buffer_in[row*columns*bands+band*columns+column];
//         }
//       }
//     }
//
//     std::stringstream ss;
//     ss << argv[1];
//     std::string folderName = ss.str();
//     std::string newDirectory = "mkdir -p ./" +folderName+".Images";
//     system(newDirectory.c_str());
//
//
//
//       for(int band=0; band<bands; band++){
//
//         std::string filename = "./" +folderName+".Images" + "/" + std::to_string(band) + ".png";
//         cv::Mat grayScaleMat = cv::Mat(rows, columns, CV_16UC1, waveLengths[band]);
//         imwrite(filename,grayScaleMat);
//       }
//
//       //Make RGB image
//
//       std::vector<cv::Mat> rgbChannels;
//
// // RGB: 25 33 47
//           rgbChannels.push_back(cv::Mat(rows, columns, CV_16UC1, waveLengths[22]));
//           rgbChannels.push_back(cv::Mat(rows, columns, CV_16UC1, waveLengths[32]));
//           rgbChannels.push_back(cv::Mat(rows, columns, CV_16UC1, waveLengths[49]));
//
//           cv::Mat rgbImage;
//           cv::merge(rgbChannels, rgbImage);
//           std::string filename = "./" +folderName+".Images" + "/" + "color" + ".png";
//           imwrite(filename,rgbImage);
//
//
//
//     delete waveLengths;
    return 0;
}
