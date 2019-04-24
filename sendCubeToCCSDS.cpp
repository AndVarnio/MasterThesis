
#include <iostream>
#include "CubeDMADriver.hpp"
extern "C"{
  #include "DMA_kernel_module/dma_parameters.h"
}
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>

#define TIMEOUT 0xFFFFFF


int columns = 720;
int bands = 107;
int rows = 500;

uint16_t* buffer;

int main(int argc, char* argv[])
{
  if (argc<2) {
    puts("Usage: a.out [PATH TO RAW]");
    return -1;
  }
  const int SIZEINPUTFILE = columns*bands*rows;
  buffer = new uint16_t[SIZEINPUTFILE];

  FILE * pFile = fopen ( argv[1] , "rb" );
  if (pFile==NULL) {fputs ("File error\n",stderr); exit (1);}

  size_t result = fread (buffer, 2, SIZEINPUTFILE, pFile);
  if (result != SIZEINPUTFILE){
    printf("Read %d, SIZEINPUTFILE=%d\n", result, SIZEINPUTFILE);
    fputs ("Reading error\n",stderr);
    exit (3);
  }


  int fd_send;
  int fd_recieve;

  struct dma_data* send_channel;
  struct dma_data* recieve_channel;

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

  recieve_channel = (struct dma_data *)mmap(NULL, sizeof(struct dma_data),
                  PROT_READ | PROT_WRITE, MAP_SHARED, fd_recieve, 0);

  if ((send_channel == MAP_FAILED) || (recieve_channel == MAP_FAILED)) {
    printf("Failed to mmap\n");
  }

  for(int i=0; i<SIZEINPUTFILE; i++){
    send_channel->buffer[i].value = buffer[i];
  }

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
				.width = 720,
				.height = 500,
				.depth = 107,
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
  unsigned long dummy; //This does nothing, parameter rquired by the kernel.
  ioctl(fd_send, 0, &dummy);

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

  while(!cdma.cubedma_TransferDone(S2MM)){

  }
	if (err != SUCCESS) {
		printf("ERROR: S2MM transfer timed out!\n\r");
	}

  // Flush cache
  ioctl(fd_send, 1, &dummy); //TODO: Make enum for clean and flush command

  ///////////////Debug stuff
  int nPrinted = 0;
  uint32_t matches = 0;
  uint32_t misses = 0;
  printf("i:   src   dest\n\r");
  for (uint32_t i = 0; i < TEST_SIZE; i++){
    // if (source[i] == destin[i]) {
    if (send_channel->buffer[i].value == recieve_channel->buffer[i].value) {
      matches++;
    }
    else {
      if (nPrinted<20) {
        nPrinted++;
        // printf("%u: %4u %4u\n\r", i, source[i], destin[i]);
        printf("%u: %4u %4u\n\r", i, send_channel->buffer[i].value, recieve_channel->buffer[i].value);
      }
    }
  }

	if (matches != TEST_SIZE) {
		fprintf(stderr, "ERROR: Only %f%% of the data matches\n\r", \
				(double)matches*100/TEST_SIZE);

	}
  else {
		printf("Transfer success!\n\r");
	}

  int recieved_byte_count = cdma.get_received_length();

  FILE * fp2;
  fp2 = fopen ("compressedImageFPGA.raw","wb");

  if (fp2==NULL){
    printf("\nFile cant be opened");
    return 0;
  }

  fwrite (recieve_channel, sizeof(uint12_t), recieved_byte_count/2, fp2);//TODO g_bit_depth

  fclose (fp2);




  /////////////////////////////////

  return 0;
}
