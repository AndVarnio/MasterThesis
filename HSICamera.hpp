#include<ueye.h>
#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include<time.h>
#include <sys/time.h>
#include <arm_neon.h>

enum cubeFormat { Bil, Bip, Bsq };
enum cameraTriggerMode {Freerun, Swtrigger, Hwtrigger};
enum binningMode {simdMedian, normalMean};

class HSICamera
{
    public:
      HSICamera();
      void initialize(int pixelClockMHz, int resolution, double exposureMs, int rows, int columns, int frames, double fps, cameraTriggerMode cameraMode, cubeFormat cube);
      void runCubeCapture();
      void captureSingleImage();

    private:
      HIDS camera = 1;

      const int RAWFRAMESCOUNT = 10;
      char* p_raw_single_image = NULL;
      char** p_imagesequence_camera = new char*[RAWFRAMESCOUNT];
      uint16_t** p_hsi_cube;
      uint16_t** p_binned_frames;


      // Sensor spec
      int g_sensor_rows_count;
      int g_sensor_columns_count;
      int g_bit_depth = 16;
      int g_bands_count;

      // Cube spec
      int g_cube_clumns_count;
      int g_cube_rows_count = 0;
      cubeFormat cube_type = Bsq; //a=bil b=bip c=bsq

      // Driver spec
      int g_frame_count = 100;
      double g_framerate = 28;
      cameraTriggerMode triggermode;

      // Binning
      int g_samples_last_bin_count = 0;
      int g_bands_binned_per_row_count;
      int g_full_binns_per_row_count = 0;
      const int BINNINGFACTOR = 12;
      const bool meanBinning = true;
      const binningMode binning_method = simdMedian;

      //Debug variable
      double newFrameRate;

      void swTriggerCapture();
      void freeRunCapture();
      void writeCubeToFile();
      void writeSingleToFile();
      void writeBandsToSeparateFiles();
      void writeRawDataToFile(char**, int, int);
      void writeRawDataToFile(uint16_t**, int, int);
      void captureSIMDMedianBinning();
      void captureMeanBinning();

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
};

HSICamera::HSICamera(){
  is_SetErrorReport (camera, IS_ENABLE_ERR_REP);
  INT success = is_InitCamera(&camera, NULL);
  if(success!=IS_SUCCESS){
    printf("Failed to initialize camera!, error message: %i\n", success);
    exit (EXIT_FAILURE);
  }
}

