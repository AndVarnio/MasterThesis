/*
Shared data between user space and kernel space
 */
#ifndef DMA_PARAMETERS_H_
#define DMA_PARAMETERS_H_

#define CUBE_SIZE (500 * 720 * 107) // Number of samples
#define SEND_PHYS_ADDR 0x09000000
#define RECIEVE_PHYS_ADDR 0x0ffff000
#define DEVICE_NAME "cubedma"    ///< The device will appear at /dev/cubedma using this value
#define CLASS_NAME  "dma"        ///< The device class -- this is a character device driver

#pragma pack(push, 1)
typedef struct {
	uint16_t value; //Use for 16 bit values
	// uint16_t value:12; //Use for 12 bit values
} uint12_t;
#pragma pack(pop)

struct dma_data {
	uint12_t buffer[CUBE_SIZE];
	unsigned int length;
};

#endif
