#include <ueye.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <arm_neon.h>
#include <algorithm>

#include <sys/mman.h>
#include <fcntl.h>

#include "CubeDMADriver.hpp"
#include "HSICamera.hpp"
#include <sys/ioctl.h>

#include <byteswap.h>
////////////////For debug
#define TIMEOUT 0xFFFFFF
// #define CUBE_SIZE (500 * 720 * 107)
///////////////////////////////////

HSICamera::HSICamera(){
  camera = 1;
  is_SetErrorReport (camera, IS_ENABLE_ERR_REP);
  INT success = is_InitCamera(&camera, NULL);
  if(success!=IS_SUCCESS){
    printf("Failed to initialize camera!, error message: %i\n", success);
    exit (EXIT_FAILURE);
  }
}

HSICamera::~HSICamera(){
  is_ExitCamera(camera);
}

void HSICamera::initialize(double exposureMs, int rows, int columns, int frames, double fps, cubeFormat cube){

  printf("Initializing camera parameters\n");
  g_sensor_rows_count = rows;
  // g_sensor_columns_count = columns/2;//Subsampling
  // g_bands_count = columns/2;  //Subsampling
  g_sensor_columns_count = columns;
  g_bands_count = columns;
  g_bands_binned_per_row_count = g_bands_count;
  g_frame_count = frames;
  g_framerate = fps;
  cube_type = cube;


  ////////////////////Set up binning variables////////////////////
  g_samples_last_bin_count = g_bands_count%BINNINGFACTOR;
  g_full_binns_per_row_count = g_bands_count/BINNINGFACTOR;
  g_bands_binned_per_row_count = (g_bands_count + BINNINGFACTOR - 1) / BINNINGFACTOR;
  g_binned_pixels_per_frame_count = g_bands_binned_per_row_count * g_sensor_rows_count;

  if(g_bands_binned_per_row_count % 2 == 0){
    even_bin_count = true;
  }
  else{
    even_bin_count = false;
  }

  if(cube_type==Bil || cube_type==Bip || cube_type==None){

    g_cube_clumns_count = g_sensor_rows_count*g_bands_binned_per_row_count;
    g_cube_rows_count = g_frame_count;

  }
  else{//BSQ
    g_cube_clumns_count = g_sensor_rows_count;
    g_cube_rows_count = g_frame_count*g_bands_binned_per_row_count;
  }

  ////////////////////Initialize///////////////////
  int errorCode;

  // Set maximum pixel clock
  UINT pixel_clock_range[3];
  errorCode = is_PixelClock(camera, IS_PIXELCLOCK_CMD_GET_RANGE, (void*)pixel_clock_range, sizeof(pixel_clock_range));
  if (errorCode == IS_SUCCESS){
    int errorCode = is_PixelClock(camera, IS_PIXELCLOCK_CMD_SET, (void*)&pixel_clock_range[1], sizeof(pixel_clock_range[1]));
    if(errorCode!=IS_SUCCESS){
      printf("Something went wrong with setting pixel clock, error code: %d\n", errorCode);
    };
  }
  else{
    printf("Something went wrong with getting pixel clock range, error code: %d\n", errorCode);
  }

  errorCode = is_SetDisplayMode(camera, IS_SET_DM_DIB);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the display mode, error code: %d\n", errorCode);
  };

  // Set AOI
  int sensor_width = 1936;
  int sensor_height = 1216;

  image_x_offset_sensor = (sensor_width-columns)/2;
  image_y_offset_sensor = (sensor_height-rows)/2;

  UINT res = 36; // Full resolution
  errorCode = is_ImageFormat (camera, IMGFRMT_CMD_SET_FORMAT, &res, 4);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the resolution setup, error code: %d\n", errorCode);
  };

  IS_RECT AOI_parameters;
  AOI_parameters.s32X     = image_x_offset_sensor | IS_AOI_IMAGE_POS_ABSOLUTE;
  AOI_parameters.s32Y     = image_y_offset_sensor | IS_AOI_IMAGE_POS_ABSOLUTE;
  AOI_parameters.s32Width = columns;
  AOI_parameters.s32Height = rows;

  errorCode = is_AOI( camera, IS_AOI_IMAGE_SET_AOI, (void*)&AOI_parameters, sizeof(AOI_parameters));
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the display mode, error code: %d\n", errorCode);
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

  // errorCode = is_SetSubSampling (camera, IS_SUBSAMPLING_2X_HORIZONTAL);
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with the subsampling, error code: %d\n", errorCode);
  // };

  errorCode = is_SetGainBoost(camera, IS_SET_GAINBOOST_ON);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the gainboost, error code: %d\n", errorCode);
  };

  errorCode = is_SetHardwareGain (camera, 50, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
  if(errorCode!=IS_SUCCESS){
    printf("Something went wrong with the hardware gain, error code: %d\n", errorCode);
  };

  ////////////// Allocate memory
  p_imagesequence_camera = new char*[RAWFRAMESCOUNT];
  p_frame_ID = new int[RAWFRAMESCOUNT];

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
      for(int imageMemory=0; imageMemory<RAWFRAMESCOUNT; imageMemory++){
        // is_AllocImageMem(camera, g_sensor_columns_count, g_sensor_rows_count, 16, &p_imagesequence_camera[imageMemory], &p_frame_ID[imageMemory]);
        // is_AddToSequence (camera, p_imagesequence_camera[imageMemory], p_frame_ID[imageMemory]);

        is_AllocImageMem(camera, sensor_width, sensor_height, 16, &p_imagesequence_camera[imageMemory], &p_frame_ID[imageMemory]);
        is_AddToSequence (camera, p_imagesequence_camera[imageMemory], p_frame_ID[imageMemory]);
      }

      is_InitImageQueue (camera, 0);

      errorCode = is_SetFrameRate(camera, g_framerate, &newFrameRate);
      if(errorCode!=IS_SUCCESS){
        printf("Something went wrong with setting the framerate, error code: %d\n", errorCode);
      };

      errorCode = is_CaptureVideo (camera, IS_WAIT);
      if(errorCode!=IS_SUCCESS){
        printf("Something went wrong with putting camera in freerun mode, error code: %d\n", errorCode);
        exit (EXIT_FAILURE);
      };
      }
      break;
    default:
      break;
  }

  ////Get pointer to DMA memory
  fd_send = open("/dev/cubedmasend", O_RDWR);
  if (fd_send < 1) {
    printf("Unable to open send channel");
  }

  fd_recieve = open("/dev/cubedmarecieve", O_RDWR);
  if (fd_recieve < 1) {
    printf("Unable to open receive channel");
  }

  send_channel = (struct dma_data *)mmap(NULL, sizeof(struct dma_data),
                  PROT_READ | PROT_WRITE, MAP_SHARED, fd_send, 0);

  recieve_channel = (uint8_t *)mmap(NULL, sizeof(struct dma_data),
                  PROT_READ | PROT_WRITE, MAP_SHARED, fd_recieve, 0);

  if ((send_channel == MAP_FAILED) || (recieve_channel == MAP_FAILED)) {
    printf("Failed to mmap\n");
  }
}