void HSICamera::initialize(int pixelClockMHz, int resolution, double exposureMs, int rows, int columns, int frames, double fps, cameraTriggerMode cameraMode, cubeFormat cube){

  printf("Initializing camera parameters\n");
  g_sensor_rows_count = rows;
  g_sensor_columns_count = columns;
  g_bands_count = columns;
  g_bands_binned_per_row_count = g_bands_count;
  UINT pixel_clock = pixelClockMHz;
  g_frame_count = frames;
  triggermode = cameraMode;
  cube_type = cube;
  g_framerate = fps;

  ////////////////////Set up binning variables////////////////////
  g_samples_last_bin_count = g_bands_count%BINNINGFACTOR;
  g_full_binns_per_row_count = g_bands_count/BINNINGFACTOR;
  g_bands_binned_per_row_count = (g_bands_count + BINNINGFACTOR - 1) / BINNINGFACTOR;

  if(cube_type==Bil || cube_type==Bip){

    g_cube_clumns_count = g_sensor_rows_count*g_bands_binned_per_row_count;
    g_cube_rows_count = g_frame_count;

  }
  else{//BSQ
    g_cube_clumns_count = g_sensor_rows_count;
    g_cube_rows_count = g_frame_count*g_bands_binned_per_row_count;
  }

  ////////////////////Initialize///////////////////
  int errorCode = is_PixelClock(camera, IS_PIXELCLOCK_CMD_SET,
                        (void*)&pixel_clock,
                        sizeof(pixel_clock));
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the pixel clock, error code: %d\n", errorCode);
  };

  // Get actual pixel clock range and fps for debugging
    UINT nRange[3];
    ZeroMemory(nRange, sizeof(nRange));
    INT nRet = is_PixelClock(camera, IS_PIXELCLOCK_CMD_GET_RANGE, (void*)nRange, sizeof(nRange));

    if (nRet == IS_SUCCESS){
      // UINT nMin = nRange[0];
      // UINT nMax = nRange[1];
      // UINT nInc = nRange[2];
      printf("min: %d max: %d intervall: %d\n", nRange[0], nRange[1], nRange[2]);
    }

  double min, max, intervall;
  is_GetFrameTimeRange (camera, &min, &max, &intervall);
  printf("min: %f max: %f intervall: %f\n", min, max, intervall);
  ////////////////////////

  errorCode = is_SetDisplayMode(camera, IS_SET_DM_DIB);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the display mode, error code: %d\n", errorCode);
  };

  UINT res = resolution;
  errorCode = is_ImageFormat (camera, IMGFRMT_CMD_SET_FORMAT, &res, 4);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the resolution setup, error code: %d\n", errorCode);
  };

  double expTime = exposureMs;
  errorCode = is_Exposure(camera, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*)&expTime, sizeof(expTime));
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the exposure time, error code: %d\n", errorCode);
  };

  errorCode = is_SetColorMode(camera, IS_CM_MONO12);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the color mode, error code: %d\n", errorCode);
  };

  // errorCode = is_SetGainBoost(camera, IS_SET_GAINBOOST_ON);
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with the gainboost, error code: %d\n", errorCode);
  // };
  //
  // errorCode = is_SetHardwareGain (camera, 50, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with the hardware gain, error code: %d\n", errorCode);
  // };

  ////////////// Allocate memory
  switch(triggermode)
  {
    case Swtrigger:
      {
      int memID = 0;
      is_AllocImageMem(camera, g_sensor_columns_count, g_sensor_rows_count, g_bit_depth, &p_raw_single_image, &memID);
      is_SetImageMem(camera, p_raw_single_image, memID);
      is_SetExternalTrigger(camera, IS_SET_TRIGGER_SOFTWARE);
      }
      break;

    case Hwtrigger:
    case Freerun:
      {
      for(int imageMemory=0; imageMemory<=RAWFRAMESCOUNT; imageMemory++){
        is_AllocImageMem(camera, g_sensor_columns_count, g_sensor_rows_count, 16, &p_imagesequence_camera[imageMemory], &imageMemory);
        is_AddToSequence (camera, p_imagesequence_camera[imageMemory], imageMemory);
      }

      is_InitImageQueue (camera, 0);

      p_binned_frames = new uint16_t*[g_frame_count];
      for(int image=0; image<g_frame_count; image++){
        p_binned_frames[image] = new uint16_t[g_bands_binned_per_row_count*g_sensor_rows_count];//TODO pixeldepth
      }

      errorCode = is_SetFrameRate(camera, g_framerate, &newFrameRate);
      if(errorCode!=IS_SUCCESS){
        printf("Something went wrong with setting the framerate, error code: %d\n", errorCode);
      };

      errorCode = is_CaptureVideo (camera, IS_WAIT);
      if(errorCode!=IS_SUCCESS){
        printf("Something went wrong with putting camera in freerun mode, error code: %d\n", errorCode);
      };
      }
      break;
    default:
      break;
  }
}

void HSICamera::runCubeCapture(){
  switch(triggermode)
  {
    case Swtrigger:
      swTriggerCapture();
      break;
    case Hwtrigger:
    case Freerun:
      freeRunCapture();
      break;
    default:
      break;
  }
}

