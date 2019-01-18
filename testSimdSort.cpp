#include <arm_neon.h>
#include<stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int bands = 1936;
int columns = bands;
int sensorColumns = bands;
int rows = 1216;
int sensorRows = rows;
int factorLastBands;
int nFullBinnsPerRow;
int nBandsBinned;
int binningFactor = 12;
int main(){
  // uint16_t rawImageP[16];
  // for(int i=4; i<16; i++){
  //   rawImageP[i] = i;
  // }
  // rawImageP[0] = rawImageP[1] = rawImageP[2] = rawImageP[3] = 0;
  factorLastBands = bands%binningFactor;
  nFullBinnsPerRow = bands/binningFactor;
  nBandsBinned = (bands + binningFactor - 1) / binningFactor;

  struct timeval  tv1, tv2, tv3;
  FILE * pFile = fopen ( "2Single.raw" , "rb" );
  if (pFile==NULL) {fputs ("File error\n",stderr); exit (1);}

  uint16_t buffer[2354176];

  size_t result = fread (buffer, 2, 2354176, pFile);
  if (result != 2354176) {fputs ("Reading error",stderr); exit (3);}

  uint16_t rawImageP[16] = {0,0,0,0,5,22,666,1,65,8884,0,16,5,0,5,999};

  // int imageSequenceID = 1;
  // for(int imageNumber=0; imageNumber<nSingleFrames; imageNumber++){

    gettimeofday(&tv1, NULL);
    /////Binning
    #pragma omp parallel for num_threads(2)
    for(int row=0; row<sensorRows; row++){
      int rowOffset = row*sensorColumns;
      int binnedIdxOffset = row*nBandsBinned;

      int binOffset = 0;
      for(int binnIterator=0; binnIterator<nFullBinnsPerRow; binnIterator++){

        int rowAndBinOffset = rowOffset+binOffset;

        // Load vectors
        uint16x8_t vec1_16x8, vec2_16x8;
        uint16x4_t vec1_16x4, vec2_16x4, vec3_16x4, vec4_16x4;

        vec1_16x4 = vld1_u16(rawImageP+rowAndBinOffset);
        vec2_16x4 = vld1_u16(rawImageP+rowAndBinOffset+4);
        vec3_16x4 = vld1_u16(rawImageP+rowAndBinOffset+8);
        vec4_16x4 = vld1_u16(rawImageP+rowAndBinOffset+12);

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

        // printf("Transposed\n");
        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }
        // printf("\n");

        // Bitonic merge 4x2
        // TODO Test: vrev64q_u16(uint16x8_t vec);
        // uint16x4_t interleavedVectorLow = vget_low_u16(interleavedVector.val[0]);
        // uint16x4_t interleavedVectorHigh = vget_high_u16(interleavedVector.val[0]);
        uint16x8_t reversed_vector = vrev64q_u16(interleavedVector.val[1]);


        // printf("Reversed\n");
        // vst1q_u16(rawImageP, interleavedVector.val[0]);
        // vst1q_u16(rawImageP+8, reversed_vector);
        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }
        // printf("\n");

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

        // vst1q_u16(rawImageP, min_vec);
        // vst1q_u16(rawImageP+8, max_vec);

        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }
        // printf("\n");
        //
        // vst1q_u16(rawImageP, vec1_L2);
        // vst1q_u16(rawImageP+8, vec2_L2);
        //
        // printf("Input L2\n");
        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }
        // printf("\n");
        // L2
        max_vec = vmaxq_u16(vec1_L2, vec2_L2);
        min_vec = vminq_u16(vec1_L2, vec2_L2);

        uint16x8x2_t vec1_L3 = vtrnq_u16(min_vec, max_vec);

        // Shuffle lines into right vectors
        // line1 = vget_high_u16(interleavedTmp.val[0]);
        // line3 = vget_low_u16(interleavedTmp.val[0]);
        // line2 = vget_high_u16(interleavedTmp.val[1]);
        // line4 = vget_low_u16(interleavedTmp.val[1]);
        // uint16x8_t vec1_L3 = vcombine_u16(line1, line2);
        // uint16x8_t vec2_L3 = vcombine_u16(line3, line4);

        // vst1q_u16(rawImageP, min_vec);
        // vst1q_u16(rawImageP+8, max_vec);

        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }
        // printf("\n");
        //
        // vst1q_u16(rawImageP, vec1_L3.val[0]);
        // vst1q_u16(rawImageP+8, vec1_L3.val[1]);
        //
        // printf("Input L3\n");
        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }
        // printf("\n");

        // L3
        max_vec = vmaxq_u16(vec1_L3.val[0], vec1_L3.val[1]);
        min_vec = vminq_u16(vec1_L3.val[0], vec1_L3.val[1]);

        uint16x8x2_t zippedResult = vzipq_u16(min_vec, max_vec);
        // vst1q_u16(rawImageP, zippedResult.val[0]);
        // vst1q_u16(rawImageP+8, zippedResult.val[1]);

        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }
        // printf("\n");
        // Bitonic merge 8x2
        reversed_vector = vrev64q_u16(zippedResult.val[0]);
        uint16x4_t reversed_vector_high = vget_high_u16(reversed_vector);
        uint16x4_t reversed_vector_low = vget_low_u16(reversed_vector);
        reversed_vector = vcombine_u16(reversed_vector_high, reversed_vector_low);

        max_vec = vmaxq_u16(reversed_vector, zippedResult.val[1]);
        min_vec = vminq_u16(reversed_vector, zippedResult.val[1]);

        // vst1q_u16(rawImageP, min_vec);
        // vst1q_u16(rawImageP+8, max_vec);

        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }
        // printf("\n");

        // Shuffle lines into right vectors
        uint16x4_t max_vec_high = vget_high_u16(max_vec);
        uint16x4_t max_vec_low = vget_low_u16(max_vec);
        uint16x4_t min_vec_high = vget_high_u16(min_vec);
        uint16x4_t min_vec_low = vget_low_u16(min_vec);
        uint16x8_t shuffled_vec1 = vcombine_u16(min_vec_low, max_vec_low);
        uint16x8_t shuffled_vec2 = vcombine_u16(min_vec_high, max_vec_high);

        uint16x8_t input_bitonic4x2_max = vmaxq_u16(shuffled_vec1, shuffled_vec2);
        uint16x8_t input_bitonic4x2_min = vminq_u16(shuffled_vec1, shuffled_vec2);


        // printf("Bitonic merge 8x2\n");

        // vst1q_u16(rawImageP, reversed_vector);
        // vst1q_u16(rawImageP+8, zippedResult.val[1]);

        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }
        // printf("\n");

        // vst1q_u16(rawImageP, shuffled_vec1);
        // vst1q_u16(rawImageP+8, shuffled_vec2);

        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }
        // printf("\n");
        //
        // vst1q_u16(rawImageP, input_bitonic4x2_min);
        // vst1q_u16(rawImageP+8, input_bitonic4x2_max);
        //
        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }


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

        // vst1q_u16(rawImageP, min_vec);
        // vst1q_u16(rawImageP+8, max_vec);

        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }
        // printf("\n");

        // vst1q_u16(rawImageP, vec1_L2);
        // vst1q_u16(rawImageP+8, vec2_L2);

        // printf("Input L2\n");
        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }
        // printf("\n");
        // L2
        max_vec = vmaxq_u16(vec1_L2, vec2_L2);
        min_vec = vminq_u16(vec1_L2, vec2_L2);

        vec1_L3 = vtrnq_u16(min_vec, max_vec);

        // Shuffle lines into right vectors
        // line1 = vget_high_u16(interleavedTmp.val[0]);
        // line3 = vget_low_u16(interleavedTmp.val[0]);
        // line2 = vget_high_u16(interleavedTmp.val[1]);
        // line4 = vget_low_u16(interleavedTmp.val[1]);
        // uint16x8_t vec1_L3 = vcombine_u16(line1, line2);
        // uint16x8_t vec2_L3 = vcombine_u16(line3, line4);

        // vst1q_u16(rawImageP, min_vec);
        // vst1q_u16(rawImageP+8, max_vec);

        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }
        // printf("\n");

        // vst1q_u16(rawImageP, vec1_L3.val[0]);
        // vst1q_u16(rawImageP+8, vec1_L3.val[1]);

        // printf("Input L3\n");
        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }
        // printf("\n");

        // L3
        max_vec = vmaxq_u16(vec1_L3.val[0], vec1_L3.val[1]);
        min_vec = vminq_u16(vec1_L3.val[0], vec1_L3.val[1]);

        zippedResult = vzipq_u16(min_vec, max_vec);
        vst1q_u16(rawImageP+rowAndBinOffset, zippedResult.val[0]);
        vst1q_u16(rawImageP+rowAndBinOffset+8, zippedResult.val[1]);

        // for(int i=0; i<16; i++){
        //   printf("%u ", rawImageP[i]);
        //   if(i==3||i==7||i==11||i==15){
        //     printf("\n");
        //   }
        // }
        // printf("\n");

        binOffset += binningFactor;
      }
      if(factorLastBands>0){

      }

    }
    gettimeofday(&tv2, NULL);
    printf("%f\n", (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec));

  // }


  // printf("Initial\n");
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");

  // Load vectors
  // uint16x8_t vec1_16x8, vec2_16x8;
  // uint16x4_t vec1_16x4, vec2_16x4, vec3_16x4, vec4_16x4;
  //
  // vec1_16x4 = vld1_u16(rawImageP);
  // vec2_16x4 = vld1_u16(rawImageP+4);
  // vec3_16x4 = vld1_u16(rawImageP+8);
  // vec4_16x4 = vld1_u16(rawImageP+12);
  //
  // // Sorting across registers
  // uint16x4_t max_vec_sort = vmax_u16(vec4_16x4, vec3_16x4);
  // uint16x4_t min_vec_sort = vmin_u16(vec4_16x4, vec3_16x4);
  //
  // vec3_16x4 = vmin_u16(max_vec_sort, vec2_16x4);
  // vec4_16x4 = vmax_u16(max_vec_sort, vec2_16x4);
  //
  // vec2_16x4 = vmin_u16(vec3_16x4, min_vec_sort);
  // vec3_16x4 = vmax_u16(vec3_16x4, min_vec_sort);
  //
  // // Transpose vectors
  // vec1_16x8 = vcombine_u16(vec1_16x4, vec2_16x4);
  // vec2_16x8 = vcombine_u16(vec3_16x4, vec4_16x4);
  //
  // uint16x8x2_t interleavedVector = vzipq_u16(vec1_16x8, vec2_16x8);
  // interleavedVector = vzipq_u16(interleavedVector.val[0], interleavedVector.val[1]);
  //
  // vst1q_u16(rawImageP, interleavedVector.val[0]);
  // vst1q_u16(rawImageP+8, interleavedVector.val[1]);
  // printf("Transposed\n");
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");
  //
  // // Bitonic merge 4x2
  // // TODO Test: vrev64q_u16(uint16x8_t vec);
  // // uint16x4_t interleavedVectorLow = vget_low_u16(interleavedVector.val[0]);
  // // uint16x4_t interleavedVectorHigh = vget_high_u16(interleavedVector.val[0]);
  // uint16x8_t reversed_vector = vrev64q_u16(interleavedVector.val[1]);
  //
  //
  // printf("Reversed\n");
  // vst1q_u16(rawImageP, interleavedVector.val[0]);
  // vst1q_u16(rawImageP+8, reversed_vector);
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");
  //
  // // L1
  // uint16x8_t max_vec = vmaxq_u16(reversed_vector, interleavedVector.val[0]);
  // uint16x8_t min_vec = vminq_u16(reversed_vector, interleavedVector.val[0]);
  //
  // uint16x8x2_t shuffleTmp = vtrnq_u16(min_vec, max_vec);
  // uint16x8x2_t interleavedTmp = vzipq_u16(shuffleTmp.val[0], shuffleTmp.val[1]);
  //
  // // Shuffle lines into right vectors
  // uint16x4_t line1 = vget_high_u16(interleavedTmp.val[0]);
  // uint16x4_t line3 = vget_low_u16(interleavedTmp.val[0]);
  // uint16x4_t line2 = vget_high_u16(interleavedTmp.val[1]);
  // uint16x4_t line4 = vget_low_u16(interleavedTmp.val[1]);
  // uint16x8_t  vec1_L2 = vcombine_u16(line1, line2);
  // uint16x8_t  vec2_L2 = vcombine_u16(line3, line4);
  //
  // vst1q_u16(rawImageP, min_vec);
  // vst1q_u16(rawImageP+8, max_vec);
  //
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");
  //
  // vst1q_u16(rawImageP, vec1_L2);
  // vst1q_u16(rawImageP+8, vec2_L2);
  //
  // printf("Input L2\n");
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");
  // // L2
  // max_vec = vmaxq_u16(vec1_L2, vec2_L2);
  // min_vec = vminq_u16(vec1_L2, vec2_L2);
  //
  // uint16x8x2_t vec1_L3 = vtrnq_u16(min_vec, max_vec);
  //
  // // Shuffle lines into right vectors
  // // line1 = vget_high_u16(interleavedTmp.val[0]);
  // // line3 = vget_low_u16(interleavedTmp.val[0]);
  // // line2 = vget_high_u16(interleavedTmp.val[1]);
  // // line4 = vget_low_u16(interleavedTmp.val[1]);
  // // uint16x8_t vec1_L3 = vcombine_u16(line1, line2);
  // // uint16x8_t vec2_L3 = vcombine_u16(line3, line4);
  //
  // vst1q_u16(rawImageP, min_vec);
  // vst1q_u16(rawImageP+8, max_vec);
  //
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");
  //
  // vst1q_u16(rawImageP, vec1_L3.val[0]);
  // vst1q_u16(rawImageP+8, vec1_L3.val[1]);
  //
  // printf("Input L3\n");
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");
  //
  // // L3
  // max_vec = vmaxq_u16(vec1_L3.val[0], vec1_L3.val[1]);
  // min_vec = vminq_u16(vec1_L3.val[0], vec1_L3.val[1]);
  //
  // uint16x8x2_t zippedResult = vzipq_u16(min_vec, max_vec);
  // vst1q_u16(rawImageP, zippedResult.val[0]);
  // vst1q_u16(rawImageP+8, zippedResult.val[1]);
  //
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");
  // // Bitonic merge 8x2
  // reversed_vector = vrev64q_u16(zippedResult.val[0]);
  // uint16x4_t reversed_vector_high = vget_high_u16(reversed_vector);
  // uint16x4_t reversed_vector_low = vget_low_u16(reversed_vector);
  // reversed_vector = vcombine_u16(reversed_vector_high, reversed_vector_low);
  //
  // max_vec = vmaxq_u16(reversed_vector, zippedResult.val[1]);
  // min_vec = vminq_u16(reversed_vector, zippedResult.val[1]);
  //
  // vst1q_u16(rawImageP, min_vec);
  // vst1q_u16(rawImageP+8, max_vec);
  //
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");
  //
  // // Shuffle lines into right vectors
  // uint16x4_t max_vec_high = vget_high_u16(max_vec);
  // uint16x4_t max_vec_low = vget_low_u16(max_vec);
  // uint16x4_t min_vec_high = vget_high_u16(min_vec);
  // uint16x4_t min_vec_low = vget_low_u16(min_vec);
  // uint16x8_t shuffled_vec1 = vcombine_u16(min_vec_low, max_vec_low);
  // uint16x8_t shuffled_vec2 = vcombine_u16(min_vec_high, max_vec_high);
  //
  // uint16x8_t input_bitonic4x2_max = vmaxq_u16(shuffled_vec1, shuffled_vec2);
  // uint16x8_t input_bitonic4x2_min = vminq_u16(shuffled_vec1, shuffled_vec2);
  //
  //
  // printf("Bitonic merge 8x2\n");
  //
  // vst1q_u16(rawImageP, reversed_vector);
  // vst1q_u16(rawImageP+8, zippedResult.val[1]);
  //
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");
  //
  // vst1q_u16(rawImageP, shuffled_vec1);
  // vst1q_u16(rawImageP+8, shuffled_vec2);
  //
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");
  //
  // vst1q_u16(rawImageP, input_bitonic4x2_min);
  // vst1q_u16(rawImageP+8, input_bitonic4x2_max);
  //
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  //
  //
  // // Bitonic merge 4x2
  // // L1
  // max_vec = vmaxq_u16(input_bitonic4x2_min, input_bitonic4x2_max);
  // min_vec = vminq_u16(input_bitonic4x2_min, input_bitonic4x2_max);
  //
  // shuffleTmp = vtrnq_u16(min_vec, max_vec);
  // interleavedTmp = vzipq_u16(shuffleTmp.val[0], shuffleTmp.val[1]);
  //
  // // Shuffle lines into right vectors
  // line1 = vget_high_u16(interleavedTmp.val[0]);
  // line3 = vget_low_u16(interleavedTmp.val[0]);
  // line2 = vget_high_u16(interleavedTmp.val[1]);
  // line4 = vget_low_u16(interleavedTmp.val[1]);
  // vec1_L2 = vcombine_u16(line1, line2);
  // vec2_L2 = vcombine_u16(line3, line4);
  //
  // vst1q_u16(rawImageP, min_vec);
  // vst1q_u16(rawImageP+8, max_vec);
  //
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");
  //
  // vst1q_u16(rawImageP, vec1_L2);
  // vst1q_u16(rawImageP+8, vec2_L2);
  //
  // printf("Input L2\n");
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");
  // // L2
  // max_vec = vmaxq_u16(vec1_L2, vec2_L2);
  // min_vec = vminq_u16(vec1_L2, vec2_L2);
  //
  // vec1_L3 = vtrnq_u16(min_vec, max_vec);
  //
  // // Shuffle lines into right vectors
  // // line1 = vget_high_u16(interleavedTmp.val[0]);
  // // line3 = vget_low_u16(interleavedTmp.val[0]);
  // // line2 = vget_high_u16(interleavedTmp.val[1]);
  // // line4 = vget_low_u16(interleavedTmp.val[1]);
  // // uint16x8_t vec1_L3 = vcombine_u16(line1, line2);
  // // uint16x8_t vec2_L3 = vcombine_u16(line3, line4);
  //
  // vst1q_u16(rawImageP, min_vec);
  // vst1q_u16(rawImageP+8, max_vec);
  //
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");
  //
  // vst1q_u16(rawImageP, vec1_L3.val[0]);
  // vst1q_u16(rawImageP+8, vec1_L3.val[1]);
  //
  // printf("Input L3\n");
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");
  //
  // // L3
  // max_vec = vmaxq_u16(vec1_L3.val[0], vec1_L3.val[1]);
  // min_vec = vminq_u16(vec1_L3.val[0], vec1_L3.val[1]);
  //
  // zippedResult = vzipq_u16(min_vec, max_vec);
  // vst1q_u16(rawImageP, zippedResult.val[0]);
  // vst1q_u16(rawImageP+8, zippedResult.val[1]);
  //
  // for(int i=0; i<16; i++){
  //   printf("%u ", rawImageP[i]);
  //   if(i==3||i==7||i==11||i==15){
  //     printf("\n");
  //   }
  // }
  // printf("\n");

}


























