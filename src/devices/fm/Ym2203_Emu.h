#include "deftypes.h"		/* use RAINE */

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
void* YM2203Init(UINT64 baseclock, int rate);

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
void YM2203UpdateOne(void *chip, int *buffer, int length, bool fill = true);

/*
** Write
** return : InterruptLevel
*/
int YM2203Write(void *chip,int a,unsigned char v);


void YM2203SetMute(void *chip,int mask);


void YM2203GetAllTL(void *chip,int *levels,int *freqs);