void HSICamera::runCubeCapture(){
  switch(triggermode)
  {
    case Swtrigger:
      swTriggerCapture();// TODO remove sw trigger mode
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
    }
}

void HSICamera::freeRunCapture(){

  switch(binning_method)
  {
    case simdMedian:
      captureSIMDMedianBinning();
      break;
    case normalMean:
      captureMeanBinning();
      break;
    case simdMean:
      captureSIMDMeanBinning();
      break;
    case testBinn:
      testBinning();
      break;
    default:
      break;
  }
  // Write un-compressed cube to file
  if(cube_type!=None){
    writeCubeToFile();
  }

  // Compress and write to file
  transferDMA();
  // writeRawDataToFile(send_channel->buffer, g_cube_rows_count*g_bands_binned_per_row_count*g_sensor_rows_count);

  printf("Freeing imageMemory\n");
  for(int imageMemory=0; imageMemory<RAWFRAMESCOUNT; imageMemory++){
    printf("Freeing: %d\n", imageMemory);
    is_UnlockSeqBuf (camera, p_frame_ID[imageMemory], p_imagesequence_camera[imageMemory]);
  }

  for(int imageMemory=0; imageMemory<RAWFRAMESCOUNT; imageMemory++){
    printf("Freeing: %d\n", imageMemory);
    is_FreeImageMem (camera, p_imagesequence_camera[imageMemory], p_frame_ID[imageMemory]);
  }
}

