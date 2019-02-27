#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/mman.h>


static volatile uint32_t *MM2S_control_register;
static volatile uint32_t *MM2S_status_register;
static volatile uint32_t *MM2S_base_adress_register;
static volatile uint32_t *MM2S_dimension_register1;
static volatile uint32_t *MM2S_dimension_register2;
static volatile uint32_t *MM2S_row_size_register;

static const uint32_t cubeDMA_base_addr = 0x0000;

static const uint32_t MM2S_control_register_addr = 0x00;
static const uint32_t MM2S_status_register_addr = 0x04;
static const uint32_t MM2S_base_adress_register_addr = 0x08;
static const uint32_t MM2S_dimension_register1_addr = 0x0c;
static const uint32_t MM2S_dimension_register2_addr = 0x10;
static const uint32_t MM2S_row_size_register_addr = 0x14;

uint32_t *deviceMem;

void map_cubeDMA(void)
{
 int fd;

if ((fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0)

if (fd < 0) {
 perror("/dev/mem");
 exit(-1);
 }

// if ((MM2S_control_register = mmap(0, 4, PROT_READ|PROT_WRITE, MAP_SHARED, fd, MM2S_control_register_addr))
//  == MAP_FAILED) {
//  perror("MM2S_control_register");
//  exit(-1);
//  }

 // MM2S_control_register = (uint32_t volatile *) 0x0;

// printf("%u\n", *MM2S_control_register);
// printf("%p\n", MM2S_control_register);
printf("Pagesize: %d\n", getpagesize());

if ((deviceMem = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, MM2S_control_register_addr))
 == MAP_FAILED) {
 perror("MM2S_status_register");
 exit(-1);
 }

// if ((MM2S_base_adress_register = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, MM2S_base_adress_register_addr))
//  == MAP_FAILED) {
//  perror("MM2S_base_adress_register");
//  exit(-1);
//  }
//
//  if ((MM2S_dimension_register1 = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, MM2S_dimension_register1_addr))
//   == MAP_FAILED) {
//   perror("MM2S_dimension_register1");
//   exit(-1);
//   }
//
//   if ((MM2S_dimension_register2 = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, MM2S_dimension_register2_addr))
//    == MAP_FAILED) {
//    perror("MM2S_dimension_register2");
//    exit(-1);
//    }
//
//    if ((MM2S_row_size_register = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, MM2S_row_size_register_addr))
//     == MAP_FAILED) {
//     perror("MM2S_row_size_register");
//     exit(-1);
//     }
}


int main()
{

  map_cubeDMA();

  uint16_t image[64];
  for(int i=0; i<64; i++){
    image[i] = i;
  }

  // Cube address
  deviceMem[MM2S_base_adress_register_addr] = (uint32_t)&image[0];
  // Cube dimension
  deviceMem[MM2S_dimension_register1_addr] = (0x00 & 0x08) | (0x00 & 0x08)<<12 | (0x00 & 0x08)<<24;
  // Block dimension
  deviceMem[MM2S_dimension_register2_addr] = (0x00 & 0x00) | (0x00 & 0x00)<<4 | (0x00 & 0x00)<<12;
  // Row size
  deviceMem[MM2S_row_size_register_addr] = 0x04;
  // Control
  deviceMem[MM2S_control_register_addr] = 0x0000;


  printf("deviceMem[MM2S_status_register_addr] = %u\n\n", deviceMem[MM2S_status_register_addr]);
  // Start
  deviceMem[MM2S_control_register_addr] = 0x0001;
  do {
    printf("deviceMem[MM2S_status_register_addr] = %u\n", deviceMem[MM2S_status_register_addr]);
    
  }while (deviceMem[MM2S_status_register_addr] & 1);
  printf("\n");
  printf("deviceMem[MM2S_status_register_addr] = %u\n", deviceMem[MM2S_status_register_addr]);



  return 0;
}
