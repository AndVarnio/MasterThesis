/*
 * Lars Henrik Bolstad 2018
 *
 * cubedma_poll_example.c: Polled CubeDMA example
 *
 * https://www.ntnu.no/wiki/display/NSSL/Cube+DMA
 *
 * This application configures and tests the CubeDMA core,
 * with unprocessed data (eg. connected through a FIFO).
 * PS7 UART (Zynq) is not initialized by this application,
 * since bootrom/bsp configures it to baud rate 115200.
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

 // #define for_each_cpu(cpu, mask)			\
 //  for ((cpu) = 0; (cpu) < 1; (cpu)++, (void)mask)
 // #define for_each_cpu_and(cpu, mask, and)	\
 //  for ((cpu) = 0; (cpu) < 1; (cpu)++, (void)mask, (void)and
 //
// #include <linux/dma-mapping.h>
#include <stdio.h>
// #include <xil_io.h> /* TODO: Remove debug traces */
// #include <xil_cache.h>
#include "cubedma.h"
// #include "platform.h"
#include <sys/time.h>
#include<time.h>
#include <stdlib.h>
#include <arm_neon.h>

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
#include "dma_parameters.h"
#include <sys/ioctl.h>

// #include <cachectl.h>
// #include <asm/outercache.h>

static struct dma_proxy_channel_interface *send_channel;
static struct dma_proxy_channel_interface *recieve_channel;


typedef enum { TEST_SUCCESS, TEST_FAIL } test_result_t;
test_result_t cubedma_RunTests();

int main()
{
    // init_platform();

    printf("Running tests\n\r");

    if (cubedma_RunTests() == TEST_SUCCESS) {
    	printf("Test successfull!\n\r");
    }
    else {
    	printf("Test failed!\n\r");
    }

    printf("Execution done!\n\r");

    // cleanup_platform();
    return 0;
}

#define COMPONENTS TEST_SIZE//1920*1080*60;
#define TIMEOUT 0xFFFFF

// volatile uint32_t source[COMPONENTS] __attribute__ ((aligned (32)));
// volatile uint32_t destin[COMPONENTS] __attribute__ ((aligned (32)));

// volatile uint8_t source[COMPONENTS] __attribute__ ((aligned (32)));
// volatile uint8_t destin[COMPONENTS] __attribute__ ((aligned (32)));

uint8_t* source;
uint8_t* destin;

// void dbg_print(uint32_t addr, uint32_t n){
// 	for (uint32_t i = 0; i < n; i++) {
// 		uint32_t val = Xil_In32(addr+i*4);
// 		printf("0x%08lx: 0x%08lx\n\r", (uint32_t)(addr)+i*4, val);
// 	}
// }

test_result_t cubedma_RunTests(){
  ///////////Map transmit buffer
  // int fd;
  //
  // if ((fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0)
  // printf("Opened file\n");
  //
  // if (fd < 0) {
  // perror("/dev/mem");
  // exit(-1);
  // }
  //
  // printf("Pagesize: %d\n", getpagesize());
  //
  // if ((source = static_cast<uint8_t*>(mmap(0, COMPONENTS, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x09000000)))
  //  == MAP_FAILED) {
  //  perror("MM2S_status_register");
  //  exit(-1);
  //  }
  //  ///////////////Map recieve buff
  //
  //  if ((destin = static_cast<uint8_t*>(mmap(0, COMPONENTS, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x0c000000)))
  //   == MAP_FAILED) {
  //   perror("gister");
  //   exit(-1);
  //   }


  // ///////////Map transmit n recieve buffer
    int fd_send = open("/dev/cubedmasend", O_RDWR);
  	if (fd_send < 1) {
  		printf("Unable to open ebbcharsend");

  	}

  	int fd_recieve = open("/dev/cubedmareceive", O_RDWR);
  	if (fd_recieve < 1) {
  		printf("Unable to open ebbcharrecieve");

  	}

    printf("Userspace: Allocating %d Bytes\n", sizeof(struct dma_proxy_channel_interface));
    send_channel = (struct dma_proxy_channel_interface *)mmap(NULL, sizeof(struct dma_proxy_channel_interface),
  									PROT_READ | PROT_WRITE, MAP_SHARED, fd_send, 0);

  	recieve_channel = (struct dma_proxy_channel_interface *)mmap(NULL, sizeof(struct dma_proxy_channel_interface),
  									PROT_READ | PROT_WRITE, MAP_SHARED, fd_recieve, 0);

      	if ((send_channel == MAP_FAILED) || (recieve_channel == MAP_FAILED)) {
          	printf("Failed to mmap\n");

      	}
  ///////////////////////////////////////////

	printf("Transferring %u components from 0x%08x to 0x%08x\n\r",
			(uint32_t)COMPONENTS, (uint32_t)&send_channel, (uint32_t)&recieve_channel);
  printf("What happens here?\n");
	cubedma_init_t cubedma_parameters = {
		.address = {
			.source      = (uint32_t)(SEND_PHYS_ADDR),
			.destination = (uint32_t)(RECIEVE_PHYS_ADDR)
      // .source      = (uint32_t)0x00120000,
			// .destination = (uint32_t)0x00154344
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
				.width = 1024,
				.height = 1024,
				.depth = 90,
				.size_row = 92160
			}
		},
		.interrupt_enable = {
			{0, 0}, {0, 0}
		}
	};
