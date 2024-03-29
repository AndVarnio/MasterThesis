#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "CubeDMADriver.hpp"

#include <stdio.h>

#include <unistd.h>
#include <stdint.h>
#define CUBEDMA_BASE 0x43c00000
// #define CUBEDMA_BASE 0x40000000
#define SR_DONE_MSK			1U

static const int MM2S_control_register_addr_MMAP = 0;
static const int MM2S_status_register_addr_MMAP = 1;
static const int MM2S_base_adress_register_addr_MMAP = 2;
static const int MM2S_dimension_register1_addr_MMAP = 3;
static const int MM2S_dimension_register2_addr_MMAP = 4;
static const int MM2S_row_size_register_addr_MMAP = 5;

static const int S2MM_control_register_addr_MMAP = 8;
static const int S2MM_status_register_addr_MMAP = 9;
static const int S2MM_base_adress_register_addr_MMAP = 10;
static const int S2MM_recieved_length_register_addr_MMAP = 11;

uint32_t *deviceMem;

#define CR_OFFSET(mode) (mode==MM2S)? \
(MM2S_control_register_addr_MMAP): \
(S2MM_control_register_addr_MMAP)

#define SR_OFFSET(mode) (mode==MM2S)? \
(MM2S_status_register_addr_MMAP): \
(S2MM_status_register_addr_MMAP)


#define cubedma_RegWrite(offset, val) deviceMem[offset] = val;
#define cubedma_RegRead(offset) deviceMem[offset]

#define cubedma_RegSet(offset, val) \
cubedma_RegWrite(offset, cubedma_RegRead(offset)|(val))


void CubeDMADriver::cubedma_ClearInterrupts(){
  cubedma_RegWrite(SR_OFFSET(MM2S), 3U << 4); // 0011 0000
  cubedma_RegWrite(SR_OFFSET(S2MM), 3U << 4);
}

cubedma_error_t CubeDMADriver::cubedma_Init(cubedma_init_t param){

  int fd;
  fd = open("/dev/mem", O_RDWR|O_SYNC);


  if (fd < 0) {
    perror("/dev/mem");
    exit(-1);
  }

  if ((deviceMem = static_cast<uint32_t*>(mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, CUBEDMA_BASE)))
  == MAP_FAILED) {
    perror("MM2S_status_register");
    exit(-1);
  }

  deviceMem[MM2S_control_register_addr_MMAP] = \
  (param.cube.blocks.enabled & 1U) << 2 \
  | (param.cube.planewise & 1U) << 3 \
  | (param.interrupt_enable.mm2s.error & 1) << 4 \
  | (param.interrupt_enable.mm2s.complete & 1) << 5 \
  | (param.cube.n_planes & 0xFF) << 8 \
  | (param.cube.c_offset & 0xFF) << 16;

  deviceMem[MM2S_status_register_addr_MMAP] = 3U << 4;
  deviceMem[S2MM_status_register_addr_MMAP] = 3U << 4;

  deviceMem[MM2S_base_adress_register_addr_MMAP] = param.address.source;

  deviceMem[MM2S_dimension_register1_addr_MMAP] =
  (param.cube.dims.width & 0xFFF) \
  | (param.cube.dims.height & 0xFFF) << 12 \
  | (param.cube.dims.depth & 0xFF) << 24;

  deviceMem[MM2S_dimension_register2_addr_MMAP] =
  (param.cube.blocks.dims.width & 0xF) \
  | (param.cube.blocks.dims.height & 0xF) << 4 \
  | ((param.cube.dims.depth >> 8U) & 0xF) << 8 \
  | (param.cube.blocks.dims.size_last_row & 0xFFFFF) << 12;

  deviceMem[MM2S_row_size_register_addr_MMAP] = param.cube.dims.size_row;

  deviceMem[S2MM_control_register_addr_MMAP] =
  (param.interrupt_enable.s2mm.error & 1U) << 4 \
  | (param.interrupt_enable.s2mm.complete & 1U) << 5;

  deviceMem[S2MM_base_adress_register_addr_MMAP] = param.address.destination;

  return SUCCESS;
}

cubedma_error_t CubeDMADriver::cubedma_StartTransfer(transfer_t transfer){

  switch (transfer) {
    case MM2S:
    break;
    case S2MM:
    break;
    default:
    return ERR_INV_PARAM;
  }

  if ((cubedma_RegRead(CR_OFFSET(transfer))&1U) \
  && !(cubedma_RegRead(SR_OFFSET(transfer))&1U)) {
    printf("%d: ERR_BUSY\n", transfer);
    return ERR_BUSY;
  }
  printf("Starting transfer\n");

  cubedma_RegSet(CR_OFFSET(transfer), 1U);

  return SUCCESS;

}

cubedma_interrupt_t CubeDMADriver::cubedma_ReadInterrupts(transfer_t transfer) {
  cubedma_interrupt_t flags = (cubedma_interrupt_t)( \
    (cubedma_RegRead(SR_OFFSET(transfer)) >> 4) & 3U); //magic
    switch ((cubedma_interrupt_t)flags){
      case COMPLETE:
      return COMPLETE;
      case NONE:
      return NONE;
      default:
      return ERROR; // 11 or 01
    }
  }


  bool CubeDMADriver::cubedma_TransferDone(transfer_t transfer){

    if (cubedma_RegRead(SR_OFFSET(transfer)) & SR_DONE_MSK) {
      if (transfer == S2MM) {
        printf("Recieved length %u\n", deviceMem[S2MM_recieved_length_register_addr_MMAP]);
      }
      return true;
    }
    return false;
  }

  int CubeDMADriver::get_received_length(){
    return deviceMem[S2MM_recieved_length_register_addr_MMAP];
  }
