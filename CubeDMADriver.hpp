#ifndef SRC_CUBEDMA_H_
#define SRC_CUBEDMA_H_

#include<stdint.h>
typedef enum {
	SUCCESS,
	ERR_TIMEOUT,
	ERR_BUSY,
	ERR_INV_PARAM
} cubedma_error_t;

typedef enum {
	MM2S,
	S2MM
} transfer_t;

typedef enum {
	NONE = 0,
	ERROR = 1,
	COMPLETE = 2
} cubedma_interrupt_t;

typedef struct {
	uint8_t error:1;
	uint8_t complete:1;
} cubedma_init_enable_irq_t;

typedef struct {
	struct{
		uint32_t source;
		uint32_t destination;
	} address;
	struct {
		uint8_t n_planes;
		uint8_t c_offset;
		uint8_t planewise:1;
		struct {
			uint8_t enabled:1;
			struct {
				uint8_t width:4;
				uint8_t height:4;
				uint32_t size_last_row:20;
			} dims;
		} blocks;
		struct {
			uint16_t width:12;
			uint16_t height:12;
			uint16_t depth:12;
			uint32_t size_row:20;
		} dims;
	} cube;
	struct {
		cubedma_init_enable_irq_t mm2s;
		cubedma_init_enable_irq_t s2mm;
	} interrupt_enable;
} cubedma_init_t;

class CubeDMADriver{
	public:
		cubedma_error_t cubedma_Init(cubedma_init_t param);
		cubedma_error_t cubedma_StartTransfer(transfer_t transfer);
		cubedma_interrupt_t cubedma_ReadInterrupts(transfer_t transfer);
		bool cubedma_TransferDone(transfer_t transfer);
		void cubedma_ClearInterrupts();
		int get_received_length();
	private:

};

#endif /* SRC_CUBEDMA_H_ */