printf("Fill mem\n");
	/* Fill memory */
	for (uint32_t i = 0; i < COMPONENTS; i++) {
    // printf("Writing element: %d\n", i);
		// source[i] = (uint8_t)i;
		// destin[i] = 0xF;
    // if(i%100000==0){
    //   printf("Writing element: %d\n", i);
    // }
    send_channel->buffer[i] = (uint8_t)i;
    recieve_channel->buffer[i] = 0xF;
	}

  unsigned long dummy =658;
  ioctl(fd_send, 0, &dummy);
	/* Make sure the destination system memory is reset/cleaned for a valid
	 * test result. Source memory is flushed by the driver itself.
	 */
	printf("Flushing %d bytes or elements\n", sizeof(source));

/*	for(int row=0; row<cubedma_parameters.cube.dims.height*cubedma_parameters.cube.dims.depth; row++){
		printf("%d\n", row);
		//Xil_DCacheFlushRange((INTPTR)(destin+cubedma_parameters.cube.dims.width*row), cubedma_parameters.cube.dims.width*sizeof(destin));
		Xil_DCacheFlushRange((INTPTR)(source+cubedma_parameters.cube.dims.width*row), cubedma_parameters.cube.dims.width*sizeof(source));
	}*/

	// Xil_DCacheFlushRange((uint8_t*)source, sizeof(source));
  // cacheflush((uint8_t*)source, sizeof(source), DCACHE);
  // dcache_clean();

  //__cpuc_flush_dcache_area(source, COMPONENTS);

  printf("Starting to innit\n");

	cubedma_Init(cubedma_parameters);
	printf("Innited cubeDMA\n");

  dbg_cmp_mem();

	cubedma_StartTransfer(S2MM);
	cubedma_StartTransfer(MM2S);
  dbg_cmp_mem();
	printf("Started transfer\n");
	/* Wait for transfer to finish */
	volatile uint32_t time;
	cubedma_error_t err = ERR_TIMEOUT;
	for (time = 0; time < TIMEOUT; time++) {
		if (cubedma_TransferDone(MM2S)) {
			err = SUCCESS;
			break;
		}
	}
	if (err != SUCCESS) {
		printf("ERROR: MM2S transfer timed out!\n\r");
	}

  dbg_cmp_mem();

	err = ERR_TIMEOUT;
	for (time = 0; time < TIMEOUT; time++) {
		if (cubedma_TransferDone(S2MM)) {
			err = SUCCESS;
			break;
		}
	}
	if (err != SUCCESS) {
		printf("ERROR: S2MM transfer timed out!\n\r");
	}

	/* Check matching data */
	// uint32_t matches = 0;
	// uint32_t misses = 0;
	// for (uint32_t i = 0; i < COMPONENTS; i++){
	// 	if (source[i] == destin[i]) {
	// 		matches++;
	// 	}
	// 	else {
	// 		if (++misses < 10) {
	// 			printf("%8lx: %08lx %08lx\n\r", i*4, source[i], destin[i]);
	// 		}
	// 	}
	// }

  ioctl(fd_send, 1, &dummy);

  int nPrinted = 0;
  uint32_t matches = 0;
  uint32_t misses = 0;
  printf("i:   src   dest\n\r");
  for (uint32_t i = 0; i < COMPONENTS; i++){
    // if (source[i] == destin[i]) {
    if (send_channel->buffer[i] == recieve_channel->buffer[i]) {
      matches++;
    }
    else {
      if (recieve_channel->buffer[i]!=0xF) {
        nPrinted++;
        // printf("%u: %4u %4u\n\r", i, source[i], destin[i]);
        printf("%u: %4u %4u\n\r", i, send_channel->buffer[i], recieve_channel->buffer[i]);
      }
    }
  }

	if (matches != COMPONENTS) {
		fprintf(stderr, "ERROR: Only %f%% of the data matches\n\r", \
				(double)matches*100/COMPONENTS);
		return TEST_FAIL;
	}
	else {
		printf("Transfer success!\n\r");
	}

	/* TODO: Handle interrupt error */

	return TEST_SUCCESS;
}

static inline void dcache_clean(void)
{
    const int zero = 0;
    // /* clean entire D cache -> push to external memory. */
    // __asm volatile ("1: mrc p15, 0, r15, c7, c10, 3\n"
    //                 " bne 1b\n" ::: "cc");
   //  /* drain the write buffer */
   // __asm volatile ("mcr 15, 0, %0, c7, c10, 4"::"r" (zero));
}