void HSICamera::swTriggerCapture(){
    for(int cubeRow=0; cubeRow<g_cube_rows_count; cubeRow++){
      is_FreezeVideo(camera, IS_WAIT);

      for(int band=0; band<g_bands_count; band++){
        for(int pixel=0; pixel<g_sensor_rows_count; pixel++){
          p_hsi_cube[cubeRow][band*g_sensor_rows_count+pixel] = p_raw_single_image[g_sensor_columns_count*(g_sensor_rows_count-1)-g_sensor_columns_count*pixel+band];
        }
      }
        // usleep(20000);
    }
}
void HSICamera::freeRunCapture(){
  ////////////// For debugging
  double dblFPS;
  is_GetFramesPerSecond (camera, &dblFPS);
  printf("Actual framerate: %f\n", dblFPS);
  ///////////////////////////

  switch(binning_method)
  {
    case simdMedian:
      captureSIMDMedianBinning();
      break;
    case normalMean:
      captureMeanBinning();
      break;
    default:
      break;
  }

  char file_name_cube[64];
  strcpy(file_name_cube, "cube.raw");
  FILE * p_file_cube = fopen ( file_name_cube , "wb" );
  if (p_file_cube==NULL) {fputs ("File error\n",stderr); exit (1);}

  uint16_t* column_cube = new uint16_t[g_cube_clumns_count];
  ///Make cube
  if(cube_type==Bil){//BIL
    for(int cube_row=0; cube_row<g_cube_rows_count; cube_row++){
      for(int band=0; band<g_bands_binned_per_row_count; band++){
        for(int pixel=0; pixel<g_sensor_rows_count; pixel++){
          column_cube[band*g_sensor_rows_count+pixel] = p_binned_frames[cube_row][g_bands_binned_per_row_count*(g_sensor_rows_count-1)-g_bands_binned_per_row_count*pixel+band];
        }
      }
      fwrite (column_cube, sizeof(uint16_t), g_sensor_rows_count*g_full_binns_per_row_count, p_file_cube);
    }
  }
  else if(cube_type==Bip){//BIP
    for(int cube_row=0; cube_row<g_cube_rows_count; cube_row++){
      for(int pixel=0; pixel<g_sensor_rows_count; pixel++){
        for(int band=0; band<g_full_binns_per_row_count; band++){
          column_cube[pixel*g_full_binns_per_row_count+band] = p_binned_frames[cube_row][g_full_binns_per_row_count*(g_sensor_rows_count-1)-g_full_binns_per_row_count*pixel+band];
        }
      }
      fwrite (column_cube, sizeof(uint16_t), g_sensor_rows_count*g_full_binns_per_row_count, p_file_cube);
    }
  }
  else{//BSQ
    for(int band=0; band<g_full_binns_per_row_count; band++){
      for(int rowSpatial=0; rowSpatial<g_frame_count; rowSpatial++){
        for(int pixel=0; pixel<g_cube_clumns_count; pixel++){
          column_cube[pixel] = p_binned_frames[rowSpatial][g_bands_binned_per_row_count*(g_sensor_rows_count-1)-g_bands_binned_per_row_count*pixel+band];
        }
        fwrite (column_cube, sizeof(uint16_t), g_sensor_rows_count*g_full_binns_per_row_count, p_file_cube);
      }
    }
    fclose (p_file_cube);
  }
}

