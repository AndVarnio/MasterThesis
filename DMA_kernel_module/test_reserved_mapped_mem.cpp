#include "hello_world_kernel_module.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/mman.h>
#include<errno.h>

struct dma_proxy_channel_interface *transmitt_buff;
struct dma_proxy_channel_interface *recieve_buff;
static char receive[TEST_SIZE];

int main(){


  ///////////Map transmit buffer
  int fd;

  if ((fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0)
  printf("Opened file\n");

  if (fd < 0) {
  perror("/dev/mem");
  exit(-1);
  }

  printf("Pagesize: %d\n", getpagesize());

  if ((transmitt_buff = static_cast<dma_proxy_channel_interface*>(mmap(0, sizeof(dma_proxy_channel_interface), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x09000000)))
   == MAP_FAILED) {
   perror("MM2S_status_register");
   exit(-1);
   }
   ///////////////Map recieve buff

   if ((recieve_buff = static_cast<dma_proxy_channel_interface*>(mmap(0, sizeof(dma_proxy_channel_interface), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x0c000000)))
    == MAP_FAILED) {
    perror("gister");
    exit(-1);
    }


  transmitt_buff->length = TEST_SIZE;

  for(int i=0; i<TEST_SIZE; i++){
    transmitt_buff->buffer[i] = i;
  }


    for(int i=0; i<20; i++){
      printf("%d\n", recieve_buff->buffer[i]);
    }



 return 0;
}
