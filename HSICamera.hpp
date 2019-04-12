#ifndef INCLUDE_HSICAMERA_HPP
#define INCLUDE_HSICAMERA_HPP

#include <ueye.h>
#include <stdlib.h>
// #include "../DMA_kernel_module/dma_parameters.h"

extern "C" {
#include "DMA_kernel_module/dma_parameters.h"
}
// #include "CubeDMADriver.hpp"

enum cubeFormat { Bil, Bip, Bsq };
enum cameraTriggerMode {Freerun, Swtrigger, Hwtrigger};
enum binningMode {simdMedian, simdMean, normalMean, testBinn};

class HSICamera
{
    public:
      HSICamera();
      void initialize(int pixelClockMHz, int resolution, double exposureMs, int rows, int columns, int frames, double fps, cameraTriggerMode cameraMode, cubeFormat cube);
      void runCubeCapture();
      void captureSingleImage();

    private:

      HIDS camera = 1;


      char** p_imagesequence_camera;
      int* p_frame_ID;


      int fd_send;
      int fd_recieve;
      char* p_raw_single_image;
      uint16_t** p_hsi_cube;
      uint16_t** p_binned_frames;

      struct dma_data* send_channel;
      struct dma_data* recieve_channel;

      // Sensor spec
      int g_sensor_rows_count;
      int g_sensor_columns_count;
      int g_bands_count;
      int g_bit_depth;

      // Cube spec
      int g_cube_clumns_count;
      int g_cube_rows_count;
      cubeFormat cube_type; //a=bil b=bip c=bsq

      // Driver spec
      int g_frame_count;
      double g_framerate;
      cameraTriggerMode triggermode;

      // Binning

      int g_samples_last_bin_count;
      int g_bands_binned_per_row_count;
      int g_full_binns_per_row_count;

      bool even_bin_count;
      int g_binned_pixels_per_frame_count;

      //Debug variable
      double newFrameRate;

      void swTriggerCapture();
      void freeRunCapture();
      void writeRawDataToFile(uint12_t* data, int count);
      void writeSingleToFile();
      void writeBandsToSeparateFiles();
      void writeRawDataToFile(char**, int, int);
      void writeRawDataToFile(uint16_t**, int, int);
      void captureSIMDMedianBinning();
      void captureMeanBinning();
      void captureSIMDMeanBinning();
      void testBinning();
      void writeCubeToFile();
      void transferDMA();
      // Binning
      int random_partition(unsigned char* arr, int start, int end);
      int random_selection(unsigned char* arr, int start, int end, int k);
      void insertionSort(uint16_t* arr, int startPosition, int n);
      void swap(unsigned char* a, unsigned char* b);
      int partition (unsigned char arr[], int low, int high);
      void quickSort(unsigned char arr[], int low, int high);
      void bubbleSort(uint16_t arr[], int n);
      void swap(uint16_t *xp, uint16_t *yp);
      void bitonicMerge12(uint16_t* arr);
      void bitonicMerge6(uint16_t* arr);
};


#endif
