#ifndef ARM7_H
#define ARM7_H

#include "types.h"

typedef struct ARM7_struct {
  //ARM7 mem
  u8 ARM7_BIOS[0x4000];
  u8 ARM7_ERAM[0x10000];
  u8 ARM7_REG[0x10000];
  u8 ARM7_WIRAM[0x10000];
} ARM7_struct;

#endif