void HSICamera::captureSIMDMedianBinning(){
  ////////For debugging
  struct timeval  tv1, tv2, tv3, tv4;
  double totTime = 0;
  ////////////////////

  char* p_raw_frame;
  int imagesequence_id = 0;
  int last_pixel_in_row_offset = g_full_binns_per_row_count*BINNINGFACTOR;

  for(int image_number=0; image_number<g_frame_count; image_number++){
    gettimeofday(&tv1, NULL);
    int errorCode;
    do{
      errorCode = is_WaitForNextImage(camera, 1000, &(p_raw_frame), &imagesequence_id);

      if(errorCode!=IS_SUCCESS){
        is_UnlockSeqBuf (camera, 1, p_raw_frame);
        printf("Something went wrong with the is_WaitForNextImage, error code: %d\n", errorCode);
      }
    }while(errorCode!=IS_SUCCESS);

    // Copy values to 16 bit array
    uint16_t p_pixels_in_frame[g_sensor_rows_count*g_sensor_columns_count];
    for(int i=0; i<g_sensor_rows_count*g_sensor_columns_count; i++){
      p_pixels_in_frame[i] = uint16_t(p_raw_frame[i*2]) << 8 | p_raw_frame[i*2+1] ;
    }

    gettimeofday(&tv1, NULL);
    // Binn image
    #pragma omp parallel for num_threads(2)
    for(int sensor_row=0; sensor_row<g_sensor_rows_count; sensor_row++){
      int row_offset = sensor_row*g_sensor_columns_count;
      int binned_frames_offset = sensor_row*g_bands_binned_per_row_count;

      int bin_offset = 0;
      for(int bin_number=0; bin_number<g_full_binns_per_row_count; bin_number++){

        int row_and_bin_offset_raw_frame = row_offset+bin_offset;
        bitonicMerge12(p_pixels_in_frame+row_and_bin_offset_raw_frame);
        p_binned_frames[image_number][binned_frames_offset+bin_number] = p_pixels_in_frame[row_and_bin_offset_raw_frame+6];
        bin_offset += BINNINGFACTOR;
      }
      if(g_samples_last_bin_count>0){
        bubbleSort(p_pixels_in_frame+row_offset+last_pixel_in_row_offset, g_samples_last_bin_count);
        //insertionSort(p_raw_frame, row_offset+last_pixel_in_row_offset, g_samples_last_bin_count);
        p_binned_frames[image_number][binned_frames_offset+g_full_binns_per_row_count] = p_pixels_in_frame[last_pixel_in_row_offset+(g_samples_last_bin_count/2)];
      }
    }
    is_UnlockSeqBuf (camera, 1, p_raw_frame);
    gettimeofday(&tv2, NULL);
    totTime += (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
}

  printf("avg binning time: %f\n", totTime/g_frame_count);


  for(int imageMemory=1; imageMemory<=RAWFRAMESCOUNT; imageMemory++){
    is_FreeImageMem (camera, p_imagesequence_camera[imageMemory], imageMemory);
  }
}

void HSICamera::captureMeanBinning(){
  ////////For debugging
  struct timeval  tv1, tv2, tv3, tv4;
  double totTime = 0;
  ////////////////////

  char* p_raw_frame;
  int imagesequence_id = 0;
  int last_pixel_in_row_offset = g_full_binns_per_row_count*BINNINGFACTOR;

  for(int image_number=0; image_number<g_frame_count; image_number++){
    gettimeofday(&tv1, NULL);
    int errorCode;
    do{
      errorCode = is_WaitForNextImage(camera, 1000, &(p_raw_frame), &imagesequence_id);

      if(errorCode!=IS_SUCCESS){
        is_UnlockSeqBuf (camera, 1, p_raw_frame);
        printf("Something went wrong with the is_WaitForNextImage, error code: %d\n", errorCode);
      }
    }while(errorCode!=IS_SUCCESS);

    // Copy values to 16 bit array
    uint16_t p_pixels_in_frame[g_sensor_rows_count*g_sensor_columns_count];
    for(int i=0; i<g_sensor_rows_count*g_sensor_columns_count; i++){
      p_pixels_in_frame[i] = uint16_t(p_raw_frame[i*2]) << 8 | p_raw_frame[i*2+1] ;
    }

    gettimeofday(&tv1, NULL);
    // Binn image
    #pragma omp parallel for num_threads(2)
    for(int sensor_row=0; sensor_row<g_sensor_rows_count; sensor_row++){
      int row_offset = sensor_row*g_sensor_columns_count;
      int binned_frames_offset = sensor_row*g_bands_binned_per_row_count;

      int bin_offset = 0;
      for(int bin_number=0; bin_number<g_full_binns_per_row_count; bin_number++){
        int row_and_bin_offset_raw_frame = row_offset+bin_offset;
        int tot_pixel_val = 0;

        tot_pixel_val += (int)p_pixels_in_frame[row_and_bin_offset_raw_frame+0];
        tot_pixel_val += (int)p_pixels_in_frame[row_and_bin_offset_raw_frame+1];
        tot_pixel_val += (int)p_pixels_in_frame[row_and_bin_offset_raw_frame+2];
        tot_pixel_val += (int)p_pixels_in_frame[row_and_bin_offset_raw_frame+3];
        tot_pixel_val += (int)p_pixels_in_frame[row_and_bin_offset_raw_frame+4];
        tot_pixel_val += (int)p_pixels_in_frame[row_and_bin_offset_raw_frame+5];
        tot_pixel_val += (int)p_pixels_in_frame[row_and_bin_offset_raw_frame+6];
        tot_pixel_val += (int)p_pixels_in_frame[row_and_bin_offset_raw_frame+7];
        tot_pixel_val += (int)p_pixels_in_frame[row_and_bin_offset_raw_frame+8];
        tot_pixel_val += (int)p_pixels_in_frame[row_and_bin_offset_raw_frame+9];
        tot_pixel_val += (int)p_pixels_in_frame[row_and_bin_offset_raw_frame+10];
        tot_pixel_val += (int)p_pixels_in_frame[row_and_bin_offset_raw_frame+11];

        p_binned_frames[image_number][binned_frames_offset+bin_number] = (uint16_t)(tot_pixel_val/BINNINGFACTOR);

        bin_offset += BINNINGFACTOR;
      }
      if(g_samples_last_bin_count>0){
        int tot_pixel_val = 0;
        for(int bin_number=0; bin_number<g_samples_last_bin_count; bin_number++){
          tot_pixel_val = p_pixels_in_frame[g_bands_binned_per_row_count*BINNINGFACTOR+bin_number];
        }
        p_binned_frames[image_number][binned_frames_offset+g_full_binns_per_row_count] = (uint16_t)(tot_pixel_val/g_samples_last_bin_count);

        }
    }
    is_UnlockSeqBuf (camera, 1, p_raw_frame);
    gettimeofday(&tv2, NULL);
    totTime += (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
}

  printf("avg binning time: %f\n", totTime/g_frame_count);


  for(int imageMemory=1; imageMemory<=RAWFRAMESCOUNT; imageMemory++){
    is_FreeImageMem (camera, p_imagesequence_camera[imageMemory], imageMemory);
  }
}


void HSICamera::writeRawDataToFile(char** data, int rows_count, int columns_count){
  //Name file
  struct timespec time_system;
  clock_gettime(CLOCK_REALTIME, &time_system);
  char timeSystemString[32];
  sprintf(timeSystemString, "%lli", (long long)time_system.tv_nsec);
  char file_path[64];
  strcpy(file_path, "/home/capture/");
  strcat(file_path, timeSystemString);
  strcat(file_path, "SensorData.raw");

  //Write
  FILE * fp;
  fp = fopen (file_path,"wb");

  for(int i=0; i<rows_count; i++){
    fwrite (data[i], sizeof(char), columns_count, fp);//TODO g_bit_depth
  }
  fclose (fp);
}

void HSICamera::writeRawDataToFile(uint16_t** data, int rows_count, int columns_count){
  //Name file
  struct timespec time_system;
  clock_gettime(CLOCK_REALTIME, &time_system);
  char timeSystemString[32];
  sprintf(timeSystemString, "%lli", (long long)time_system.tv_nsec);
  char file_path[64];
  strcpy(file_path, "/home/capture/");
  strcat(file_path, timeSystemString);
  strcat(file_path, "SensorData.raw");

  //Write
  FILE * fp;
  fp = fopen (file_path,"wb");

  for(int i=0; i<rows_count; i++){
    fwrite (data[i], sizeof(char), columns_count, fp);//TODO g_bit_depth
  }
  fclose (fp);
}


void HSICamera::captureSingleImage(){
  is_FreezeVideo(camera, IS_WAIT);
  writeSingleToFile();
}

void HSICamera::writeSingleToFile(){

  //Name file
  struct timespec time_system;
  clock_gettime(CLOCK_REALTIME, &time_system);
  char timeSystemString[32];
  sprintf(timeSystemString, "%lli", (long long)time_system.tv_nsec);
  char file_path[64];
  strcpy(file_path, "/home/capture/");
  strcat(file_path, timeSystemString);
  strcat(file_path, "SensorData.raw");

  FILE * fp;
  fp = fopen (file_path,"wb");
  fwrite (p_raw_single_image, sizeof(char), g_sensor_columns_count*g_sensor_rows_count, fp);//TODO g_bit_depth
  fclose (fp);
}

void HSICamera::swap(uint16_t *xp, uint16_t *yp)
{
    uint16_t temp = *xp;
    *xp = *yp;
    *yp = temp;
}

void HSICamera::insertionSort(uint16_t arr[], int startPosition, int n)
{
  uint16_t key;
   int i, j;
   for (i = startPosition+1; i < startPosition+n; i++)
   {
      // printf("v - %u\n", *(arr+i));
      // *(arr+i) = 2;
      // printf("w - %u\n", *(arr+i));
       // key = arr[i];
       j = i-1;

       /* Move elements of arr[0..i-1], that are
          greater than key, to one position ahead
          of their current position */
       while (j >= 0 && arr[j] > key)
       {
           arr[j+1] = arr[j];
           j = j-1;
       }
       arr[j+1] = key;

   }
   // printf("arr end %p\n", arr);
}


void HSICamera::bubbleSort(uint16_t arr[], int n)
{
   int i, j;
   for (i = 0; i < n-1; i++)

       // Last i elements are already in place
       for (j = 0; j < n-i-1; j++)
           if (arr[j] > arr[j+1])
              swap(&arr[j], &arr[j+1]);
}


void HSICamera::bitonicMerge12(uint16_t* arr){

  // Load vectors
  uint16x8_t vec1_16x8, vec2_16x8;
  uint16x4_t vec1_16x4, vec2_16x4, vec3_16x4, vec4_16x4;
  uint16_t zeroPadding[4] = {0, 0, 0, 0};

  vec1_16x4 = vld1_u16(zeroPadding);
  vec2_16x4 = vld1_u16(arr);
  vec3_16x4 = vld1_u16(arr+4);
  vec4_16x4 = vld1_u16(arr+8);

  // Sorting across registers
  uint16x4_t max_vec_sort = vmax_u16(vec4_16x4, vec3_16x4);
  uint16x4_t min_vec_sort = vmin_u16(vec4_16x4, vec3_16x4);

  vec3_16x4 = vmin_u16(max_vec_sort, vec2_16x4);
  vec4_16x4 = vmax_u16(max_vec_sort, vec2_16x4);

  vec2_16x4 = vmin_u16(vec3_16x4, min_vec_sort);
  vec3_16x4 = vmax_u16(vec3_16x4, min_vec_sort);

  // Transpose vectors
  vec1_16x8 = vcombine_u16(vec1_16x4, vec2_16x4);
  vec2_16x8 = vcombine_u16(vec3_16x4, vec4_16x4);

  uint16x8x2_t interleavedVector = vzipq_u16(vec1_16x8, vec2_16x8);
  interleavedVector = vzipq_u16(interleavedVector.val[0], interleavedVector.val[1]);

  uint16x8_t reversed_vector = vrev64q_u16(interleavedVector.val[1]);

  // L1
  uint16x8_t max_vec = vmaxq_u16(reversed_vector, interleavedVector.val[0]);
  uint16x8_t min_vec = vminq_u16(reversed_vector, interleavedVector.val[0]);

  uint16x8x2_t shuffleTmp = vtrnq_u16(min_vec, max_vec);
  uint16x8x2_t interleavedTmp = vzipq_u16(shuffleTmp.val[0], shuffleTmp.val[1]);

  // Shuffle lines into right vectors
  uint16x4_t line1 = vget_high_u16(interleavedTmp.val[0]);
  uint16x4_t line3 = vget_low_u16(interleavedTmp.val[0]);
  uint16x4_t line2 = vget_high_u16(interleavedTmp.val[1]);
  uint16x4_t line4 = vget_low_u16(interleavedTmp.val[1]);
  uint16x8_t  vec1_L2 = vcombine_u16(line1, line2);
  uint16x8_t  vec2_L2 = vcombine_u16(line3, line4);

  // L2
  max_vec = vmaxq_u16(vec1_L2, vec2_L2);
  min_vec = vminq_u16(vec1_L2, vec2_L2);

  uint16x8x2_t vec1_L3 = vtrnq_u16(min_vec, max_vec);

  // L3
  max_vec = vmaxq_u16(vec1_L3.val[0], vec1_L3.val[1]);
  min_vec = vminq_u16(vec1_L3.val[0], vec1_L3.val[1]);

  uint16x8x2_t zippedResult = vzipq_u16(min_vec, max_vec);

  // Bitonic merge 8x2
  reversed_vector = vrev64q_u16(zippedResult.val[0]);
  uint16x4_t reversed_vector_high = vget_high_u16(reversed_vector);
  uint16x4_t reversed_vector_low = vget_low_u16(reversed_vector);
  reversed_vector = vcombine_u16(reversed_vector_high, reversed_vector_low);

  max_vec = vmaxq_u16(reversed_vector, zippedResult.val[1]);
  min_vec = vminq_u16(reversed_vector, zippedResult.val[1]);

  // Shuffle lines into right vectors
  uint16x4_t max_vec_high = vget_high_u16(max_vec);
  uint16x4_t max_vec_low = vget_low_u16(max_vec);
  uint16x4_t min_vec_high = vget_high_u16(min_vec);
  uint16x4_t min_vec_low = vget_low_u16(min_vec);
  uint16x8_t shuffled_vec1 = vcombine_u16(min_vec_low, max_vec_low);
  uint16x8_t shuffled_vec2 = vcombine_u16(min_vec_high, max_vec_high);

  uint16x8_t input_bitonic4x2_max = vmaxq_u16(shuffled_vec1, shuffled_vec2);
  uint16x8_t input_bitonic4x2_min = vminq_u16(shuffled_vec1, shuffled_vec2);


  // Bitonic merge 4x2
  // L1
  max_vec = vmaxq_u16(input_bitonic4x2_min, input_bitonic4x2_max);
  min_vec = vminq_u16(input_bitonic4x2_min, input_bitonic4x2_max);

  shuffleTmp = vtrnq_u16(min_vec, max_vec);
  interleavedTmp = vzipq_u16(shuffleTmp.val[0], shuffleTmp.val[1]);

  // Shuffle lines into right vectors
  line1 = vget_high_u16(interleavedTmp.val[0]);
  line3 = vget_low_u16(interleavedTmp.val[0]);
  line2 = vget_high_u16(interleavedTmp.val[1]);
  line4 = vget_low_u16(interleavedTmp.val[1]);
  vec1_L2 = vcombine_u16(line1, line2);
  vec2_L2 = vcombine_u16(line3, line4);

  // L2
  max_vec = vmaxq_u16(vec1_L2, vec2_L2);
  min_vec = vminq_u16(vec1_L2, vec2_L2);

  vec1_L3 = vtrnq_u16(min_vec, max_vec);

  // L3
  max_vec = vmaxq_u16(vec1_L3.val[0], vec1_L3.val[1]);
  min_vec = vminq_u16(vec1_L3.val[0], vec1_L3.val[1]);

  zippedResult = vzipq_u16(min_vec, max_vec);
  uint16x4_t lowestValues = vget_high_u16(zippedResult.val[0]);
  vst1_u16(arr, lowestValues);
  vst1q_u16(arr+4, zippedResult.val[1]);
}