// #include <arm_neon.h>
// #include<stdio.h>
//
// int main(){
//   uint16_t rawImageP[16];
//   for(int i=4; i<16; i++){
//     rawImageP[i] = i;
//   }
//   rawImageP[0] = rawImageP[1] = rawImageP[2] = rawImageP[3] = 0;
//
//   printf("Initial\n");
//   for(int i=0; i<16; i++){
//     printf("%u ", rawImageP[i]);
//     if(i==3||i==7||i==11||i==15){
//       printf("\n");
//     }
//   }
//   printf("\n");
//   // Load vectors
//   uint16x8_t vec1_16x8, vec2_16x8;
//   uint16x4_t vec1_16x4, vec2_16x4, vec3_16x4, vec4_16x4;
//
//   vec1_16x4 = vld1_u16(rawImageP);
//   vec2_16x4 = vld1_u16(rawImageP+4);
//   vec3_16x4 = vld1_u16(rawImageP+8);
//   vec4_16x4 = vld1_u16(rawImageP+12);
//
//   // Sorting across registers
//   uint16x4_t max_vec = vmax_u16(vec4_16x4, vec3_16x4);
//   uint16x4_t min_vec = vmin_u16(vec4_16x4, vec3_16x4);
//
//   vec3_16x4 = vmin_u16(max_vec, vec2_16x4);
//   vec4_16x4 = vmax_u16(max_vec, vec2_16x4);
//
//   vec2_16x4 = vmin_u16(vec3_16x4, min_vec);
//   vec3_16x4 = vmax_u16(vec3_16x4, min_vec);
//
//   // Transpose vectors
//   vec1_16x8 = vcombine_u16(vec1_16x4, vec2_16x4);
//   vec2_16x8 = vcombine_u16(vec3_16x4, vec4_16x4);
//
//   uint16x8x2_t interleavedVector = vzipq_u16(vec1_16x8, vec2_16x8);
//   interleavedVector = vzipq_u16(interleavedVector.val[0], interleavedVector.val[1]);
//
//   vst1q_u16(rawImageP, interleavedVector.val[0]);
//   vst1q_u16(rawImageP+8, interleavedVector.val[1]);
//   printf("Transposed\n");
//   for(int i=0; i<16; i++){
//     printf("%u ", rawImageP[i]);
//     if(i==3||i==7||i==11||i==15){
//       printf("\n");
//     }
//   }
//   printf("\n");
//
//   // Bitonic merge 4x4
//   // TODO Test: vrev64q_u16(uint16x8_t vec);
//   uint16x4_t interleavedVectorLow = vget_low_u16(interleavedVector.val[0]);
//   uint16x4_t interleavedVectorHigh = vget_high_u16(interleavedVector.val[0]);
//   uint16x4_t reversed_vector = vrev64_u16(interleavedVectorLow);
//
//   uint16x4_t interleavedVectorLow = vget_low_u16(interleavedVector.val[0]);
//   uint16x4_t interleavedVectorHigh = vget_high_u16(interleavedVector.val[0]);
//   uint16x4_t reversed_vector = vrev64_u16(interleavedVectorLow);
//
//   printf("Reversed\n");
//   vst1q_u16(rawImageP, reversed_vector);
//   vst1q_u16(rawImageP+4, interleavedVectorHigh);
//   for(int i=0; i<16; i++){
//     printf("%u ", rawImageP[i]);
//     if(i==3||i==7||i==11||i==15){
//       printf("\n");
//     }
//   }
//   printf("\n");
//
//   // L1
//   max_vec = vmax_u16(reversed_vector, interleavedVectorHigh);
//   min_vec = vmin_u16(reversed_vector, interleavedVectorHigh);
//
//   uint16x4x2_t shuffleTmp = vtrn_u16(min_vec, max_vec);
//   uint16x4x2_t vec1_L2 = vzip_u16(shuffleTmp.val[0], shuffleTmp.val[1]);
//
//   vst1q_u16(rawImageP, min_vec);
//   vst1q_u16(rawImageP+4, max_vec);
//   vst1q_u16(rawImageP+8, interleavedVector.val[1]);
//
//   for(int i=0; i<16; i++){
//     printf("%u ", rawImageP[i]);
//     if(i==3||i==7||i==11||i==15){
//       printf("\n");
//     }
//   }
//   printf("\n");
//
//   vst1q_u16(rawImageP, vec1_L2.val[0]);
//   vst1q_u16(rawImageP+4, vec1_L2.val[1]);
//   vst1q_u16(rawImageP+8, interleavedVector.val[1]);
//
//   printf("Input L2\n");
//   for(int i=0; i<16; i++){
//     printf("%u ", rawImageP[i]);
//     if(i==3||i==7||i==11||i==15){
//       printf("\n");
//     }
//   }
//   printf("\n");
//   // L2
//   max_vec = vmax_u16(vec1_L2.val[0], vec1_L2.val[1]);
//   min_vec = vmin_u16(vec1_L2.val[0], vec1_L2.val[1]);
//
//   uint16x4x2_t vec1_L3 = vtrn_u16(min_vec, max_vec);
//
//   vst1q_u16(rawImageP, min_vec);
//   vst1q_u16(rawImageP+4, max_vec);
//   vst1q_u16(rawImageP+8, interleavedVector.val[1]);
//
//   for(int i=0; i<16; i++){
//     printf("%u ", rawImageP[i]);
//     if(i==3||i==7||i==11||i==15){
//       printf("\n");
//     }
//   }
//   printf("\n");
//
//   vst1q_u16(rawImageP, vec1_L3.val[0]);
//   vst1q_u16(rawImageP+4, vec1_L3.val[1]);
//   vst1q_u16(rawImageP+8, interleavedVector.val[1]);
//
//   printf("Input L3\n");
//   for(int i=0; i<16; i++){
//     printf("%u ", rawImageP[i]);
//     if(i==3||i==7||i==11||i==15){
//       printf("\n");
//     }
//   }
//   printf("\n");
//
//   // L3
//   max_vec = vmax_u16(vec1_L3.val[0], vec1_L3.val[1]);
//   min_vec = vmin_u16(vec1_L3.val[0], vec1_L3.val[1]);
//
//   vec1_L3 = vzip_u16(min_vec, max_vec);
//   vst1q_u16(rawImageP, vec1_L3.val[0]);
//   vst1q_u16(rawImageP+4, vec1_L3.val[1]);
//   vst1q_u16(rawImageP+8, interleavedVector.val[1]);
//
//   for(int i=0; i<16; i++){
//     printf("%u ", rawImageP[i]);
//     if(i==3||i==7||i==11||i==15){
//       printf("\n");
//     }
//   }
// }
