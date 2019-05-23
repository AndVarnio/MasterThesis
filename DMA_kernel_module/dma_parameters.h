/* This header file is shared between the DMA Proxy test application and the DMA Proxy device driver. It defines the
 * shared interface to allow DMA transfers to be done from user space.
 *
 * Note: the buffer in the data structure should be 1st in the channel interface so that the buffer is cached aligned,
 * otherwise there may be issues when using cached memory. The issues were typically the 1st 32 bytes of the buffer
 * not working in the driver test.
 */
#ifndef DMA_PARAMETERS_H_
#define DMA_PARAMETERS_H_

#define CUBE_SIZE (500 * 720 * 107)
#define SEND_PHYS_ADDR 0x09000000
#define RECIEVE_PHYS_ADDR 0x0ffff000
#define DEVICE_NAME "cubedma"    ///< The device will appear at /dev/ebbchar using this value
#define CLASS_NAME  "dma"        ///< The device class -- this is a character device driver


#pragma pack(push, 1)
typedef struct {
	uint16_t value;
	// uint16_t value:12;
} uint12_t;
#pragma pack(pop)

struct dma_data {
	uint12_t buffer[CUBE_SIZE]; //TODO Ceiling divide TEST_SIZE by two
	unsigned int length;
};

#endif
