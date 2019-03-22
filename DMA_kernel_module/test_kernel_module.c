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

struct dma_proxy_channel_interface *mem_mapped_from_kernel;
static char receive[TEST_SIZE];

int main(){
 int fd;

fd = open("/dev/ebbchar", O_RDWR|O_SYNC);

if (fd < 0) {
 perror("/dev/ebbchar");
 exit(-1);
 }

 printf("Pagesize: %d\n", getpagesize());

 if ((mem_mapped_from_kernel = static_cast<struct dma_proxy_channel_interface*>(mmap(0, sizeof(dma_proxy_channel_interface), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)))
  == MAP_FAILED) {
  perror("MM2S_status_register");
  exit(-1);
  }

  mem_mapped_from_kernel->length = TEST_SIZE;

  for(int i=0; i<TEST_SIZE; i++){
    mem_mapped_from_kernel->buffer[i] = i;
  }
  printf("Reading from kernel\n");
  int ret = read(fd, receive, TEST_SIZE);

  if (ret < 0){
      perror("Failed to read the message from the device.");
      return errno;
   }

    for(int i=0; i<20; i++){
      printf("%d\n", receive[i]);
    }
 return 0;
}
