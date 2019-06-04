#ifndef INCLUDE_HSICAMERA_HPP
#define INCLUDE_HSICAMERA_HPP

#include <ueye.h>
#include <stdlib.h>

extern "C" {
#include "DMA_kernel_module/dma_parameters.h"
}

enum cubeFormat { Bil, Bip, Bsq, Raw, None };
enum cameraTriggerMode {Freerun, Swtrigger, Hwtrigger};
enum binningMode {simdMedian, simdMean, normalMean, testBinn};

class HSICamera
{
    public:
      HSICamera();
      void initialize(double exposureMs, int rows, int columns, int frames, double fps, cubeFormat cube);
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
      uint8_t* recieve_channel;

      // Sensor spec
      int g_sensor_rows_count;
      int g_sensor_columns_count;
      int g_bands_count;
      int g_bit_depth = 16;

      // Cube spec
      int g_cube_clumns_count;
      int g_cube_rows_count;
      cubeFormat cube_type; //a=bil b=bip c=bsq

      // Driver spec
      int g_frame_count;
      double g_framerate;
      cameraTriggerMode triggermode = Freerun;

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
      void writeRawDataToFile(uint8_t* data, int count);
      void writeRawDataToFile(uint12_t* data, int count);
      void writeSingleToFile();
      void writeBandsToSeparateFiles();
      void captureSIMDMedianBinning();
      void captureMeanBinning();
      void captureSIMDMeanBinning();
      void testBinning();
      void writeCubeToFile();
      void transferDMA();

      const int RAWFRAMESCOUNT = 10;
      const binningMode binning_method = simdMean;
      const int BINNINGFACTOR = 12;
      int image_x_offset_sensor;
      int image_y_offset_sensor;

      // Binning
      void bitonicMerge12(uint16_t* arr);
      void bitonicMerge8(uint16_t* arr);
      void bitonicMerge6(uint16_t* arr);
      void bitonicMerge4(uint16_t* arr);

      void swap(uint16_t *xp, uint16_t *yp);
      void bubbleSort(uint16_t arr[], int n);
};


#endif