// Function used to test different binning methods
void HSICamera::testBinning(){
  printf("Running test\n");
  ////////For debugging
  struct timeval  tv1, tv2, tv3, tv4;
  double totTime = 0;
  gettimeofday(&tv1, NULL);
  ////////////////////


  char* p_raw_frame;
  INT imagesequence_id = 1;
  int last_pixel_in_row_offset = g_full_binns_per_row_count*BINNINGFACTOR;

  // INT errorCode = is_CaptureVideo (camera, IS_DONT_WAIT);
  // if(errorCode!=IS_SUCCESS){
  //   printf("Something went wrong with putting camera in freerun mode, error code: %d\n", errorCode);
  // };

  for(int image_number=0; image_number<g_frame_count; image_number++){
    // gettimeofday(&tv1, NULL);
    int errorCode;
    do{
      errorCode = is_WaitForNextImage(camera, 1000, &p_raw_frame, &imagesequence_id);

      // printf("Returned imageseq id: %d\n", imagesequence_id);
      if(errorCode!=IS_SUCCESS){
        UEYE_CAPTURE_STATUS_INFO CaptureStatusInfo;
        INT nRet2 = is_CaptureStatus(camera, IS_CAPTURE_STATUS_INFO_CMD_GET, (void*)&CaptureStatusInfo, sizeof(CaptureStatusInfo));

        printf("Total: %d\n", CaptureStatusInfo.dwCapStatusCnt_Total);
        printf("IS_CAP_STATUS_DRV_OUT_OF_BUFFERS: %d\n", CaptureStatusInfo.adwCapStatusCnt_Detail[IS_CAP_STATUS_DRV_OUT_OF_BUFFERS]);
        printf("IS_CAP_STATUS_API_CONVERSION_FAILED: %d\n", CaptureStatusInfo.adwCapStatusCnt_Detail[IS_CAP_STATUS_API_CONVERSION_FAILED]);
        printf("IS_CAP_STATUS_API_IMAGE_LOCKED: %d\n", CaptureStatusInfo.adwCapStatusCnt_Detail[IS_CAP_STATUS_API_IMAGE_LOCKED]);
        printf("IS_CAP_STATUS_DRV_OUT_OF_BUFFERS: %d\n", CaptureStatusInfo.adwCapStatusCnt_Detail[IS_CAP_STATUS_DRV_OUT_OF_BUFFERS]);
        printf("IS_CAP_STATUS_DRV_DEVICE_NOT_READY: %d\n", CaptureStatusInfo.adwCapStatusCnt_Detail[IS_CAP_STATUS_DRV_DEVICE_NOT_READY]);
        printf("IS_CAP_STATUS_USB_TRANSFER_FAILED: %d\n", CaptureStatusInfo.adwCapStatusCnt_Detail[IS_CAP_STATUS_USB_TRANSFER_FAILED]);
        printf("IS_CAP_STATUS_DEV_TIMEOUT: %d\n", CaptureStatusInfo.adwCapStatusCnt_Detail[IS_CAP_STATUS_DEV_TIMEOUT]);
        printf("IS_CAP_STATUS_ETH_BUFFER_OVERRUN: %d\n", CaptureStatusInfo.adwCapStatusCnt_Detail[IS_CAP_STATUS_ETH_BUFFER_OVERRUN]);
        printf("IS_CAP_STATUS_API_IMAGE_LOCKED: %d\n", CaptureStatusInfo.adwCapStatusCnt_Detail[IS_CAP_STATUS_API_IMAGE_LOCKED]);
        printf("IS_CAP_STATUS_ETH_MISSED_IMAGES: %d\n", CaptureStatusInfo.adwCapStatusCnt_Detail[IS_CAP_STATUS_ETH_MISSED_IMAGES]);
        printf("IS_CAP_STATUS_DEV_FRAME_CAPTURE_FAILED: %d\n", CaptureStatusInfo.adwCapStatusCnt_Detail[IS_CAP_STATUS_DEV_FRAME_CAPTURE_FAILED]);


        is_UnlockSeqBuf (camera, imagesequence_id, p_raw_frame);
        // printf("Something went wrong with the is_WaitForNextImage, error code: %d\n", errorCode);
      }
    }while(errorCode!=IS_SUCCESS);

    // gettimeofday(&tv2, NULL);
    // totTime += (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
    gettimeofday(&tv1, NULL);
    // Copy values to 16 bit array
    // AOI resolution
    int sensor_columns_count = 1936;
    int image_pos_offset_first_row = image_y_offset_sensor * sensor_columns_count * 2 + image_x_offset_sensor * 2;
    int image_pos_offset_next_row = 0;

    uint16_t p_pixels_in_frame[g_sensor_rows_count*g_sensor_columns_count];
    for(int image_row=0; image_row<g_sensor_rows_count; image_row++){
      for(int image_pixel=0; image_pixel<g_sensor_columns_count; image_pixel++){
        // printf("image_row: %d\n", image_row);
        p_pixels_in_frame[image_row*g_sensor_columns_count+image_pixel] = uint16_t(p_raw_frame[image_pos_offset_first_row+image_pos_offset_next_row+image_row*g_sensor_columns_count*2+image_pixel*2]) << 8
        | p_raw_frame[image_pos_offset_first_row+image_pos_offset_next_row+image_row*g_sensor_columns_count*2+image_pixel*2+1];
      }
      image_pos_offset_next_row += image_x_offset_sensor + image_x_offset_sensor + image_x_offset_sensor + image_x_offset_sensor; //TODO Fix odd image width number offset right sde of image
    }
    /////////////////////////////
    // printf("imageseq id: %d\n", imagesequence_id);
    // Normal resolution
    // uint16_t p_pixels_in_frame[g_sensor_rows_count*g_sensor_columns_count];
    // for(int i=0; i<g_sensor_rows_count*g_sensor_columns_count; i++){
    //   p_pixels_in_frame[i] = uint16_t(p_raw_frame[i*2]) << 8 | p_raw_frame[i*2+1] ;
    // }
    ///////////////////
    // printf("imageseq id: %d\n", imagesequence_id);
    // printf("%u %u-%u\n", *(p_pixels_in_frame+460800), (unsigned char)p_raw_frame[0+460800], (unsigned char)p_raw_frame[1+460800]);
    int binned_frames_offset = g_binned_pixels_per_frame_count * image_number;
    // printf("%d\n", image_number);
    // gettimeofday(&tv1, NULL);
    // Binn image
    #pragma omp parallel for num_threads(2)
    for(int sensor_row=0; sensor_row<g_sensor_rows_count; sensor_row++){
      // printf("imageseq id: %d\n", imagesequence_id);
      int row_offset = sensor_row*g_sensor_columns_count;
      int binned_rows_offset = sensor_row*g_bands_binned_per_row_count;

      int bin_offset = 0;
      for(int bin_number=0; bin_number<g_full_binns_per_row_count; bin_number++){

        int row_and_bin_offset_raw_frame = row_offset+bin_offset;
        // bubbleSort(p_pixels_in_frame+row_and_bin_offset_raw_frame, p_pixels_in_frame+row_and_bin_offset_raw_frame+12);
        // bitonicMerge4(p_pixels_in_frame+row_and_bin_offset_raw_frame);
        // bitonicMerge8(p_pixels_in_frame+row_and_bin_offset_raw_frame);
        bitonicMerge12(p_pixels_in_frame+row_and_bin_offset_raw_frame);
        // bubbleSort(p_pixels_in_frame+row_and_bin_offset_raw_frame, 12);
        // bitonicMerge6(p_pixels_in_frame+row_and_bin_offset_raw_frame);
        send_channel->buffer[binned_frames_offset+binned_rows_offset+bin_number].value = __bswap_16 (p_pixels_in_frame[row_and_bin_offset_raw_frame+6]);
        // send_channel->buffer[binned_frames_offset+binned_rows_offset+bin_number].value = p_pixels_in_frame[row_and_bin_offset_raw_frame+6];
        // p_binned_frames[image_number][binned_rows_offset+bin_number] = p_pixels_in_frame[row_and_bin_offset_raw_frame+6];
        bin_offset += BINNINGFACTOR;
      }
      if(g_samples_last_bin_count>0){
        bubbleSort(p_pixels_in_frame+row_offset+last_pixel_in_row_offset, g_samples_last_bin_count);
        //insertionSort(p_raw_frame, row_offset+last_pixel_in_row_offset, g_samples_last_bin_count);
        send_channel->buffer[binned_frames_offset+binned_rows_offset+g_full_binns_per_row_count].value = p_pixels_in_frame[last_pixel_in_row_offset+(g_samples_last_bin_count/2)];
        // p_binned_frames[image_number][binned_rows_offset+g_full_binns_per_row_count] = p_pixels_in_frame[last_pixel_in_row_offset+(g_samples_last_bin_count/2)];
      }
    }
    // printf("Unlocking imageseq id: %d\n", imagesequence_id);
    is_UnlockSeqBuf (camera, imagesequence_id, p_raw_frame);
    gettimeofday(&tv2, NULL);
    totTime += (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
}

  printf("avg binning time: %f\n", totTime/g_frame_count);
}

void HSICamera::captureSIMDMedianBinning(){
  char* p_raw_frame;
  INT imagesequence_id = 0;
  int last_pixel_in_row_offset = g_full_binns_per_row_count*BINNINGFACTOR;

  for(int image_number=0; image_number<g_frame_count; image_number++){

    int errorCode;
    do{
      errorCode = is_WaitForNextImage(camera, 1000, &p_raw_frame, &imagesequence_id);
      if(errorCode!=IS_SUCCESS){
        is_UnlockSeqBuf (camera, imagesequence_id, p_raw_frame);
        printf("Something went wrong with the is_WaitForNextImage, error code: %d\n", errorCode);
      }
    }while(errorCode!=IS_SUCCESS);


    int sensor_columns_count = 1936;
    int image_pos_offset_first_row = image_y_offset_sensor * sensor_columns_count * 2 + image_x_offset_sensor * 2;
    int image_pos_offset_next_row = 0;

    uint16_t p_pixels_in_frame[g_sensor_rows_count*g_sensor_columns_count];
    for(int image_row=0; image_row<g_sensor_rows_count; image_row++){
      for(int image_pixel=0; image_pixel<g_sensor_columns_count; image_pixel++){
        p_pixels_in_frame[image_row*g_sensor_columns_count+image_pixel] = uint16_t(p_raw_frame[image_pos_offset_first_row+image_pos_offset_next_row+image_row*g_sensor_columns_count*2+image_pixel*2]) << 8
        | p_raw_frame[image_pos_offset_first_row+image_pos_offset_next_row+image_row*g_sensor_columns_count*2+image_pixel*2+1];
      }
      image_pos_offset_next_row += image_x_offset_sensor + image_x_offset_sensor + image_x_offset_sensor + image_x_offset_sensor; //TODO Fix odd image width number offset right sde of image
    }

    int binned_frames_offset = g_binned_pixels_per_frame_count * image_number;
    // Binn image
    #pragma omp parallel for num_threads(2)
    for(int sensor_row=0; sensor_row<g_sensor_rows_count; sensor_row++){
      int row_offset = sensor_row*g_sensor_columns_count;
      int binned_rows_offset = sensor_row*g_bands_binned_per_row_count;

      int bin_offset = 0;
      for(int bin_number=0; bin_number<g_full_binns_per_row_count; bin_number++){

        int row_and_bin_offset_raw_frame = row_offset+bin_offset;
        bitonicMerge12(p_pixels_in_frame+row_and_bin_offset_raw_frame);

        send_channel->buffer[binned_frames_offset+binned_rows_offset+bin_number].value = __bswap_16 (p_pixels_in_frame[row_and_bin_offset_raw_frame+6]);
        bin_offset += BINNINGFACTOR;
      }
      if(g_samples_last_bin_count>0){
        bubbleSort(p_pixels_in_frame+row_offset+last_pixel_in_row_offset, g_samples_last_bin_count);

        send_channel->buffer[binned_frames_offset+binned_rows_offset+g_full_binns_per_row_count].value = p_pixels_in_frame[last_pixel_in_row_offset+(g_samples_last_bin_count/2)];
      }
    }
    is_UnlockSeqBuf (camera, imagesequence_id, p_raw_frame);
}
}

void HSICamera::captureSIMDMeanBinning(){

  uint16_t newArr1[BINNINGFACTOR];
  uint16_t newArr2[BINNINGFACTOR];
  uint16_t newArr3[BINNINGFACTOR];
  uint16_t newArr4[BINNINGFACTOR];

  char* p_raw_frame;
  int imagesequence_id = 0;
  int last_pixel_in_row_offset = g_full_binns_per_row_count*BINNINGFACTOR;

  for(int image_number=0; image_number<g_frame_count; image_number++){

    int errorCode;
    do{
      errorCode = is_WaitForNextImage(camera, 1000, &(p_raw_frame), &imagesequence_id);

      if(errorCode!=IS_SUCCESS){
        is_UnlockSeqBuf (camera, 1, p_raw_frame);
        printf("Something went wrong with the is_WaitForNextImage, error code: %d\n", errorCode);
      }
    }while(errorCode!=IS_SUCCESS);

    int sensor_columns_count = 1936;
    int image_pos_offset_first_row = image_y_offset_sensor * sensor_columns_count * 2 + image_x_offset_sensor * 2;
    int image_pos_offset_next_row = 0;

    uint16_t p_pixels_in_frame[g_sensor_rows_count*g_sensor_columns_count];
    for(int image_row=0; image_row<g_sensor_rows_count; image_row++){
      for(int image_pixel=0; image_pixel<g_sensor_columns_count; image_pixel++){
        p_pixels_in_frame[image_row*g_sensor_columns_count+image_pixel] = uint16_t(p_raw_frame[image_pos_offset_first_row+image_pos_offset_next_row+image_row*g_sensor_columns_count*2+image_pixel*2]) << 8
        | p_raw_frame[image_pos_offset_first_row+image_pos_offset_next_row+image_row*g_sensor_columns_count*2+image_pixel*2+1];
      }
      image_pos_offset_next_row += image_x_offset_sensor + image_x_offset_sensor + image_x_offset_sensor + image_x_offset_sensor; //TODO Fix odd image width number offset right sde of image
    }

    int binned_frames_offset = g_binned_pixels_per_frame_count * image_number;

    // Binn image
    #pragma omp parallel for num_threads(2)
    for(int sensor_row=0; sensor_row<g_sensor_rows_count; sensor_row++){
      int row_offset = sensor_row*g_sensor_columns_count;
      int binned_rows_offset = sensor_row*g_bands_binned_per_row_count;

      int bin_offset = 0;
      for(int bin_number=0; bin_number<g_full_binns_per_row_count-2; bin_number+=2){

        int row_and_bin_offset_raw_frame = row_offset+bin_offset;

        for(int i=0; i<4; i++){
            newArr1[i] = p_pixels_in_frame[row_and_bin_offset_raw_frame+i];
            newArr2[i] = p_pixels_in_frame[row_and_bin_offset_raw_frame+4+i];
            newArr3[i] = p_pixels_in_frame[row_and_bin_offset_raw_frame+8+i];
        }

        for(int i=0; i<4; i++){
            newArr1[4+i] = p_pixels_in_frame[row_and_bin_offset_raw_frame+BINNINGFACTOR+i];
            newArr2[4+i] = p_pixels_in_frame[row_and_bin_offset_raw_frame+BINNINGFACTOR+4+i];
            newArr3[4+i] = p_pixels_in_frame[row_and_bin_offset_raw_frame+BINNINGFACTOR+8+i];
        }


        uint16x8_t vec1_16x8, vec2_16x8, vec3_16x8, vec4_16x8;

        vec1_16x8 = vld1q_u16(newArr1);
        vec2_16x8 = vld1q_u16(newArr2);
        vec3_16x8 = vld1q_u16(newArr3);

        vec4_16x8 = vhaddq_u16(vec1_16x8, vec2_16x8);
        vec4_16x8 = vhaddq_u16(vec3_16x8, vec4_16x8);

        uint16_t totPixVal1 = vgetq_lane_u16(vec1_16x8, 0) + vgetq_lane_u16(vec1_16x8, 1) + vgetq_lane_u16(vec1_16x8, 2) + vgetq_lane_u16(vec1_16x8, 3);
        uint16_t totPixVal2 = vgetq_lane_u16(vec1_16x8, 4) + vgetq_lane_u16(vec1_16x8, 5) + vgetq_lane_u16(vec1_16x8, 6) + vgetq_lane_u16(vec1_16x8, 7);

        send_channel->buffer[binned_frames_offset+binned_rows_offset+bin_number].value = (unsigned char)(totPixVal1/BINNINGFACTOR);
        send_channel->buffer[binned_frames_offset+binned_rows_offset+bin_number+1].value = (unsigned char)(totPixVal2/BINNINGFACTOR);
        bin_offset += BINNINGFACTOR;

      }
      if(g_samples_last_bin_count>0){
        int tot_pixel_val = 0;
        for(int bin_number=0; bin_number<g_samples_last_bin_count; bin_number++){
          tot_pixel_val = p_pixels_in_frame[g_bands_binned_per_row_count*BINNINGFACTOR+bin_number];
        }
        send_channel->buffer[binned_frames_offset+binned_rows_offset+g_full_binns_per_row_count].value = (uint16_t)(tot_pixel_val/g_samples_last_bin_count);

        }
    }
    is_UnlockSeqBuf (camera, 1, p_raw_frame);
  }
}

void HSICamera::captureMeanBinning(){

  char* p_raw_frame;
  int imagesequence_id = 0;
  int last_pixel_in_row_offset = g_full_binns_per_row_count*BINNINGFACTOR;

  for(int image_number=0; image_number<g_frame_count; image_number++){

    int errorCode;
    do{
      errorCode = is_WaitForNextImage(camera, 2000, &(p_raw_frame), &imagesequence_id);

      if(errorCode!=IS_SUCCESS){
        is_UnlockSeqBuf (camera, 1, p_raw_frame);
        // printf("Something went wrong with the is_WaitForNextImage, error code: %d\n", errorCode);
      }
    }while(errorCode!=IS_SUCCESS);

    int sensor_columns_count = 1936;
    int image_pos_offset_first_row = image_y_offset_sensor * sensor_columns_count * 2 + image_x_offset_sensor * 2;
    int image_pos_offset_next_row = 0;

    uint16_t p_pixels_in_frame[g_sensor_rows_count*g_sensor_columns_count];
    for(int image_row=0; image_row<g_sensor_rows_count; image_row++){
      for(int image_pixel=0; image_pixel<g_sensor_columns_count; image_pixel++){
        p_pixels_in_frame[image_row*g_sensor_columns_count+image_pixel] = uint16_t(p_raw_frame[image_pos_offset_first_row+image_pos_offset_next_row+image_row*g_sensor_columns_count*2+image_pixel*2]) << 8
        | p_raw_frame[image_pos_offset_first_row+image_pos_offset_next_row+image_row*g_sensor_columns_count*2+image_pixel*2+1];
      }
      image_pos_offset_next_row += image_x_offset_sensor + image_x_offset_sensor + image_x_offset_sensor + image_x_offset_sensor; //TODO Fix odd image width number offset right sde of image
    }

    int binned_frames_offset = g_binned_pixels_per_frame_count * image_number;

    // Binn image
    #pragma omp parallel for num_threads(2)
    for(int sensor_row=0; sensor_row<g_sensor_rows_count; sensor_row++){
      int row_offset = sensor_row*g_sensor_columns_count;
      int binned_rows_offset = sensor_row*g_bands_binned_per_row_count;

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

        send_channel->buffer[binned_frames_offset+binned_rows_offset+bin_number].value = (uint16_t)(tot_pixel_val/BINNINGFACTOR);

        bin_offset += BINNINGFACTOR;
      }
      if(g_samples_last_bin_count>0){
        int tot_pixel_val = 0;
        for(int bin_number=0; bin_number<g_samples_last_bin_count; bin_number++){
          tot_pixel_val = p_pixels_in_frame[g_bands_binned_per_row_count*BINNINGFACTOR+bin_number];
        }
        send_channel->buffer[binned_frames_offset+binned_rows_offset+g_full_binns_per_row_count].value = (uint16_t)(tot_pixel_val/g_samples_last_bin_count);

        }
    }
    is_UnlockSeqBuf (camera, 1, p_raw_frame);
}
}

