#include <arm_neon.h>
#include<stdio.h>

int main(){
  uint16_t rawImageP[16];
  for(int i=4; i<16; i++){
    rawImageP[i] = i;
  }
  rawImageP[0] = rawImageP[1] = rawImageP[2] = rawImageP[3] = 0;

  printf("Initial\n");
  for(int i=0; i<16; i++){
    printf("%u ", rawImageP[i]);
    if(i==3||i==7||i==11||i==15){
      printf("\n");
    }
  }
  printf("\n");
  // Load vectors
  uint16x8_t vec1_16x8, vec2_16x8;
  uint16x4_t vec1_16x4, vec2_16x4, vec3_16x4, vec4_16x4;

  vec1_16x4 = vld1_u16(rawImageP);
  vec2_16x4 = vld1_u16(rawImageP+4);
  vec3_16x4 = vld1_u16(rawImageP+8);
  vec4_16x4 = vld1_u16(rawImageP+12);

  // Sorting across registers
  uint16x4_t max_vec = vmax_u16(vec4_16x4, vec3_16x4);
  uint16x4_t min_vec = vmin_u16(vec4_16x4, vec3_16x4);

  vec3_16x4 = vmin_u16(max_vec, vec2_16x4);
  vec4_16x4 = vmax_u16(max_vec, vec2_16x4);

  vec2_16x4 = vmin_u16(vec3_16x4, min_vec);
  vec3_16x4 = vmax_u16(vec3_16x4, min_vec);

  // Transpose vectors
  vec1_16x8 = vcombine_u16(vec1_16x4, vec2_16x4);
  vec2_16x8 = vcombine_u16(vec3_16x4, vec4_16x4);

  uint16x8x2_t interleavedVector = vzipq_u16(vec1_16x8, vec2_16x8);
  interleavedVector = vzipq_u16(interleavedVector.val[0], interleavedVector.val[1]);

  vst1q_u16(rawImageP, interleavedVector.val[0]);
  vst1q_u16(rawImageP+8, interleavedVector.val[1]);
  printf("Transposed\n");
  for(int i=0; i<16; i++){
    printf("%u ", rawImageP[i]);
    if(i==3||i==7||i==11||i==15){
      printf("\n");
    }
  }
  printf("\n");

  // Bitonic merge
  uint16x4_t interleavedVectorLow = vget_low_u16(interleavedVector.val[0]);
  uint16x4_t interleavedVectorHigh = vget_high_u16(interleavedVector.val[0]);
  uint16x4_t reversed_vector = vrev64_u16(interleavedVectorLow);

  printf("Reversed\n");
  vst1_u16(rawImageP, reversed_vector);
  vst1_u16(rawImageP+4, interleavedVectorHigh);
  for(int i=0; i<16; i++){
    printf("%u ", rawImageP[i]);
    if(i==3||i==7||i==11||i==15){
      printf("\n");
    }
  }
  printf("\n");

  // L1
  max_vec = vmax_u16(reversed_vector, interleavedVectorHigh);
  min_vec = vmin_u16(reversed_vector, interleavedVectorHigh);

  uint16x4x2_t shuffleTmp = vtrn_u16(min_vec, max_vec);
  uint16x4x2_t vec1_L2 = vzip_u16(shuffleTmp.val[0], shuffleTmp.val[1]);

  vst1_u16(rawImageP, min_vec);
  vst1_u16(rawImageP+4, max_vec);
  vst1q_u16(rawImageP+8, interleavedVector.val[1]);

  for(int i=0; i<16; i++){
    printf("%u ", rawImageP[i]);
    if(i==3||i==7||i==11||i==15){
      printf("\n");
    }
  }
  printf("\n");

  vst1_u16(rawImageP, vec1_L2.val[0]);
  vst1_u16(rawImageP+4, vec1_L2.val[1]);
  vst1q_u16(rawImageP+8, interleavedVector.val[1]);

  printf("Input L2\n");
  for(int i=0; i<16; i++){
    printf("%u ", rawImageP[i]);
    if(i==3||i==7||i==11||i==15){
      printf("\n");
    }
  }
  printf("\n");
  // L2
  max_vec = vmax_u16(vec1_L2.val[0], vec1_L2.val[1]);
  min_vec = vmin_u16(vec1_L2.val[0], vec1_L2.val[1]);

  uint16x4x2_t vec1_L3 = vtrn_u16(min_vec, max_vec);

  vst1_u16(rawImageP, min_vec);
  vst1_u16(rawImageP+4, max_vec);
  vst1q_u16(rawImageP+8, interleavedVector.val[1]);

  for(int i=0; i<16; i++){
    printf("%u ", rawImageP[i]);
    if(i==3||i==7||i==11||i==15){
      printf("\n");
    }
  }
  printf("\n");

  vst1_u16(rawImageP, vec1_L3.val[0]);
  vst1_u16(rawImageP+4, vec1_L3.val[1]);
  vst1q_u16(rawImageP+8, interleavedVector.val[1]);

  printf("Input L3\n");
  for(int i=0; i<16; i++){
    printf("%u ", rawImageP[i]);
    if(i==3||i==7||i==11||i==15){
      printf("\n");
    }
  }
  printf("\n");

  // L3
  max_vec = vmax_u16(vec1_L3.val[0], vec1_L3.val[1]);
  min_vec = vmin_u16(vec1_L3.val[0], vec1_L3.val[1]);

  vec1_L3 = vzip_u16(min_vec, max_vec);
  vst1_u16(rawImageP, vec1_L3.val[0]);
  vst1_u16(rawImageP+4, vec1_L3.val[1]);
  vst1q_u16(rawImageP+8, interleavedVector.val[1]);

  for(int i=0; i<16; i++){
    printf("%u ", rawImageP[i]);
    if(i==3||i==7||i==11||i==15){
      printf("\n");
    }
  }
}
