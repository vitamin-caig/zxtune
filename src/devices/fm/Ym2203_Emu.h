#include "types.h"

/* -------------------- YM2203(OPN) Interface -------------------- */

/*
** Initialize YM2203 emulator(s).
**
** 'num'           is the number of virtual YM2203's to allocate
** 'baseclock'
** 'rate'          is sampling rate
** 'TimerHandler'  timer callback handler when timer start and clear
** 'IRQHandler'    IRQ callback handler when changed IRQ level
** return      0 = success
*/
void* YM2203Init(uint64_t baseclock, int rate);

/*
** shutdown the YM2203 emulators
*/
void YM2203Shutdown(void *chip);

/*
** reset all chip registers for YM2203 number 'num'
*/
void YM2203ResetChip(void *chip);

/*
** update one of chip
*/
void YM2203UpdateOne(void *chip, int32_t *buffer, int length);

void YM2203WriteRegs(void *chip, int reg, unsigned char val);

void YM2203GetState(void *chip, uint_t *attenuations, uint_t *periods);