// Sends cube to FPGA and waits until it has sent compressed image back
void HSICamera::transferDMA(){

  cubedma_init_t cubedma_parameters = {
		.address = {
			.source      = (uint32_t)(SEND_PHYS_ADDR),
			.destination = (uint32_t)(RECIEVE_PHYS_ADDR)
		},
		.cube = {
			.n_planes  = 1,
			.c_offset  = 0,
			.planewise =  0,
			.blocks    = {
				.enabled = 0,
				.dims = { 0, 0, 0 }
			},
			.dims = {
				.width = 107,
				.height = 720,
				.depth = 500,
				.size_row = 77040
			}
		},
		.interrupt_enable = {
			{0, 0}, {0, 0}
		}
	};

  CubeDMADriver cdma;
  cdma.cubedma_Init(cubedma_parameters);

  // Clean cache
  unsigned long dummy; //This does nothing, parameter required by the kernel.
  ioctl(fd_send, 0, &dummy); // Clean cache

  cdma.cubedma_StartTransfer(S2MM);
	cdma.cubedma_StartTransfer(MM2S);

  // Wait until transfer and receive is done
  volatile uint32_t time;
	cubedma_error_t err = ERR_TIMEOUT;
	for (time = 0; time < TIMEOUT; time++) {
		if (cdma.cubedma_TransferDone(MM2S)) {
			err = SUCCESS;
			break;
		}
	}
  if (err != SUCCESS) {
		printf("ERROR: MM2S transfer timed out!\n\r");
	}

  // err = ERR_TIMEOUT;
	// for (time = 0; time < TIMEOUT; time++) {
	// 	if (cdma.cubedma_TransferDone(S2MM)) {
	// 		err = SUCCESS;
	// 		break;
	// 	}
	// }
  while(!cdma.cubedma_TransferDone(S2MM)){

  }
	if (err != SUCCESS) {
		printf("ERROR: S2MM transfer timed out!\n\r");
	}

  // Flush cache
  ioctl(fd_send, 1, &dummy); //TODO: Make enum for clean and flush command

  ///////////////Debug stuff
  // int nPrinted = 0;
  // uint32_t matches = 0;
  // uint32_t misses = 0;
  // printf("i:   src   dest\n\r");
  // for (uint32_t i = 0; i < CUBE_SIZE; i++){
  //   // if (source[i] == destin[i]) {
  //   if (send_channel->buffer[i].value == recieve_channel->buffer[i].value) {
  //     matches++;
  //   }
  //   else {
  //     if (nPrinted<20) {
  //       nPrinted++;
  //       // printf("%u: %4u %4u\n\r", i, source[i], destin[i]);
  //       printf("%u: %4u %4u\n\r", i, send_channel->buffer[i].value, recieve_channel->buffer[i].value);
  //     }
  //   }
  // }

	// if (matches != CUBE_SIZE) {
	// 	fprintf(stderr, "ERROR: Only %f%% of the data matches\n\r", \
	// 			(double)matches*100/CUBE_SIZE);
  //
	// }
	// else {
	// 	printf("Transfer success!\n\r");
	// }
  /////////////////////////////////
  printf("Transfer success!\n\r");
  writeRawDataToFile(recieve_channel, cdma.get_received_length());
}


