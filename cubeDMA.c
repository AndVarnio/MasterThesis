#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/mman.h>

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')


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
      puts("");
  }

static volatile uint32_t *MM2S_control_register;
static volatile uint32_t *MM2S_status_register;
static volatile uint32_t *MM2S_base_adress_register;
static volatile uint32_t *MM2S_dimension_register1;
static volatile uint32_t *MM2S_dimension_register2;
static volatile uint32_t *MM2S_row_size_register;

static const uint32_t cubeDMA_base_addr = 0x40000000;

// static const uint32_t MM2S_control_register_addr = 0x00;
// static const uint32_t MM2S_status_register_addr = 0x04;
// static const uint32_t MM2S_base_adress_register_addr = 0x08;
// static const uint32_t MM2S_dimension_register1_addr = 0x0c;
// static const uint32_t MM2S_dimension_register2_addr = 0x10;
// static const uint32_t MM2S_row_size_register_addr = 0x14;

static const int MM2S_control_register_addr = 0;
static const int MM2S_status_register_addr = 1;
static const int MM2S_base_adress_register_addr = 2;
static const int MM2S_dimension_register1_addr = 3;
static const int MM2S_dimension_register2_addr = 4;
static const int MM2S_row_size_register_addr = 5;

uint32_t *deviceMem;

const int IMAGESIZE = 1920*1080*2*10;
uint16_t* image = new uint16_t[IMAGESIZE];

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

if ((deviceMem = static_cast<uint32_t*>(mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, cubeDMA_base_addr)))
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

  printf("Filling table\n");
  for(int i=0; i<IMAGESIZE; i++){
    image[i] = i;
  }
  printf("Filled table\n");


  for(int i=0; i<10; i++){
    printf("deviceMem[%d] = %u\n", i, deviceMem[i]);
  }
  printf("\n");
  // Cube address
  deviceMem[MM2S_base_adress_register_addr] = (uint32_t)&image[0];
  // Cube dimension
  deviceMem[MM2S_dimension_register1_addr] = (0x0F00) | (0x0870)<<12 | (0x000A)<<24;
  // Block dimension
  deviceMem[MM2S_dimension_register2_addr] = (0x00 & 0x00) | (0x00 & 0x00)<<4 | (0x00 & 0x00)<<12;
  // Row size
  deviceMem[MM2S_row_size_register_addr] = 0x0780;
  // Control
  for(int i=0; i<10; i++){
    printf("deviceMem[%d] = %u\n", i, deviceMem[i]);
  }
  printf("\n");
  deviceMem[MM2S_control_register_addr] = 0x0000;

  for(int i=0; i<10; i++){
    printf("deviceMem[%d] = %u\n", i, deviceMem[i]);
  }
  printf("\n");
  // printf("deviceMem[MM2S_status_register_addr] = %u\n", deviceMem[MM2S_status_register_addr]);
  printf("Status register: ");
  printBits(sizeof(deviceMem[MM2S_status_register_addr]), &deviceMem[MM2S_status_register_addr]);
  printf("Start transfer\n");
  // Start
  deviceMem[MM2S_control_register_addr] = 0x0001;
  do {
    // printf("deviceMem[MM2S_status_register_addr] = %u\n", deviceMem[MM2S_status_register_addr]);
    printf("Status register: ");
    printBits(sizeof(deviceMem[MM2S_status_register_addr]), &deviceMem[MM2S_status_register_addr]);
    usleep(2000000);
    // printf("\n");
  }while (!(deviceMem[MM2S_status_register_addr]&1));
  printf("\n");

  for(int i=0; i<10; i++){
    printf("deviceMem[%d] = %u\n", i, deviceMem[i]);
  }

  return 0;
}
