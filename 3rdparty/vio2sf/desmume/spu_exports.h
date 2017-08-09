#ifndef _SPU_EXPORTS_H
#define _SPU_EXPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

typedef struct NDS_state NDS_state;

typedef struct SoundInterface_struct
{
   int (*Init)(NDS_state *, int buffersize);
   void (*DeInit)(NDS_state *);
   void (*UpdateAudio)(NDS_state *, s16 *buffer, u32 num_samples);
} SoundInterface_struct;

void SPU_WriteLong(NDS_state *, u32 addr, u32 val);
void SPU_WriteByte(NDS_state *, u32 addr, u8 val);
void SPU_WriteWord(NDS_state *, u32 addr, u16 val);
void SPU_EmulateSamples(NDS_state *, int numsamples);
int SPU_ChangeSoundCore(NDS_state *, SoundInterface_struct *);
void SPU_Reset(NDS_state *);
int SPU_Init(NDS_state *, int, int);
void SPU_DeInit(NDS_state *);

#ifdef __cplusplus
} //extern
#endif

#endif //_SPU_EXPORTS_H