// Wtites cube that is in DMA memory in specified format
void HSICamera::writeCubeToFile(){
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
          column_cube[band*g_sensor_rows_count+pixel] = send_channel->buffer[cube_row*g_binned_pixels_per_frame_count+g_bands_binned_per_row_count*(g_sensor_rows_count-1)-g_bands_binned_per_row_count*pixel+band].value;
        }
      }
      fwrite (column_cube, sizeof(uint16_t), g_sensor_rows_count*g_full_binns_per_row_count, p_file_cube);
    }
  }
  else if(cube_type==Bip){//BIP
    for(int cube_row=0; cube_row<g_cube_rows_count; cube_row++){
      for(int pixel=0; pixel<g_sensor_rows_count; pixel++){
        for(int band=0; band<g_full_binns_per_row_count; band++){
          column_cube[pixel*g_full_binns_per_row_count+band] = send_channel->buffer[cube_row*g_binned_pixels_per_frame_count+g_full_binns_per_row_count*(g_sensor_rows_count-1)-g_full_binns_per_row_count*pixel+band].value;
        }
      }
      fwrite (column_cube, sizeof(uint16_t), g_sensor_rows_count*g_full_binns_per_row_count, p_file_cube);
    }
  }
  else if(cube_type==Bsq){//BSQ
    for(int band=0; band<g_full_binns_per_row_count; band++){
      for(int rowSpatial=0; rowSpatial<g_frame_count; rowSpatial++){
        for(int pixel=0; pixel<g_cube_clumns_count; pixel++){
          column_cube[pixel] = send_channel->buffer[rowSpatial*g_binned_pixels_per_frame_count+g_bands_binned_per_row_count*(g_sensor_rows_count-1)-g_bands_binned_per_row_count*pixel+band].value;
        }
        fwrite (column_cube, sizeof(uint16_t), g_sensor_rows_count*g_full_binns_per_row_count, p_file_cube);
      }
    }
    fclose (p_file_cube);
  }
  else{
    fwrite (send_channel->buffer, sizeof(uint16_t), g_sensor_rows_count*g_full_binns_per_row_count*g_frame_count, p_file_cube);
    fclose (p_file_cube);
  }
}

void HSICamera::writeRawDataToFile(uint12_t* data, int count){
  //Name file
  struct timespec time_system;
  clock_gettime(CLOCK_REALTIME, &time_system);
  char timeSystemString[32];
  sprintf(timeSystemString, "%lli", (long long)time_system.tv_nsec);
  char file_path[64];
  strcpy(file_path, "/home/root/");
  strcat(file_path, timeSystemString);
  strcat(file_path, "Data.raw");

  //Write
  FILE * fp;
  fp = fopen (file_path,"wb");
  if (fp==NULL) {fputs ("File error\n",stderr); exit (1);}

  fwrite (data, sizeof(uint12_t), count, fp);//TODO g_bit_depth
  fclose (fp);

}

void HSICamera::writeRawDataToFile(uint8_t* data, int count){
  //Name file
  struct timespec time_system;
  clock_gettime(CLOCK_REALTIME, &time_system);
  char timeSystemString[32];
  sprintf(timeSystemString, "%lli", (long long)time_system.tv_nsec);
  char file_path[64];
  strcpy(file_path, "/home/root/");
  strcat(file_path, timeSystemString);
  strcat(file_path, "Data.raw");

  //Write
  FILE * fp;
  fp = fopen (file_path,"wb");
  if (fp==NULL) {fputs ("File error\n",stderr); exit (1);}
  fwrite (data, sizeof(uint8_t), count, fp);//TODO g_bit_depth
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

void HSICamera::bitonicMerge8(uint16_t* arr){

  uint16x4_t vec1_16x4, vec2_16x4;
  vec1_16x4 = vld1_u16(arr);
  vec2_16x4 = vld1_u16(arr+4);

  uint16x4_t max_vec = vmax_u16(vec1_16x4, vec2_16x4);
  uint16x4_t min_vec = vmin_u16(vec1_16x4, vec2_16x4);

  vec1_16x4 = vrev32_u16(max_vec);

  max_vec = vmax_u16(vec1_16x4, min_vec);
  min_vec = vmin_u16(vec1_16x4, min_vec);

  uint16x4x2_t shuffleTmp = vtrn_u16(min_vec, max_vec);

  max_vec = vmax_u16(shuffleTmp.val[0], shuffleTmp.val[1]);
  min_vec = vmin_u16(shuffleTmp.val[0], shuffleTmp.val[1]);

  uint16x4x2_t zipped_comp = vzip_u16(max_vec, min_vec);
  uint16x4_t reversed_zipped_vector = vrev64_u16(zipped_comp.val[1]);


  // L1
  max_vec = vmax_u16(reversed_zipped_vector, zipped_comp.val[0]);
  min_vec = vmin_u16(reversed_zipped_vector, zipped_comp.val[0]);

  shuffleTmp = vtrn_u16(min_vec, max_vec);
  uint16x4x2_t interleavedTmp = vzip_u16(shuffleTmp.val[0], shuffleTmp.val[1]);


  // L2
  max_vec = vmax_u16(interleavedTmp.val[0], interleavedTmp.val[1]);
  min_vec = vmin_u16(interleavedTmp.val[0], interleavedTmp.val[1]);

  uint16x4x2_t vec1_L3 = vtrn_u16(min_vec, max_vec);

  // L3
  max_vec = vmax_u16(vec1_L3.val[0], vec1_L3.val[1]);
  min_vec = vmin_u16(vec1_L3.val[0], vec1_L3.val[1]);

  uint16x4x2_t zippedResult = vzip_u16(min_vec, max_vec);

  vst1_u16(arr, zippedResult.val[0]);
}

void HSICamera::bitonicMerge6(uint16_t* arr){

  uint16_t tmp_arr[4] = {0, 0, arr[0], arr[1]};
  uint16x4_t vec1_16x4, vec2_16x4;
  vec1_16x4 = vld1_u16(tmp_arr);
  vec2_16x4 = vld1_u16(arr+2);

  uint16x4_t max_vec = vmax_u16(vec1_16x4, vec2_16x4);
  uint16x4_t min_vec = vmin_u16(vec1_16x4, vec2_16x4);

  vec1_16x4 = vrev32_u16(max_vec);

  max_vec = vmax_u16(vec1_16x4, min_vec);
  min_vec = vmin_u16(vec1_16x4, min_vec);

  uint16x4x2_t shuffleTmp = vtrn_u16(min_vec, max_vec);

  max_vec = vmax_u16(shuffleTmp.val[0], shuffleTmp.val[1]);
  min_vec = vmin_u16(shuffleTmp.val[0], shuffleTmp.val[1]);

  uint16x4x2_t zipped_comp = vzip_u16(max_vec, min_vec);
  uint16x4_t reversed_zipped_vector = vrev64_u16(zipped_comp.val[1]);


  // L1
  max_vec = vmax_u16(reversed_zipped_vector, zipped_comp.val[0]);
  min_vec = vmin_u16(reversed_zipped_vector, zipped_comp.val[0]);

  shuffleTmp = vtrn_u16(min_vec, max_vec);
  uint16x4x2_t interleavedTmp = vzip_u16(shuffleTmp.val[0], shuffleTmp.val[1]);


  // L2
  max_vec = vmax_u16(interleavedTmp.val[0], interleavedTmp.val[1]);
  min_vec = vmin_u16(interleavedTmp.val[0], interleavedTmp.val[1]);

  uint16x4x2_t vec1_L3 = vtrn_u16(min_vec, max_vec);

  // L3
  max_vec = vmax_u16(vec1_L3.val[0], vec1_L3.val[1]);
  min_vec = vmin_u16(vec1_L3.val[0], vec1_L3.val[1]);

  uint16x4x2_t zippedResult = vzip_u16(min_vec, max_vec);

  uint16_t tmp_arr5[4];
  vst1_u16(tmp_arr5, zippedResult.val[0]);
  *arr = tmp_arr5[2];
  *(arr+1) = tmp_arr5[3];
  vst1_u16(arr+2, zippedResult.val[1]);
}

void HSICamera::bitonicMerge4(uint16_t* arr){

  uint16x4_t vec1_16x4, vec2_16x4;

  vec1_16x4 = vld1_lane_u16(arr, vec1_16x4, 0);
  vec1_16x4 = vld1_lane_u16(arr+1, vec1_16x4, 1);
  vec2_16x4 = vld1_lane_u16(arr+2, vec2_16x4, 0);
  vec2_16x4 = vld1_lane_u16(arr+3, vec2_16x4, 1);

  uint16x4_t max_vec = vmax_u16(vec1_16x4, vec2_16x4);
  uint16x4_t min_vec = vmin_u16(vec1_16x4, vec2_16x4);

  uint16x4x2_t zippedResult = vzip_u16(min_vec, max_vec);

  max_vec = vmax_u16(zippedResult.val[1], zippedResult.val[0]);
  min_vec = vmin_u16(zippedResult.val[1], zippedResult.val[0]);

  uint16x4x2_t shuffleTmp = vtrn_u16(min_vec, max_vec);
  uint16x4x2_t interleavedTmp = vzip_u16(shuffleTmp.val[0], shuffleTmp.val[1]);


  // L2
  max_vec = vmax_u16(interleavedTmp.val[0], interleavedTmp.val[1]);
  min_vec = vmin_u16(interleavedTmp.val[0], interleavedTmp.val[1]);

  uint16x4x2_t result = vtrn_u16(min_vec, max_vec);

  vst1_u16(arr, result.val[0]);
}

void HSICamera::swap(uint16_t *xp, uint16_t *yp)
{
    uint16_t temp = *xp;
    *xp = *yp;
    *yp = temp;
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
