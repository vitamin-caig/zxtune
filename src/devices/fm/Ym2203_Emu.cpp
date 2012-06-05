/*

  Originally taken from MAME sources
  Extracted and adapted by Shiru

*/


#define YM2610B_WARNING

/* this is not part of the C/C++ standards and is not present on */
/* strict ANSI compilers or when compiling under GCC with -ansi */
#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

/*
**
** File: fm.c -- software implementation of Yamaha FM sound generator
**
** Copyright (C) 2001, 2002, 2003 Jarek Burczynski (bujar at mame dot net)
** Copyright (C) 1998 Tatsuyuki Satoh , MultiArcadeMachineEmulator development
**
** Version 1.4 (final beta)
**
*/

/*
** History:
**
** 14-02-2006 Alone Coder:
**  - fixed YM2203 stop volume (511 instead of MAX_ATT_INDEX) - verified on real chip
**  - fixed YM2203 SSG-EG=#0a key off (inversion disabled) - verified on real chip
**  - uncommented sine generator in SSG-EG reinit - verified on real chip
**
** 03-08-2003 Jarek Burczynski:
**  - fixed YM2608 initial values (after the reset)
**  - fixed flag and irqmask handling (YM2608)
**  - fixed BUFRDY flag handling (YM2608)
**
** 14-06-2003 Jarek Burczynski:
**  - implemented all of the YM2608 status register flags
**  - implemented support for external memory read/write via YM2608
**  - implemented support for deltat memory limit register in YM2608 emulation
**
** 22-05-2003 Jarek Burczynski:
**  - fixed LFO PM calculations (copy&paste bugfix)
**
** 08-05-2003 Jarek Burczynski:
**  - fixed SSG support
**
** 22-04-2003 Jarek Burczynski:
**  - implemented 100% correct LFO generator (verified on real YM2610 and YM2608)
**
** 15-04-2003 Jarek Burczynski:
**  - added support for YM2608's register 0x110 - status mask
**
** 01-12-2002 Jarek Burczynski:
**  - fixed register addressing in YM2608, YM2610, YM2610B chips. (verified on real YM2608)
**    The addressing patch used for early Neo-Geo games can be removed now.
**
** 26-11-2002 Jarek Burczynski, Nicola Salmoria:
**  - recreated YM2608 ADPCM ROM using data from real YM2608's output which leads to:
**  - added emulation of YM2608 drums.
**  - output of YM2608 is two times lower now - same as YM2610 (verified on real YM2608)
**
** 16-08-2002 Jarek Burczynski:
**  - binary exact Envelope Generator (verified on real YM2203);
**    identical to YM2151
**  - corrected 'off by one' error in feedback calculations (when feedback is off)
**  - corrected connection (algorithm) calculation (verified on real YM2203 and YM2610)
**
** 18-12-2001 Jarek Burczynski:
**  - added SSG-EG support (verified on real YM2203)
**
** 12-08-2001 Jarek Burczynski:
**  - corrected sin_tab and tl_tab data (verified on real chip)
**  - corrected feedback calculations (verified on real chip)
**  - corrected phase generator calculations (verified on real chip)
**  - corrected envelope generator calculations (verified on real chip)
**  - corrected FM volume level (YM2610 and YM2610B).
**  - changed YMxxxUpdateOne() functions (YM2203, YM2608, YM2610, YM2610B, YM2612) :
**    this was needed to calculate YM2610 FM channels output correctly.
**    (Each FM channel is calculated as in other chips, but the output of the channel
**    gets shifted right by one *before* sending to accumulator. That was impossible to do
**    with previous implementation).
**
** 23-07-2001 Jarek Burczynski, Nicola Salmoria:
**  - corrected YM2610 ADPCM type A algorithm and tables (verified on real chip)
**
** 11-06-2001 Jarek Burczynski:
**  - corrected end of sample bug in ADPCMA_calc_cha().
**    Real YM2610 checks for equality between current and end addresses (only 20 LSB bits).
**
** 08-12-98 hiro-shi:
** rename ADPCMA -> ADPCMB, ADPCMB -> ADPCMA
** move ROM limit check.(CALC_CH? -> 2610Write1/2)
** test program (ADPCMB_TEST)
** move ADPCM A/B end check.
** ADPCMB repeat flag(no check)
** change ADPCM volume rate (8->16) (32->48).
**
** 09-12-98 hiro-shi:
** change ADPCM volume. (8->16, 48->64)
** replace ym2610 ch0/3 (YM-2610B)
** change ADPCM_SHIFT (10->8) missing bank change 0x4000-0xffff.
** add ADPCM_SHIFT_MASK
** change ADPCMA_DECODE_MIN/MAX.
*/




/************************************************************************/
/*    comment of hiro-shi(Hiromitsu Shioya)                             */
/*    YM2610(B) = OPN-B                                                 */
/*    YM2610  : PSG:3ch FM:4ch ADPCM(18.5KHz):6ch DeltaT ADPCM:1ch      */
/*    YM2610B : PSG:3ch FM:6ch ADPCM(18.5KHz):6ch DeltaT ADPCM:1ch      */
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "Ym2203_Emu.h"

/* select bit size of output : 8 or 16 */
#define FM_SAMPLE_BITS 16

/* select timer system internal or external */
#define FM_INTERNAL_TIMER 0

/* --- speedup optimize --- */
/* busy flag enulation , The definition of FM_GET_TIME_NOW() is necessary. */
#define FM_BUSY_FLAG_SUPPORT 0


#define INLINE inline


/* globals */
#define TYPE_SSG    0x01    /* SSG support          */
#define TYPE_LFOPAN 0x02    /* OPN type LFO and PAN */
#define TYPE_6CH    0x04    /* FM 6CH / 3CH         */
#define TYPE_DAC    0x08    /* YM2612's DAC device  */
#define TYPE_ADPCM  0x10    /* two ADPCM units      */

#define FREQ_SH			16  /* 16.16 fixed point (frequency calculations) */
#define EG_SH			16  /* 16.16 fixed point (envelope generator timing) */
#define LFO_SH			24  /*  8.24 fixed point (LFO calculations)       */
#define TIMER_SH		16  /* 16.16 fixed point (timers calculations)    */

#define FREQ_MASK		((1<<FREQ_SH)-1)

#define ENV_BITS		10
#define ENV_LEN			(1<<ENV_BITS)
#define ENV_STEP		(128.0/ENV_LEN)

#define MAX_ATT_INDEX	(ENV_LEN-1) /* 1023 */
#define MIN_ATT_INDEX	(0)			/* 0 */

#define EG_ATT			4
#define EG_DEC			3
#define EG_SUS			2
#define EG_REL			1
#define EG_OFF			0

#define SIN_BITS		10
#define SIN_LEN			(1<<SIN_BITS)
#define SIN_MASK		(SIN_LEN-1)

#define TL_RES_LEN		(256) /* 8 bits addressing (real chip) */


#if (FM_SAMPLE_BITS==16)
	#define FINAL_SH	(0)
	#define MAXOUT		(+32767)
	#define MINOUT		(-32768)
#else
	#define FINAL_SH	(8)
	#define MAXOUT		(+127)
	#define MINOUT		(-128)
#endif


//contextless tables

/*  TL_TAB_LEN is calculated as:
*   13 - sinus amplitude bits     (Y axis)
*   2  - sinus sign bit           (Y axis)
*   TL_RES_LEN - sinus resolution (X axis)
*/
#define TL_TAB_LEN (13*2*TL_RES_LEN)
static signed int tl_tab[TL_TAB_LEN];

#define ENV_QUIET		(TL_TAB_LEN>>3)

/* sin waveform table in 'decibel' scale */
static unsigned int sin_tab[SIN_LEN];

/* sustain level table (3dB per step) */
/* bit0, bit1, bit2, bit3, bit4, bit5, bit6 */
/* 1,    2,    4,    8,    16,   32,   64   (value)*/
/* 0.75, 1.5,  3,    6,    12,   24,   48   (dB)*/

/* 0 - 15: 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,93 (dB)*/
#define SC(db) (UINT32) ( db * (4.0/ENV_STEP) )
static const UINT32 sl_table[16]={
 SC( 0),SC( 1),SC( 2),SC(3 ),SC(4 ),SC(5 ),SC(6 ),SC( 7),
 SC( 8),SC( 9),SC(10),SC(11),SC(12),SC(13),SC(14),SC(31)
};
#undef SC


#define RATE_STEPS (8)
static const UINT8 eg_inc[19*RATE_STEPS]={

/*cycle:0 1  2 3  4 5  6 7*/

/* 0 */ 0,1, 0,1, 0,1, 0,1, /* rates 00..11 0 (increment by 0 or 1) */
/* 1 */ 0,1, 0,1, 1,1, 0,1, /* rates 00..11 1 */
/* 2 */ 0,1, 1,1, 0,1, 1,1, /* rates 00..11 2 */
/* 3 */ 0,1, 1,1, 1,1, 1,1, /* rates 00..11 3 */

/* 4 */ 1,1, 1,1, 1,1, 1,1, /* rate 12 0 (increment by 1) */
/* 5 */ 1,1, 1,2, 1,1, 1,2, /* rate 12 1 */
/* 6 */ 1,2, 1,2, 1,2, 1,2, /* rate 12 2 */
/* 7 */ 1,2, 2,2, 1,2, 2,2, /* rate 12 3 */

/* 8 */ 2,2, 2,2, 2,2, 2,2, /* rate 13 0 (increment by 2) */
/* 9 */ 2,2, 2,4, 2,2, 2,4, /* rate 13 1 */
/*10 */ 2,4, 2,4, 2,4, 2,4, /* rate 13 2 */
/*11 */ 2,4, 4,4, 2,4, 4,4, /* rate 13 3 */

/*12 */ 4,4, 4,4, 4,4, 4,4, /* rate 14 0 (increment by 4) */
/*13 */ 4,4, 4,8, 4,4, 4,8, /* rate 14 1 */
/*14 */ 4,8, 4,8, 4,8, 4,8, /* rate 14 2 */
/*15 */ 4,8, 8,8, 4,8, 8,8, /* rate 14 3 */

/*16 */ 8,8, 8,8, 8,8, 8,8, /* rates 15 0, 15 1, 15 2, 15 3 (increment by 8) */
/*17 */ 16,16,16,16,16,16,16,16, /* rates 15 2, 15 3 for attack */
/*18 */ 0,0, 0,0, 0,0, 0,0, /* infinity rates for attack and decay(s) */
};


#define O(a) (a*RATE_STEPS)

/*note that there is no O(17) in this table - it's directly in the code */
static const UINT8 eg_rate_select[32+64+32]={	/* Envelope Generator rates (32 + 64 rates + 32 RKS) */
/* 32 infinite time rates */
O(18),O(18),O(18),O(18),O(18),O(18),O(18),O(18),
O(18),O(18),O(18),O(18),O(18),O(18),O(18),O(18),
O(18),O(18),O(18),O(18),O(18),O(18),O(18),O(18),
O(18),O(18),O(18),O(18),O(18),O(18),O(18),O(18),

/* rates 00-11 */
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),
O( 0),O( 1),O( 2),O( 3),

/* rate 12 */
O( 4),O( 5),O( 6),O( 7),

/* rate 13 */
O( 8),O( 9),O(10),O(11),

/* rate 14 */
O(12),O(13),O(14),O(15),

/* rate 15 */
O(16),O(16),O(16),O(16),

/* 32 dummy rates (same as 15 3) */
O(16),O(16),O(16),O(16),O(16),O(16),O(16),O(16),
O(16),O(16),O(16),O(16),O(16),O(16),O(16),O(16),
O(16),O(16),O(16),O(16),O(16),O(16),O(16),O(16),
O(16),O(16),O(16),O(16),O(16),O(16),O(16),O(16)

};
#undef O

/*rate  0,    1,    2,   3,   4,   5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15*/
/*shift 11,   10,   9,   8,   7,   6,  5,  4,  3,  2, 1,  0,  0,  0,  0,  0 */
/*mask  2047, 1023, 511, 255, 127, 63, 31, 15, 7,  3, 1,  0,  0,  0,  0,  0 */

#define O(a) (a*1)
static const UINT8 eg_rate_shift[32+64+32]={	/* Envelope Generator counter shifts (32 + 64 rates + 32 RKS) */
/* 32 infinite time rates */
O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),
O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),
O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),
O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),

/* rates 00-11 */
O(11),O(11),O(11),O(11),
O(10),O(10),O(10),O(10),
O( 9),O( 9),O( 9),O( 9),
O( 8),O( 8),O( 8),O( 8),
O( 7),O( 7),O( 7),O( 7),
O( 6),O( 6),O( 6),O( 6),
O( 5),O( 5),O( 5),O( 5),
O( 4),O( 4),O( 4),O( 4),
O( 3),O( 3),O( 3),O( 3),
O( 2),O( 2),O( 2),O( 2),
O( 1),O( 1),O( 1),O( 1),
O( 0),O( 0),O( 0),O( 0),

/* rate 12 */
O( 0),O( 0),O( 0),O( 0),

/* rate 13 */
O( 0),O( 0),O( 0),O( 0),

/* rate 14 */
O( 0),O( 0),O( 0),O( 0),

/* rate 15 */
O( 0),O( 0),O( 0),O( 0),

/* 32 dummy rates (same as 15 3) */
O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0)

};
#undef O

static const UINT8 dt_tab[4 * 32]={
/* this is YM2151 and YM2612 phase increment data (in 10.10 fixed point format)*/
/* FD=0 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* FD=1 */
	0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
	2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 8, 8,
/* FD=2 */
	1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5,
	5, 6, 6, 7, 8, 8, 9,10,11,12,13,14,16,16,16,16,
/* FD=3 */
	2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7,
	8 , 8, 9,10,11,12,13,14,16,17,19,20,22,22,22,22
};


/* OPN key frequency number -> key code follow table */
/* fnum higher 4bit -> keycode lower 2bit */
static const UINT8 opn_fktable[16] = {0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3};


/* 8 LFO speed parameters */
/* each value represents number of samples that one LFO level will last for */
static const UINT32 lfo_samples_per_step[8] = {108, 77, 71, 67, 62, 44, 8, 5};



/*There are 4 different LFO AM depths available, they are:
  0 dB, 1.4 dB, 5.9 dB, 11.8 dB
  Here is how it is generated (in EG steps):

  11.8 dB = 0, 2, 4, 6, 8, 10,12,14,16...126,126,124,122,120,118,....4,2,0
   5.9 dB = 0, 1, 2, 3, 4, 5, 6, 7, 8....63, 63, 62, 61, 60, 59,.....2,1,0
   1.4 dB = 0, 0, 0, 0, 1, 1, 1, 1, 2,...15, 15, 15, 15, 14, 14,.....0,0,0

  (1.4 dB is loosing precision as you can see)

  It's implemented as generator from 0..126 with step 2 then a shift
  right N times, where N is:
    8 for 0 dB
    3 for 1.4 dB
    1 for 5.9 dB
    0 for 11.8 dB
*/
static const UINT8 lfo_ams_depth_shift[4] = {8, 3, 1, 0};



/*There are 8 different LFO PM depths available, they are:
  0, 3.4, 6.7, 10, 14, 20, 40, 80 (cents)

  Modulation level at each depth depends on F-NUMBER bits: 4,5,6,7,8,9,10
  (bits 8,9,10 = FNUM MSB from OCT/FNUM register)

  Here we store only first quarter (positive one) of full waveform.
  Full table (lfo_pm_table) containing all 128 waveforms is build
  at run (init) time.

  One value in table below represents 4 (four) basic LFO steps
  (1 PM step = 4 AM steps).

  For example:
   at LFO SPEED=0 (which is 108 samples per basic LFO step)
   one value from "lfo_pm_output" table lasts for 432 consecutive
   samples (4*108=432) and one full LFO waveform cycle lasts for 13824
   samples (32*432=13824; 32 because we store only a quarter of whole
            waveform in the table below)
*/
static const UINT8 lfo_pm_output[7*8][8]={ /* 7 bits meaningful (of F-NUMBER), 8 LFO output levels per one depth (out of 32), 8 LFO depths */
/* FNUM BIT 4: 000 0001xxxx */
/* DEPTH 0 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 1 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 2 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 3 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 4 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 5 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 6 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 7 */ {0,   0,   0,   0,   1,   1,   1,   1},

/* FNUM BIT 5: 000 0010xxxx */
/* DEPTH 0 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 1 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 2 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 3 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 4 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 5 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 6 */ {0,   0,   0,   0,   1,   1,   1,   1},
/* DEPTH 7 */ {0,   0,   1,   1,   2,   2,   2,   3},

/* FNUM BIT 6: 000 0100xxxx */
/* DEPTH 0 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 1 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 2 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 3 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 4 */ {0,   0,   0,   0,   0,   0,   0,   1},
/* DEPTH 5 */ {0,   0,   0,   0,   1,   1,   1,   1},
/* DEPTH 6 */ {0,   0,   1,   1,   2,   2,   2,   3},
/* DEPTH 7 */ {0,   0,   2,   3,   4,   4,   5,   6},

/* FNUM BIT 7: 000 1000xxxx */
/* DEPTH 0 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 1 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 2 */ {0,   0,   0,   0,   0,   0,   1,   1},
/* DEPTH 3 */ {0,   0,   0,   0,   1,   1,   1,   1},
/* DEPTH 4 */ {0,   0,   0,   1,   1,   1,   1,   2},
/* DEPTH 5 */ {0,   0,   1,   1,   2,   2,   2,   3},
/* DEPTH 6 */ {0,   0,   2,   3,   4,   4,   5,   6},
/* DEPTH 7 */ {0,   0,   4,   6,   8,   8, 0xa, 0xc},

/* FNUM BIT 8: 001 0000xxxx */
/* DEPTH 0 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 1 */ {0,   0,   0,   0,   1,   1,   1,   1},
/* DEPTH 2 */ {0,   0,   0,   1,   1,   1,   2,   2},
/* DEPTH 3 */ {0,   0,   1,   1,   2,   2,   3,   3},
/* DEPTH 4 */ {0,   0,   1,   2,   2,   2,   3,   4},
/* DEPTH 5 */ {0,   0,   2,   3,   4,   4,   5,   6},
/* DEPTH 6 */ {0,   0,   4,   6,   8,   8, 0xa, 0xc},
/* DEPTH 7 */ {0,   0,   8, 0xc,0x10,0x10,0x14,0x18},

/* FNUM BIT 9: 010 0000xxxx */
/* DEPTH 0 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 1 */ {0,   0,   0,   0,   2,   2,   2,   2},
/* DEPTH 2 */ {0,   0,   0,   2,   2,   2,   4,   4},
/* DEPTH 3 */ {0,   0,   2,   2,   4,   4,   6,   6},
/* DEPTH 4 */ {0,   0,   2,   4,   4,   4,   6,   8},
/* DEPTH 5 */ {0,   0,   4,   6,   8,   8, 0xa, 0xc},
/* DEPTH 6 */ {0,   0,   8, 0xc,0x10,0x10,0x14,0x18},
/* DEPTH 7 */ {0,   0,0x10,0x18,0x20,0x20,0x28,0x30},

/* FNUM BIT10: 100 0000xxxx */
/* DEPTH 0 */ {0,   0,   0,   0,   0,   0,   0,   0},
/* DEPTH 1 */ {0,   0,   0,   0,   4,   4,   4,   4},
/* DEPTH 2 */ {0,   0,   0,   4,   4,   4,   8,   8},
/* DEPTH 3 */ {0,   0,   4,   4,   8,   8, 0xc, 0xc},
/* DEPTH 4 */ {0,   0,   4,   8,   8,   8, 0xc,0x10},
/* DEPTH 5 */ {0,   0,   8, 0xc,0x10,0x10,0x14,0x18},
/* DEPTH 6 */ {0,   0,0x10,0x18,0x20,0x20,0x28,0x30},
/* DEPTH 7 */ {0,   0,0x20,0x30,0x40,0x40,0x50,0x60},

};

/* all 128 LFO PM waveforms */
static INT32 lfo_pm_table[128*8*32]; /* 128 combinations of 7 bits meaningful (of F-NUMBER), 8 LFO depths, 32 LFO output levels per one depth */





/* register number to channel number , slot offset */
#define OPN_CHAN(N) (N&3)
#define OPN_SLOT(N) ((N>>2)&3)

/* slot number */
#define SLOT1 0
#define SLOT2 2
#define SLOT3 1
#define SLOT4 3

/* bit0 = Right enable , bit1 = Left enable */
#define OUTD_RIGHT  1
#define OUTD_LEFT   2
#define OUTD_CENTER 3


/* struct describing a single operator (SLOT) */
typedef struct
{
	INT32	*DT;		/* detune          :dt_tab[DT] */
	UINT8	KSR;		/* key scale rate  :3-KSR */
	UINT32	ar;			/* attack rate  */
	UINT32	d1r;		/* decay rate   */
	UINT32	d2r;		/* sustain rate */
	UINT32	rr;			/* release rate */
	UINT8	ksr;		/* key scale rate  :kcode>>(3-KSR) */
	UINT32	mul;		/* multiple        :ML_TABLE[ML] */

	/* Phase Generator */
	UINT32	phase;		/* phase counter */
	UINT32	Incr;		/* phase step */

	/* Envelope Generator */
	UINT8	state;		/* phase type */
	UINT32	tl;			/* total level: TL << 3 */
	INT32	volume;		/* envelope counter */
	UINT32	sl;			/* sustain level:sl_table[SL] */
	UINT32	vol_out;	/* current output from EG circuit (without AM from LFO) */

	UINT8	eg_sh_ar;	/*  (attack state) */
	UINT8	eg_sel_ar;	/*  (attack state) */
	UINT8	eg_sh_d1r;	/*  (decay state) */
	UINT8	eg_sel_d1r;	/*  (decay state) */
	UINT8	eg_sh_d2r;	/*  (sustain state) */
	UINT8	eg_sel_d2r;	/*  (sustain state) */
	UINT8	eg_sh_rr;	/*  (release state) */
	UINT8	eg_sel_rr;	/*  (release state) */

	UINT8	ssg;		/* SSG-EG waveform */
	UINT8	ssgn;		/* SSG-EG negated output */

	UINT32	key;		/* 0=last key was KEY OFF, 1=KEY ON */

	/* LFO */
	UINT32	AMmask;		/* AM enable flag */

} FM_SLOT;

typedef struct
{
	FM_SLOT	SLOT[4];	/* four SLOTs (operators) */

	UINT8	ALGO;		/* algorithm */
	UINT8	FB;			/* feedback shift */
	INT32	op1_out[2];	/* op1 output for feedback */

	INT32	*connect1;	/* SLOT1 output pointer */
	INT32	*connect3;	/* SLOT3 output pointer */
	INT32	*connect2;	/* SLOT2 output pointer */
	INT32	*connect4;	/* SLOT4 output pointer */

	INT32	*mem_connect;/* where to put the delayed sample (MEM) */
	INT32	mem_value;	/* delayed sample (MEM) value */

	INT32	pms;		/* channel PMS */
	UINT8	ams;		/* channel AMS */

	UINT32	fc;			/* fnum,blk:adjusted to sample rate */
	UINT8	kcode;		/* key code:                        */
	UINT32	block_fnum;	/* current blk/fnum value for this slot (can be different betweeen slots of one channel in 3slot mode) */
} FM_CH;


typedef struct
{
	void *	param;		/* this chip parameter  */
	UINT64		clock;		/* master clock  (Hz)   */
	int		rate;		/* sampling rate (Hz)   */
	double	freqbase;	/* frequency base       */
	double	TimerBase;	/* Timer base time      */
#if FM_BUSY_FLAG_SUPPORT
	double	BusyExpire;	/* ExpireTime of Busy clear */
#endif
	UINT8	address;	/* address register     */
	UINT8	irq;		/* interrupt level      */
	UINT8	irqmask;	/* irq mask             */
	UINT8	status;		/* status flag          */
	UINT32	mode;		/* mode  CSM / 3SLOT    */
	UINT8	prescaler_sel;/* prescaler selector */
	UINT8	fn_h;		/* freq latch           */
	INT32	TA;			/* timer a              */
	INT32	TAC;		/* timer a counter      */
	UINT8	TB;			/* timer b              */
	INT32	TBC;		/* timer b counter      */
	/* local time tables */
	INT32	dt_tab[8][32];/* DeTune table       */
	/* Extention Timer and IRQ handler */
	//FM_TIMERHANDLER	Timer_Handler;
	//FM_IRQHANDLER	IRQ_Handler;
	//const struct ssg_callbacks *SSG;
} FM_ST;



/***********************************************************/
/* OPN unit                                                */
/***********************************************************/

/* OPN 3slot struct */
typedef struct
{
	UINT32  fc[3];			/* fnum3,blk3: calculated */
	UINT8	fn_h;			/* freq3 latch */
	UINT8	kcode[3];		/* key code */
	UINT32	block_fnum[3];	/* current fnum value for this slot (can be different betweeen slots of one channel in 3slot mode) */
} FM_3SLOT;

/* OPN/A/B common state */
typedef struct
{
	UINT8	type;			/* chip type */
	FM_ST	ST;				/* general state */
	FM_3SLOT SL3;			/* 3 slot mode state */
	FM_CH	*P_CH;			/* pointer of CH */
	unsigned int pan[6*2];	/* fm channels output masks (0xffffffff = enable) */

	UINT32	eg_cnt;			/* global envelope generator counter */
	UINT32	eg_timer;		/* global envelope generator counter works at frequency = chipclock/64/3 */
	UINT32	eg_timer_add;	/* step of eg_timer */
	UINT32	eg_timer_overflow;/* envelope generator timer overlfows every 3 samples (on real chip) */


	/* there are 2048 FNUMs that can be generated using FNUM/BLK registers
        but LFO works with one more bit of a precision so we really need 4096 elements */

	UINT32	fn_table[4096];	/* fnumber->increment counter */


	/* LFO */
	UINT32	lfo_cnt;
	UINT32	lfo_inc;

	UINT32	lfo_freq[8];	/* LFO FREQ table */
} FM_OPN;

/* current chip state */
typedef struct
{
  INT32	m2,c1,c2;		/* Phase Modulation input for operators 2,3,4 */
  INT32	mem;			/* one sample delay memory */

  INT32	out_fm[8];		/* outputs of working channels */

  UINT32	LFO_AM;			/* runtime LFO calculations helper */
  INT32	LFO_PM;			/* runtime LFO calculations helper */
} FM_STATE;


/* log output level */
#define LOG_ERR  3      /* ERROR       */
#define LOG_WAR  2      /* WARNING     */
#define LOG_INF  1      /* INFORMATION */
#define LOG_LEVEL LOG_INF

#ifndef __RAINE__
#define LOG(n,x) if( (n)>=LOG_LEVEL ) logerror x
#endif

/* limitter */
#define Limit(val, max,min) { \
	if ( val > max )      val = max; \
	else if ( val < min ) val = min; \
}


/* status set and IRQ handling */
inline void FM_STATUS_SET(FM_ST *ST,int flag)
{
	/* set status flag */
	ST->status |= flag;
	if ( !(ST->irq) && (ST->status & ST->irqmask) )
	{
		ST->irq = 1;
		/* callback user interrupt handler (IRQ is OFF to ON) */
		//if(ST->IRQ_Handler) (ST->IRQ_Handler)(ST->param,1);
	}
}

/* status reset and IRQ handling */
inline void FM_STATUS_RESET(FM_ST *ST,int flag)
{
	/* reset status flag */
	ST->status &=~flag;
	if ( (ST->irq) && !(ST->status & ST->irqmask) )
	{
		ST->irq = 0;
		/* callback user interrupt handler (IRQ is ON to OFF) */
		//if(ST->IRQ_Handler) (ST->IRQ_Handler)(ST->param,0);
	}
}

/* IRQ mask set */
inline void FM_IRQMASK_SET(FM_ST *ST,int flag)
{
	ST->irqmask = flag;
	/* IRQ handling check */
	FM_STATUS_SET(ST,0);
	FM_STATUS_RESET(ST,0);
}

/* OPN Mode Register Write */
inline void set_timers( FM_ST *ST, void *n, int v )
{
	/* b7 = CSM MODE */
	/* b6 = 3 slot mode */
	/* b5 = reset b */
	/* b4 = reset a */
	/* b3 = timer enable b */
	/* b2 = timer enable a */
	/* b1 = load b */
	/* b0 = load a */
	ST->mode = v;

	/* reset Timer b flag */
	if( v & 0x20 )
		FM_STATUS_RESET(ST,0x02);
	/* reset Timer a flag */
	if( v & 0x10 )
		FM_STATUS_RESET(ST,0x01);
	/* load b */
	if( v & 0x02 )
	{
		if( ST->TBC == 0 )
		{
			ST->TBC = ( 256-ST->TB)<<4;
			/* External timer handler */
			//if (ST->Timer_Handler) (ST->Timer_Handler)(n,1,ST->TBC,ST->TimerBase);
		}
	}
	else
	{	/* stop timer b */
		if( ST->TBC != 0 )
		{
			ST->TBC = 0;
			//if (ST->Timer_Handler) (ST->Timer_Handler)(n,1,0,ST->TimerBase);
		}
	}
	/* load a */
	if( v & 0x01 )
	{
		if( ST->TAC == 0 )
		{
			ST->TAC = (1024-ST->TA);
			/* External timer handler */
			//if (ST->Timer_Handler) (ST->Timer_Handler)(n,0,ST->TAC,ST->TimerBase);
		}
	}
	else
	{	/* stop timer a */
		if( ST->TAC != 0 )
		{
			ST->TAC = 0;
			//if (ST->Timer_Handler) (ST->Timer_Handler)(n,0,0,ST->TimerBase);
		}
	}
}


/* Timer A Overflow */
inline void TimerAOver(FM_ST *ST)
{
	/* set status (if enabled) */
	if(ST->mode & 0x04) FM_STATUS_SET(ST,0x01);
	/* clear or reload the counter */
	ST->TAC = (1024-ST->TA);
	//if (ST->Timer_Handler) (ST->Timer_Handler)(ST->param,0,ST->TAC,ST->TimerBase);
}
/* Timer B Overflow */
inline void TimerBOver(FM_ST *ST)
{
	/* set status (if enabled) */
	if(ST->mode & 0x08) FM_STATUS_SET(ST,0x02);
	/* clear or reload the counter */
	ST->TBC = ( 256-ST->TB)<<4;
	//if (ST->Timer_Handler) (ST->Timer_Handler)(ST->param,1,ST->TBC,ST->TimerBase);
}


#if FM_INTERNAL_TIMER
/* ----- internal timer mode , update timer */

/* ---------- calculate timer A ---------- */
	#define INTERNAL_TIMER_A(ST,CSM_CH)					\
	{													\
		if( ST->TAC &&  (ST->Timer_Handler==0) )		\
			if( (ST->TAC -= (int)(ST->freqbase*4096)) <= 0 )	\
			{											\
				TimerAOver( ST );						\
				/* CSM mode total level latch and auto key on */	\
				if( ST->mode & 0x80 )					\
					CSMKeyControll( CSM_CH );			\
			}											\
	}
/* ---------- calculate timer B ---------- */
	#define INTERNAL_TIMER_B(ST,step)						\
	{														\
		if( ST->TBC && (ST->Timer_Handler==0) )				\
			if( (ST->TBC -= (int)(ST->freqbase*4096*step)) <= 0 )	\
				TimerBOver( ST );							\
	}
#else /* FM_INTERNAL_TIMER */
/* external timer mode */
#define INTERNAL_TIMER_A(ST,CSM_CH)
#define INTERNAL_TIMER_B(ST,step)
#endif /* FM_INTERNAL_TIMER */



#if FM_BUSY_FLAG_SUPPORT
inline UINT8 FM_STATUS_FLAG(FM_ST *ST)
{
	if( ST->BusyExpire )
	{
		if( (ST->BusyExpire - FM_GET_TIME_NOW()) > 0)
			return ST->status | 0x80;	/* with busy */
		/* expire */
		ST->BusyExpire = 0;
	}
	return ST->status;
}
inline void FM_BUSY_SET(FM_ST *ST,int busyclock )
{
	ST->BusyExpire = FM_GET_TIME_NOW() + (ST->TimerBase * busyclock);
}
#define FM_BUSY_CLEAR(ST) ((ST)->BusyExpire = 0)
#else
#define FM_STATUS_FLAG(ST) ((ST)->status)
#define FM_BUSY_SET(ST,bclock) {}
#define FM_BUSY_CLEAR(ST) {}
#endif




inline void FM_KEYON(FM_CH *CH , int s )
{
	FM_SLOT *SLOT = &CH->SLOT[s];
	if( !SLOT->key )
	{
		SLOT->key = 1;
		SLOT->phase = 0;		/* restart Phase Generator */
		SLOT->state = EG_ATT;	/* phase -> Attack */
		if ( SLOT->volume >= MAX_ATT_INDEX )SLOT->volume = 511;	/* Alone Coder */
	}
}

inline void FM_KEYOFF(FM_CH *CH , int s )
{
	FM_SLOT *SLOT = &CH->SLOT[s];
	if( SLOT->key )
	{
		SLOT->key = 0;
		if (SLOT->state>EG_REL)
			SLOT->state = EG_REL;/* phase -> Release */
	}
}

/* set algorithm connection */
static void setup_connection(FM_STATE* state, FM_CH *CH, int ch )
{
	INT32 *carrier = &state->out_fm[ch];

	INT32 **om1 = &CH->connect1;
	INT32 **om2 = &CH->connect3;
	INT32 **oc1 = &CH->connect2;

	INT32 **memc = &CH->mem_connect;

	switch( CH->ALGO ){
	case 0:
		/* M1---C1---MEM---M2---C2---OUT */
		*om1 = &state->c1;
		*oc1 = &state->mem;
		*om2 = &state->c2;
		*memc= &state->m2;
		break;
	case 1:
		/* M1------+-MEM---M2---C2---OUT */
		/*      C1-+                     */
		*om1 = &state->mem;
		*oc1 = &state->mem;
		*om2 = &state->c2;
		*memc= &state->m2;
		break;
	case 2:
		/* M1-----------------+-C2---OUT */
		/*      C1---MEM---M2-+          */
		*om1 = &state->c2;
		*oc1 = &state->mem;
		*om2 = &state->c2;
		*memc= &state->m2;
		break;
	case 3:
		/* M1---C1---MEM------+-C2---OUT */
		/*                 M2-+          */
		*om1 = &state->c1;
		*oc1 = &state->mem;
		*om2 = &state->c2;
		*memc= &state->c2;
		break;
	case 4:
		/* M1---C1-+-OUT */
		/* M2---C2-+     */
		/* MEM: not used */
		*om1 = &state->c1;
		*oc1 = carrier;
		*om2 = &state->c2;
		*memc= &state->mem;	/* store it anywhere where it will not be used */
		break;
	case 5:
		/*    +----C1----+     */
		/* M1-+-MEM---M2-+-OUT */
		/*    +----C2----+     */
		*om1 = 0;	/* special mark */
		*oc1 = carrier;
		*om2 = carrier;
		*memc= &state->m2;
		break;
	case 6:
		/* M1---C1-+     */
		/*      M2-+-OUT */
		/*      C2-+     */
		/* MEM: not used */
		*om1 = &state->c1;
		*oc1 = carrier;
		*om2 = carrier;
		*memc= &state->mem;	/* store it anywhere where it will not be used */
		break;
	case 7:
		/* M1-+     */
		/* C1-+-OUT */
		/* M2-+     */
		/* C2-+     */
		/* MEM: not used*/
		*om1 = carrier;
		*oc1 = carrier;
		*om2 = carrier;
		*memc= &state->mem;	/* store it anywhere where it will not be used */
		break;
	}

	CH->connect4 = carrier;
}

/* set detune & multiple */
inline void set_det_mul(FM_ST *ST,FM_CH *CH,FM_SLOT *SLOT,int v)
{
	SLOT->mul = (v&0x0f)? (v&0x0f)*2 : 1;
	SLOT->DT  = ST->dt_tab[(v>>4)&7];
	CH->SLOT[SLOT1].Incr=-1;
}

/* set total level */
inline void set_tl(FM_CH *CH,FM_SLOT *SLOT , int v)
{
	SLOT->tl = (v&0x7f)<<(ENV_BITS-7); /* 7bit TL */
}

/* set attack rate & key scale  */
inline void set_ar_ksr(FM_CH *CH,FM_SLOT *SLOT,int v)
{
	UINT8 old_KSR = SLOT->KSR;

	SLOT->ar = (v&0x1f) ? 32 + ((v&0x1f)<<1) : 0;

	SLOT->KSR = 3-(v>>6);
	if (SLOT->KSR != old_KSR)
	{
		CH->SLOT[SLOT1].Incr=-1;
	}
	else
	{
		/* refresh Attack rate */
		if ((SLOT->ar + SLOT->ksr) < 32+62)
		{
			SLOT->eg_sh_ar  = eg_rate_shift [SLOT->ar  + SLOT->ksr ];
			SLOT->eg_sel_ar = eg_rate_select[SLOT->ar  + SLOT->ksr ];
		}
		else
		{
			SLOT->eg_sh_ar  = 0;
			SLOT->eg_sel_ar = 17*RATE_STEPS;
		}
	}
}

/* set decay rate */
inline void set_dr(FM_SLOT *SLOT,int v)
{
	SLOT->d1r = (v&0x1f) ? 32 + ((v&0x1f)<<1) : 0;

	SLOT->eg_sh_d1r = eg_rate_shift [SLOT->d1r + SLOT->ksr];
	SLOT->eg_sel_d1r= eg_rate_select[SLOT->d1r + SLOT->ksr];

}

/* set sustain rate */
inline void set_sr(FM_SLOT *SLOT,int v)
{
	SLOT->d2r = (v&0x1f) ? 32 + ((v&0x1f)<<1) : 0;

	SLOT->eg_sh_d2r = eg_rate_shift [SLOT->d2r + SLOT->ksr];
	SLOT->eg_sel_d2r= eg_rate_select[SLOT->d2r + SLOT->ksr];
}

/* set release rate */
inline void set_sl_rr(FM_SLOT *SLOT,int v)
{
	SLOT->sl = sl_table[ v>>4 ];

	SLOT->rr  = 34 + ((v&0x0f)<<2);

	SLOT->eg_sh_rr  = eg_rate_shift [SLOT->rr  + SLOT->ksr];
	SLOT->eg_sel_rr = eg_rate_select[SLOT->rr  + SLOT->ksr];
}



inline signed int op_calc(UINT32 phase, unsigned int env, signed int pm)
{
	UINT32 p;

	p = (env<<3) + sin_tab[ ( ((signed int)((phase & ~FREQ_MASK) + (pm<<15))) >> FREQ_SH ) & SIN_MASK ];

	if (p >= TL_TAB_LEN)
		return 0;
	return tl_tab[p];
}

inline signed int op_calc1(UINT32 phase, unsigned int env, signed int pm)
{
	UINT32 p;

	p = (env<<3) + sin_tab[ ( ((signed int)((phase & ~FREQ_MASK) + pm      )) >> FREQ_SH ) & SIN_MASK ];

	if (p >= TL_TAB_LEN)
		return 0;
	return tl_tab[p];
}

/* advance LFO to next sample */
inline void advance_lfo(FM_STATE* state, FM_OPN *OPN)
{
	UINT8 pos;
	UINT8 prev_pos;

	if (OPN->lfo_inc)	/* LFO enabled ? */
	{
		prev_pos = OPN->lfo_cnt>>LFO_SH & 127;

		OPN->lfo_cnt += OPN->lfo_inc;

		pos = (OPN->lfo_cnt >> LFO_SH) & 127;


		/* update AM when LFO output changes */

		/*if (prev_pos != pos)*/
		/* actually I can't optimize is this way without rewritting chan_calc()
        to use chip->lfo_am instead of global lfo_am */
		{

			/* triangle */
			/* AM: 0 to 126 step +2, 126 to 0 step -2 */
			if (pos<64)
				state->LFO_AM = (pos&63) * 2;
			else
				state->LFO_AM = 126 - ((pos&63) * 2);
		}

		/* PM works with 4 times slower clock */
		prev_pos >>= 2;
		pos >>= 2;
		/* update PM when LFO output changes */
		/*if (prev_pos != pos)*/ /* can't use global lfo_pm for this optimization, must be chip->lfo_pm instead*/
		{
			state->LFO_PM = pos;
		}

	}
	else
	{
		state->LFO_AM = 0;
		state->LFO_PM = 0;
	}
}

inline void advance_eg_channel(FM_OPN *OPN, FM_SLOT *SLOT)
{
	unsigned int out;
	unsigned int swap_flag = 0;
	unsigned int i;

#define SSGEG_SCALE	4 //8 //вместо 4, как в оригинальном исходнике - так более правдиво звучит, но всё равно не совсем то

	i = 4; /* four operators per channel */
	do
	{
		switch(SLOT->state)
		{
		case EG_ATT:		/* attack phase */
			if ( !(OPN->eg_cnt & ((1<<SLOT->eg_sh_ar)-1) ) )
			{
				SLOT->volume += (~SLOT->volume *
                                  (eg_inc[SLOT->eg_sel_ar + ((OPN->eg_cnt>>SLOT->eg_sh_ar)&7)])
                                ) >>4;

				if (SLOT->volume <= MIN_ATT_INDEX)
				{
					SLOT->volume = MIN_ATT_INDEX;
					SLOT->state = EG_DEC;
				}
			}
		break;

		case EG_DEC:	/* decay phase */
			if (SLOT->ssg&0x08)	/* SSG EG type envelope selected */
			{
				if ( !(OPN->eg_cnt & ((1<<SLOT->eg_sh_d1r)-1) ) )
				{
					//SLOT->volume += 4 * eg_inc[SLOT->eg_sel_d1r + ((OPN->eg_cnt>>SLOT->eg_sh_d1r)&7)];
					SLOT->volume += SSGEG_SCALE * eg_inc[SLOT->eg_sel_d1r + ((OPN->eg_cnt>>SLOT->eg_sh_d1r)&7)];

					if ( SLOT->volume >= (INT32)(SLOT->sl) )
						SLOT->state = EG_SUS;
				}
			}
			else
			{
				if ( !(OPN->eg_cnt & ((1<<SLOT->eg_sh_d1r)-1) ) )
				{
					SLOT->volume += eg_inc[SLOT->eg_sel_d1r + ((OPN->eg_cnt>>SLOT->eg_sh_d1r)&7)];

					if ( SLOT->volume >= (INT32)(SLOT->sl) )
						SLOT->state = EG_SUS;
				}
			}
		break;

		case EG_SUS:	/* sustain phase */
			if (SLOT->ssg&0x08)	/* SSG EG type envelope selected */
			{
				if ( !(OPN->eg_cnt & ((1<<SLOT->eg_sh_d2r)-1) ) )
				{
					//SLOT->volume += 4 * eg_inc[SLOT->eg_sel_d2r + ((OPN->eg_cnt>>SLOT->eg_sh_d2r)&7)];
					SLOT->volume += SSGEG_SCALE * eg_inc[SLOT->eg_sel_d2r + ((OPN->eg_cnt>>SLOT->eg_sh_d2r)&7)];

					if ( SLOT->volume >= 512 /* было MAX_ATT_INDEX */ ) //Alone Coder
					{
						SLOT->volume = MAX_ATT_INDEX;

						if (SLOT->ssg&0x01)	/* bit 0 = hold */
						{
							if (SLOT->ssgn&1)	/* have we swapped once ??? */
							{
								/* yes, so do nothing, just hold current level */
							}
							else
								swap_flag = (SLOT->ssg&0x02) | 1 ; /* bit 1 = alternate */

						}
						else
						{
							/* same as KEY-ON operation */

							/* restart of the Phase Generator should be here,
                                only if AR is not maximum ??? ALWAYS! */
							SLOT->phase = 0; //Alone Coder

							/* phase -> Attack */
						   SLOT->volume = 511; //Alone Coder
							SLOT->state = EG_ATT;

							swap_flag = (SLOT->ssg&0x02); /* bit 1 = alternate */
						}
					}
				}
			}
			else
			{
				if ( !(OPN->eg_cnt & ((1<<SLOT->eg_sh_d2r)-1) ) )
				{
					SLOT->volume += eg_inc[SLOT->eg_sel_d2r + ((OPN->eg_cnt>>SLOT->eg_sh_d2r)&7)];

					if ( SLOT->volume >= MAX_ATT_INDEX )
					{
						SLOT->volume = MAX_ATT_INDEX;
						/* do not change SLOT->state (verified on real chip) */
					}
				}

			}
		break;

		case EG_REL:	/* release phase */
				if ( !(OPN->eg_cnt & ((1<<SLOT->eg_sh_rr)-1) ) )
				{
					SLOT->volume += eg_inc[SLOT->eg_sel_rr + ((OPN->eg_cnt>>SLOT->eg_sh_rr)&7)];

					if ( SLOT->volume >= MAX_ATT_INDEX )
					{
						SLOT->volume = MAX_ATT_INDEX;
						SLOT->state = EG_OFF;
					}
				}
		break;

		}

		out = SLOT->tl + ((UINT32)SLOT->volume);

		if ((SLOT->ssg&0x08) && (SLOT->ssgn&2) && (SLOT->state != EG_OFF/*Alone Coder*/))	/* negate output (changes come from alternate bit, init comes from attack bit) */
			out ^= 511/*Alone Coder*/; //((1<<ENV_BITS)-1); /* 1023 */

		/* we need to store the result here because we are going to change ssgn
            in next instruction */
		SLOT->vol_out = out;

		SLOT->ssgn ^= swap_flag;

		SLOT++;
		i--;
	}while (i);

}



#define volume_calc(OP) ((OP)->vol_out + (AM & (OP)->AMmask))

inline void chan_calc(FM_STATE* state, FM_OPN *OPN, FM_CH *CH)
{
	unsigned int eg_out;

	UINT32 AM = state->LFO_AM >> CH->ams;


	state->m2 = state->c1 = state->c2 = state->mem = 0;

	*CH->mem_connect = CH->mem_value;	/* restore delayed sample (MEM) value to m2 or c2 */

	eg_out = volume_calc(&CH->SLOT[SLOT1]);
	{
		INT32 out = CH->op1_out[0] + CH->op1_out[1];
		CH->op1_out[0] = CH->op1_out[1];

		if( !CH->connect1 ){
			/* algorithm 5  */
			state->mem = state->c1 = state->c2 = CH->op1_out[0];
		}else{
			/* other algorithms */
			*CH->connect1 += CH->op1_out[0];
		}

		CH->op1_out[1] = 0;
		if( eg_out < ENV_QUIET )	/* SLOT 1 */
		{
			if (!CH->FB)
				out=0;

			CH->op1_out[1] = op_calc1(CH->SLOT[SLOT1].phase, eg_out, (out<<CH->FB) );
		}
	}

	eg_out = volume_calc(&CH->SLOT[SLOT3]);
	if( eg_out < ENV_QUIET )		/* SLOT 3 */
		*CH->connect3 += op_calc(CH->SLOT[SLOT3].phase, eg_out, state->m2);

	eg_out = volume_calc(&CH->SLOT[SLOT2]);
	if( eg_out < ENV_QUIET )		/* SLOT 2 */
		*CH->connect2 += op_calc(CH->SLOT[SLOT2].phase, eg_out, state->c1);

	eg_out = volume_calc(&CH->SLOT[SLOT4]);
	if( eg_out < ENV_QUIET )		/* SLOT 4 */
		*CH->connect4 += op_calc(CH->SLOT[SLOT4].phase, eg_out, state->c2);


	/* store current MEM */
	CH->mem_value = state->mem;

	/* update phase counters AFTER output calculations */
	if(CH->pms)
	{


	/* add support for 3 slot mode */


		UINT32 block_fnum = CH->block_fnum;

		UINT32 fnum_lfo   = ((block_fnum & 0x7f0) >> 4) * 32 * 8;
		INT32  lfo_fn_table_index_offset = lfo_pm_table[ fnum_lfo + CH->pms + state->LFO_PM ];

		if (lfo_fn_table_index_offset)	/* LFO phase modulation active */
		{
			UINT8  blk;
			UINT32 fn;
			int kc,fc;

			block_fnum = block_fnum*2 + lfo_fn_table_index_offset;

			blk = (block_fnum&0x7000) >> 12;
			fn  = block_fnum & 0xfff;

			/* keyscale code */
			kc = (blk<<2) | opn_fktable[fn >> 8];
 			/* phase increment counter */
			fc = OPN->fn_table[fn]>>(7-blk);

			CH->SLOT[SLOT1].phase += ((fc+CH->SLOT[SLOT1].DT[kc])*CH->SLOT[SLOT1].mul) >> 1;
			CH->SLOT[SLOT2].phase += ((fc+CH->SLOT[SLOT2].DT[kc])*CH->SLOT[SLOT2].mul) >> 1;
			CH->SLOT[SLOT3].phase += ((fc+CH->SLOT[SLOT3].DT[kc])*CH->SLOT[SLOT3].mul) >> 1;
			CH->SLOT[SLOT4].phase += ((fc+CH->SLOT[SLOT4].DT[kc])*CH->SLOT[SLOT4].mul) >> 1;
		}
		else	/* LFO phase modulation  = zero */
		{
			CH->SLOT[SLOT1].phase += CH->SLOT[SLOT1].Incr;
			CH->SLOT[SLOT2].phase += CH->SLOT[SLOT2].Incr;
			CH->SLOT[SLOT3].phase += CH->SLOT[SLOT3].Incr;
			CH->SLOT[SLOT4].phase += CH->SLOT[SLOT4].Incr;
		}
	}
	else	/* no LFO phase modulation */
	{
		CH->SLOT[SLOT1].phase += CH->SLOT[SLOT1].Incr;
		CH->SLOT[SLOT2].phase += CH->SLOT[SLOT2].Incr;
		CH->SLOT[SLOT3].phase += CH->SLOT[SLOT3].Incr;
		CH->SLOT[SLOT4].phase += CH->SLOT[SLOT4].Incr;
	}
}

/* update phase increment and envelope generator */
inline void refresh_fc_eg_slot(FM_SLOT *SLOT , int fc , int kc )
{
	int ksr;

	/* (frequency) phase increment counter */
	SLOT->Incr = ((fc+SLOT->DT[kc])*SLOT->mul) >> 1;

	ksr = kc >> SLOT->KSR;
	if( SLOT->ksr != ksr )
	{
		SLOT->ksr = ksr;

		/* calculate envelope generator rates */
		if ((SLOT->ar + SLOT->ksr) < 32+62)
		{
			SLOT->eg_sh_ar  = eg_rate_shift [SLOT->ar  + SLOT->ksr ];
			SLOT->eg_sel_ar = eg_rate_select[SLOT->ar  + SLOT->ksr ];
		}
		else
		{
			SLOT->eg_sh_ar  = 0;
			SLOT->eg_sel_ar = 17*RATE_STEPS;
		}

		SLOT->eg_sh_d1r = eg_rate_shift [SLOT->d1r + SLOT->ksr];
		SLOT->eg_sel_d1r= eg_rate_select[SLOT->d1r + SLOT->ksr];

		SLOT->eg_sh_d2r = eg_rate_shift [SLOT->d2r + SLOT->ksr];
		SLOT->eg_sel_d2r= eg_rate_select[SLOT->d2r + SLOT->ksr];

		SLOT->eg_sh_rr  = eg_rate_shift [SLOT->rr  + SLOT->ksr];
		SLOT->eg_sel_rr = eg_rate_select[SLOT->rr  + SLOT->ksr];
	}
}

/* update phase increment counters */
inline void refresh_fc_eg_chan(FM_CH *CH )
{
	if( CH->SLOT[SLOT1].Incr==-1){
		int fc = CH->fc;
		int kc = CH->kcode;
		refresh_fc_eg_slot(&CH->SLOT[SLOT1] , fc , kc );
		refresh_fc_eg_slot(&CH->SLOT[SLOT2] , fc , kc );
		refresh_fc_eg_slot(&CH->SLOT[SLOT3] , fc , kc );
		refresh_fc_eg_slot(&CH->SLOT[SLOT4] , fc , kc );
	}
}

/* initialize time tables */
static void init_timetables( FM_ST *ST , const UINT8 *dttable )
{
	int i,d;
	double rate;

#if 0
	logerror("FM.C: samplerate=%8i chip clock=%8i  freqbase=%f  \n",
			 ST->rate, ST->clock, ST->freqbase );
#endif

	/* DeTune table */
	for (d = 0;d <= 3;d++){
		for (i = 0;i <= 31;i++){
			rate = ((double)dttable[d*32 + i]) * SIN_LEN  * ST->freqbase  * (1<<FREQ_SH) / ((double)(1<<20));
			ST->dt_tab[d][i]   = (INT32) rate;
			ST->dt_tab[d+4][i] = -ST->dt_tab[d][i];
#if 0
			logerror("FM.C: DT [%2i %2i] = %8x  \n", d, i, ST->dt_tab[d][i] );
#endif
		}
	}

}


static void reset_channels( FM_ST *ST , FM_CH *CH , int num )
{
	int c,s;

	ST->mode   = 0;	/* normal mode */
	ST->TA     = 0;
	ST->TAC    = 0;
	ST->TB     = 0;
	ST->TBC    = 0;

	for( c = 0 ; c < num ; c++ )
	{
		CH[c].fc = 0;
		for(s = 0 ; s < 4 ; s++ )
		{
			CH[c].SLOT[s].ssg = 0;
			CH[c].SLOT[s].ssgn = 0;
			CH[c].SLOT[s].state= EG_OFF;
			CH[c].SLOT[s].volume = MAX_ATT_INDEX;
			CH[c].SLOT[s].vol_out= MAX_ATT_INDEX;
		}
	}
}

/* initialize generic tables */
static int init_tables(void)
{
	signed int i,x;
	signed int n;
	double o,m;

	for (x=0; x<TL_RES_LEN; x++)
	{
		m = (1<<16) / pow(2.0, (x+1) * (ENV_STEP/4.0) / 8.0);
		m = floor(m);

		/* we never reach (1<<16) here due to the (x+1) */
		/* result fits within 16 bits at maximum */

		n = (int)m;		/* 16 bits here */
		n >>= 4;		/* 12 bits here */
		if (n&1)		/* round to nearest */
			n = (n>>1)+1;
		else
			n = n>>1;
						/* 11 bits here (rounded) */
		n <<= 2;		/* 13 bits here (as in real chip) */
		tl_tab[ x*2 + 0 ] = n;
		tl_tab[ x*2 + 1 ] = -tl_tab[ x*2 + 0 ];

		for (i=1; i<13; i++)
		{
			tl_tab[ x*2+0 + i*2*TL_RES_LEN ] =  tl_tab[ x*2+0 ]>>i;
			tl_tab[ x*2+1 + i*2*TL_RES_LEN ] = -tl_tab[ x*2+0 + i*2*TL_RES_LEN ];
		}
	#if 0
			logerror("tl %04i", x);
			for (i=0; i<13; i++)
				logerror(", [%02i] %4x", i*2, tl_tab[ x*2 /*+1*/ + i*2*TL_RES_LEN ]);
			logerror("\n");
		}
	#endif
	}
	/*logerror("FM.C: TL_TAB_LEN = %i elements (%i bytes)\n",TL_TAB_LEN, (int)sizeof(tl_tab));*/


	for (i=0; i<SIN_LEN; i++)
	{
		/* non-standard sinus */
		m = sin( ((i*2)+1) * M_PI / SIN_LEN ); /* checked against the real chip */

		/* we never reach zero here due to ((i*2)+1) */

		if (m>0.0)
			o = 8*log(1.0/m)/log(2.0);	/* convert to 'decibels' */
		else
			o = 8*log(-1.0/m)/log(2.0);	/* convert to 'decibels' */

		o = o / (ENV_STEP/4);

		n = (int)(2.0*o);
		if (n&1)						/* round to nearest */
			n = (n>>1)+1;
		else
			n = n>>1;

		sin_tab[ i ] = n*2 + (m>=0.0? 0: 1 );
		/*logerror("FM.C: sin [%4i]= %4i (tl_tab value=%5i)\n", i, sin_tab[i],tl_tab[sin_tab[i]]);*/
	}

	/*logerror("FM.C: ENV_QUIET= %08x\n",ENV_QUIET );*/


	/* build LFO PM modulation table */
	for(i = 0; i < 8; i++) /* 8 PM depths */
	{
		UINT8 fnum;
		for (fnum=0; fnum<128; fnum++) /* 7 bits meaningful of F-NUMBER */
		{
			UINT8 value;
			UINT8 step;
			UINT32 offset_depth = i;
			UINT32 offset_fnum_bit;
			UINT32 bit_tmp;

			for (step=0; step<8; step++)
			{
				value = 0;
				for (bit_tmp=0; bit_tmp<7; bit_tmp++) /* 7 bits */
				{
					if (fnum & (1<<bit_tmp)) /* only if bit "bit_tmp" is set */
					{
						offset_fnum_bit = bit_tmp * 8;
						value += lfo_pm_output[offset_fnum_bit + offset_depth][step];
					}
				}
				lfo_pm_table[(fnum*32*8) + (i*32) + step   + 0] = value;
				lfo_pm_table[(fnum*32*8) + (i*32) +(step^7)+ 8] = value;
				lfo_pm_table[(fnum*32*8) + (i*32) + step   +16] = -value;
				lfo_pm_table[(fnum*32*8) + (i*32) +(step^7)+24] = -value;
			}
#if 0
			logerror("LFO depth=%1x FNUM=%04x (<<4=%4x): ", i, fnum, fnum<<4);
			for (step=0; step<16; step++) /* dump only positive part of waveforms */
				logerror("%02x ", lfo_pm_table[(fnum*32*8) + (i*32) + step] );
			logerror("\n");
#endif

		}
	}

	return 1;
}



static void FMCloseTable( void )
{
	return;
}


/* CSM Key Controll */
inline void CSMKeyControll(FM_CH *CH)
{
	/* this is wrong, atm */

	/* all key on */
	FM_KEYON(CH,SLOT1);
	FM_KEYON(CH,SLOT2);
	FM_KEYON(CH,SLOT3);
	FM_KEYON(CH,SLOT4);
}


//#if BUILD_OPN



/* prescaler set (and make time tables) */
static void OPNSetPres(FM_OPN *OPN , int pres , int TimerPres, int SSGpres)
{
	int i;

	/* frequency base */
	OPN->ST.freqbase = (OPN->ST.rate) ? ((double)OPN->ST.clock / OPN->ST.rate) / pres : 0;

#if 0
	OPN->ST.rate = (double)OPN->ST.clock / pres;
	OPN->ST.freqbase = 1.0;
#endif

	OPN->eg_timer_add  = (UINT32)((1<<EG_SH)  *  OPN->ST.freqbase);
	OPN->eg_timer_overflow = ( 3 ) * (1<<EG_SH);


	/* Timer base time */
	OPN->ST.TimerBase = 1.0/((double)OPN->ST.clock / (double)TimerPres);

	/* SSG part  prescaler set */
//	if( SSGpres ) (*OPN->ST.SSG->set_clock)( OPN->ST.param, OPN->ST.clock * 2 / SSGpres );

	/* make time tables */
	init_timetables( &OPN->ST, dt_tab );

	/* there are 2048 FNUMs that can be generated using FNUM/BLK registers
        but LFO works with one more bit of a precision so we really need 4096 elements */
	/* calculate fnumber -> increment counter table */
	for(i = 0; i < 4096; i++)
	{
		/* freq table for octave 7 */
		/* OPN phase increment counter = 20bit */
		OPN->fn_table[i] = (UINT32)( (double)i * 32 * OPN->ST.freqbase * (1<<(FREQ_SH-10)) ); /* -10 because chip works with 10.10 fixed point, while we use 16.16 */
#if 0
		logerror("FM.C: fn_table[%4i] = %08x (dec=%8i)\n",
				 i, OPN->fn_table[i]>>6,OPN->fn_table[i]>>6 );
#endif
	}

	/* LFO freq. table */
	for(i = 0; i < 8; i++)
	{
		/* Amplitude modulation: 64 output levels (triangle waveform); 1 level lasts for one of "lfo_samples_per_step" samples */
		/* Phase modulation: one entry from lfo_pm_output lasts for one of 4 * "lfo_samples_per_step" samples  */
		OPN->lfo_freq[i] = (UINT32)((1.0 / lfo_samples_per_step[i]) * (1<<LFO_SH) * OPN->ST.freqbase);
#if 0
		logerror("FM.C: lfo_freq[%i] = %08x (dec=%8i)\n",
				 i, OPN->lfo_freq[i],OPN->lfo_freq[i] );
#endif
	}
}



/* write a OPN mode register 0x20-0x2f */
static void OPNWriteMode(FM_OPN *OPN, int r, int v)
{
	UINT8 c;
	FM_CH *CH;

	switch(r){
	case 0x21:	/* Test */
		break;
	case 0x22:	/* LFO FREQ (YM2608/YM2610/YM2610B/YM2612) */
		if( OPN->type & TYPE_LFOPAN )
		{
			if (v&0x08) /* LFO enabled ? */
			{
				OPN->lfo_inc = OPN->lfo_freq[v&7];
			}
			else
			{
				OPN->lfo_inc = 0;
			}
		}
		break;
	case 0x24:	/* timer A High 8*/
		OPN->ST.TA = (OPN->ST.TA & 0x03)|(((int)v)<<2);
		break;
	case 0x25:	/* timer A Low 2*/
		OPN->ST.TA = (OPN->ST.TA & 0x3fc)|(v&3);
		break;
	case 0x26:	/* timer B */
		OPN->ST.TB = v;
		break;
	case 0x27:	/* mode, timer control */
		set_timers( &(OPN->ST),OPN->ST.param,v );
		break;
	case 0x28:	/* key on / off */
		c = v & 0x03;
		if( c == 3 ) break;
		if( (v&0x04) && (OPN->type & TYPE_6CH) ) c+=3;
		CH = OPN->P_CH;
		CH = &CH[c];
		if(v&0x10) FM_KEYON(CH,SLOT1); else FM_KEYOFF(CH,SLOT1);
		if(v&0x20) FM_KEYON(CH,SLOT2); else FM_KEYOFF(CH,SLOT2);
		if(v&0x40) FM_KEYON(CH,SLOT3); else FM_KEYOFF(CH,SLOT3);
		if(v&0x80) FM_KEYON(CH,SLOT4); else FM_KEYOFF(CH,SLOT4);
		break;
	}
}

/* write a OPN register (0x30-0xff) */
static void OPNWriteReg(FM_STATE* state, FM_OPN *OPN, int r, int v)
{
	FM_CH *CH;
	FM_SLOT *SLOT;

	UINT8 c = OPN_CHAN(r);

	if (c == 3) return; /* 0xX3,0xX7,0xXB,0xXF */

	if (r >= 0x100) c+=3;

	CH = OPN->P_CH;
	CH = &CH[c];

	SLOT = &(CH->SLOT[OPN_SLOT(r)]);

	switch( r & 0xf0 ) {
	case 0x30:	/* DET , MUL */
		set_det_mul(&OPN->ST,CH,SLOT,v);
		break;

	case 0x40:	/* TL */
		set_tl(CH,SLOT,v);
		break;

	case 0x50:	/* KS, AR */
		set_ar_ksr(CH,SLOT,v);
		break;

	case 0x60:	/* bit7 = AM ENABLE, DR */
		set_dr(SLOT,v);

		if(OPN->type & TYPE_LFOPAN) /* YM2608/2610/2610B/2612 */
		{
			SLOT->AMmask = (v&0x80) ? ~0 : 0;
		}
		break;

	case 0x70:	/*     SR */
		set_sr(SLOT,v);
		break;

	case 0x80:	/* SL, RR */
		set_sl_rr(SLOT,v);
		break;

	case 0x90:	/* SSG-EG */

		SLOT->ssg  =  v&0x0f;
		SLOT->ssgn = (v&0x04)>>1; /* bit 1 in ssgn = attack */

		/* SSG-EG envelope shapes :

        E AtAlH
        1 0 0 0  \\\\

        1 0 0 1  \___

        1 0 1 0  \/\/
                  ___
        1 0 1 1  \

        1 1 0 0  ////
                  ___
        1 1 0 1  /

        1 1 1 0  /\/\

        1 1 1 1  /___


        E = SSG-EG enable


        The shapes are generated using Attack, Decay and Sustain phases.

        Each single character in the diagrams above represents this whole
        sequence:

        - when KEY-ON = 1, normal Attack phase is generated (*without* any
          difference when compared to normal mode),

        - later, when envelope level reaches minimum level (max volume),
          the EG switches to Decay phase (which works with bigger steps
          when compared to normal mode - see below),

        - later when envelope level passes the SL level,
          the EG swithes to Sustain phase (which works with bigger steps
          when compared to normal mode - see below),

        - finally when envelope level reaches maximum level (min volume),
          the EG switches to Attack phase again (depends on actual waveform).

        Important is that when switch to Attack phase occurs, the phase counter
        of that operator will be zeroed-out (as in normal KEY-ON) but not always.
        (I havent found the rule for that - perhaps only when the output level is low)

        The difference (when compared to normal Envelope Generator mode) is
        that the resolution in Decay and Sustain phases is 4 times lower;
        this results in only 256 steps instead of normal 1024.
        In other words:
        when SSG-EG is disabled, the step inside of the EG is one,
        when SSG-EG is enabled, the step is four (in Decay and Sustain phases).

        Times between the level changes are the same in both modes.


        Important:
        Decay 1 Level (so called SL) is compared to actual SSG-EG output, so
        it is the same in both SSG and no-SSG modes, with this exception:

        when the SSG-EG is enabled and is generating raising levels
        (when the EG output is inverted) the SL will be found at wrong level !!!
        For example, when SL=02:
            0 -6 = -6dB in non-inverted EG output
            96-6 = -90dB in inverted EG output
        Which means that EG compares its level to SL as usual, and that the
        output is simply inverted afterall.


        The Yamaha's manuals say that AR should be set to 0x1f (max speed).
        That is not necessary, but then EG will be generating Attack phase.

        */


		break;

	case 0xa0:
		switch( OPN_SLOT(r) ){
		case 0:		/* 0xa0-0xa2 : FNUM1 */
			{
				UINT32 fn = (((UINT32)( (OPN->ST.fn_h)&7))<<8) + v;
				UINT8 blk = OPN->ST.fn_h>>3;
				/* keyscale code */
				CH->kcode = (blk<<2) | opn_fktable[fn >> 7];
				/* phase increment counter */
				CH->fc = OPN->fn_table[fn*2]>>(7-blk);

				/* store fnum in clear form for LFO PM calculations */
				CH->block_fnum = (blk<<11) | fn;

				CH->SLOT[SLOT1].Incr=-1;
			}
			break;
		case 1:		/* 0xa4-0xa6 : FNUM2,BLK */
			OPN->ST.fn_h = v&0x3f;
			break;
		case 2:		/* 0xa8-0xaa : 3CH FNUM1 */
			if(r < 0x100)
			{
				UINT32 fn = (((UINT32)(OPN->SL3.fn_h&7))<<8) + v;
				UINT8 blk = OPN->SL3.fn_h>>3;
				/* keyscale code */
				OPN->SL3.kcode[c]= (blk<<2) | opn_fktable[fn >> 7];
				/* phase increment counter */
				OPN->SL3.fc[c] = OPN->fn_table[fn*2]>>(7-blk);
				OPN->SL3.block_fnum[c] = fn;
				(OPN->P_CH)[2].SLOT[SLOT1].Incr=-1;
			}
			break;
		case 3:		/* 0xac-0xae : 3CH FNUM2,BLK */
			if(r < 0x100)
				OPN->SL3.fn_h = v&0x3f;
			break;
		}
		break;

	case 0xb0:
		switch( OPN_SLOT(r) ){
		case 0:		/* 0xb0-0xb2 : FB,ALGO */
			{
				int feedback = (v>>3)&7;
				CH->ALGO = v&7;
				CH->FB   = feedback ? feedback+6 : 0;
				setup_connection(state, CH, c );
			}
			break;
		case 1:		/* 0xb4-0xb6 : L , R , AMS , PMS (YM2612/YM2610B/YM2610/YM2608) */
			if( OPN->type & TYPE_LFOPAN)
			{
				/* b0-2 PMS */
				CH->pms = (v & 7) * 32; /* CH->pms = PM depth * 32 (index in lfo_pm_table) */

				/* b4-5 AMS */
				CH->ams = lfo_ams_depth_shift[(v>>4) & 0x03];

				/* PAN :  b7 = L, b6 = R */
				OPN->pan[ c*2   ] = (v & 0x80) ? ~0 : 0;
				OPN->pan[ c*2+1 ] = (v & 0x40) ? ~0 : 0;

			}
			break;
		}
		break;
	}
}

//#endif /* BUILD_OPN */

//#if BUILD_OPN_PRESCALER
/*
  prescaler circuit (best guess to verified chip behaviour)

               +--------------+  +-sel2-+
               |              +--|in20  |
         +---+ |  +-sel1-+       |      |
M-CLK -+-|1/2|-+--|in10  | +---+ |   out|--INT_CLOCK
       | +---+    |   out|-|1/3|-|in21  |
       +----------|in11  | +---+ +------+
                  +------+

reg.2d : sel2 = in21 (select sel2)
reg.2e : sel1 = in11 (select sel1)
reg.2f : sel1 = in10 , sel2 = in20 (clear selector)
reset  : sel1 = in11 , sel2 = in21 (clear both)

*/
void OPNPrescaler_w(FM_OPN *OPN , int addr, int pre_divider)
{
	static const int opn_pres[4] = { 2*12 , 2*12 , 6*12 , 3*12 };
	static const int ssg_pres[4] = { 1    ,    1 ,    4 ,    2 };
	int sel;

	switch(addr)
	{
	case 0:		/* when reset */
		OPN->ST.prescaler_sel = 2;
		break;
	case 1:		/* when postload */
		break;
	case 0x2d:	/* divider sel : select 1/1 for 1/3line    */
		OPN->ST.prescaler_sel |= 0x02;
		break;
	case 0x2e:	/* divider sel , select 1/3line for output */
		OPN->ST.prescaler_sel |= 0x01;
		break;
	case 0x2f:	/* divider sel , clear both selector to 1/2,1/2 */
		OPN->ST.prescaler_sel = 0;
		break;
	}
	sel = OPN->ST.prescaler_sel & 3;
	/* update prescaler */
	OPNSetPres( OPN,	opn_pres[sel]*pre_divider,
						opn_pres[sel]*pre_divider,
						ssg_pres[sel]*pre_divider );
}
//#endif /* BUILD_OPN_PRESCALER */

//#if BUILD_YM2203
/*****************************************************************************/
/*      YM2203 local section                                                 */
/*****************************************************************************/

/* here's the virtual YM2203(OPN) */
typedef struct
{
	UINT8 REGS[256];		/* registers         */
	FM_STATE State;
	FM_OPN OPN;				/* OPN state         */
	FM_CH CH[3];			/* channel state     */
	int mute;				//маска заглушения каналов
} YM2203;

/* Generate samples for one of the YM2203s */
void YM2203UpdateOne(void *chip, short *buffer, int length)
{
	YM2203 *F2203 = (YM2203*)chip;
	FM_OPN *OPN =   &F2203->OPN;
	FM_STATE *state = &F2203->State;
	int i;
	short *buf = buffer;
	FM_CH	*cch[3];

	cch[0]   = &F2203->CH[0];
	cch[1]   = &F2203->CH[1];
	cch[2]   = &F2203->CH[2];

	/* refresh PG and EG */
	refresh_fc_eg_chan( cch[0] );
	refresh_fc_eg_chan( cch[1] );
	if( (F2203->OPN.ST.mode & 0xc0) )
	{
		/* 3SLOT MODE */
		if( cch[2]->SLOT[SLOT1].Incr==-1)
		{
			refresh_fc_eg_slot(&cch[2]->SLOT[SLOT1] , OPN->SL3.fc[1] , OPN->SL3.kcode[1] );
			refresh_fc_eg_slot(&cch[2]->SLOT[SLOT2] , OPN->SL3.fc[2] , OPN->SL3.kcode[2] );
			refresh_fc_eg_slot(&cch[2]->SLOT[SLOT3] , OPN->SL3.fc[0] , OPN->SL3.kcode[0] );
			refresh_fc_eg_slot(&cch[2]->SLOT[SLOT4] , cch[2]->fc , cch[2]->kcode );
		}
	}else refresh_fc_eg_chan( cch[2] );


	/* YM2203 doesn't have LFO so we must keep these globals at 0 level */
	state->LFO_AM = 0;
	state->LFO_PM = 0;

	/* buffering */
	for (i=0; i < length ; i++)
	{
		/* clear outputs */
		state->out_fm[0] = 0;
		state->out_fm[1] = 0;
		state->out_fm[2] = 0;

		/* advance envelope generator */
		OPN->eg_timer += OPN->eg_timer_add;
		while (OPN->eg_timer >= OPN->eg_timer_overflow)
		{
			OPN->eg_timer -= OPN->eg_timer_overflow;
			OPN->eg_cnt++;

			advance_eg_channel(OPN, &cch[0]->SLOT[SLOT1]);
			advance_eg_channel(OPN, &cch[1]->SLOT[SLOT1]);
			advance_eg_channel(OPN, &cch[2]->SLOT[SLOT1]);
		}

		/* calculate FM */
		chan_calc(state, OPN, cch[0] );
		chan_calc(state, OPN, cch[1] );
		chan_calc(state, OPN, cch[2] );

		/* buffering */
		{
			int lt;

			//lt = out_fm[0] + out_fm[1] + out_fm[2];
			lt=0;
			if(F2203->mute&0x01) lt+=state->out_fm[0];
			if(F2203->mute&0x02) lt+=state->out_fm[1];
			if(F2203->mute&0x04) lt+=state->out_fm[2];

			lt >>= FINAL_SH;

			Limit( lt , MAXOUT, MINOUT );

			/* buffering */
			buf[i] += lt;
		}

		/* timer A control */
		INTERNAL_TIMER_A( &F2203->OPN.ST , cch[2] )
	}
	INTERNAL_TIMER_B(&F2203->OPN.ST,length)
}

/* ---------- reset one of chip ---------- */
void YM2203ResetChip(void *chip)
{
	int i;
	YM2203 *F2203 = (YM2203*)chip;
	FM_OPN *OPN = &F2203->OPN;
	FM_STATE *state = &F2203->State;

	/* Reset Prescaler */
	OPNPrescaler_w(OPN, 0 , 1 );
	/* reset SSG section */
//	(*OPN->ST.SSG->reset)(OPN->ST.param);
	/* status clear */
	FM_IRQMASK_SET(&OPN->ST,0x03);
	FM_BUSY_CLEAR(&OPN->ST);
	OPNWriteMode(OPN,0x27,0x30); /* mode 0 , timer reset */

	OPN->eg_timer = 0;
	OPN->eg_cnt   = 0;

	FM_STATUS_RESET(&OPN->ST, 0xff);

	reset_channels( &OPN->ST , F2203->CH , 3 );
	/* reset OPerator paramater */
	for(i = 0xb2 ; i >= 0x30 ; i-- ) OPNWriteReg(state, OPN,i,0);
	for(i = 0x26 ; i >= 0x20 ; i-- ) OPNWriteReg(state, OPN,i,0);
}


/* ----------  Initialize YM2203 emulator(s) ----------
   'num' is the number of virtual YM2203s to allocate
   'clock' is the chip clock in Hz
   'rate' is sampling rate
*/
void * YM2203Init(UINT64 clock, int rate)
{
	YM2203 *F2203;

	/* allocate ym2203 state space */
	if( (F2203 = (YM2203 *)malloc(sizeof(YM2203)))==NULL)
		return NULL;
	/* clear */
	memset(F2203,0,sizeof(YM2203));

	if( !init_tables() )
	{
		free( F2203 );
		return NULL;
	}

	F2203->OPN.ST.param =0;
	F2203->OPN.type = 0;//TYPE_YM2203;
	F2203->OPN.P_CH = F2203->CH;
	F2203->OPN.ST.clock = clock;
	F2203->OPN.ST.rate = rate;
	F2203->mute=0x0f;

	//F2203->OPN.ST.Timer_Handler = TimerHandler;
	//F2203->OPN.ST.IRQ_Handler   = IRQHandler;
	//F2203->OPN.ST.SSG           = ssg;
	YM2203ResetChip(F2203);

	return F2203;
}

/* shut down emulator */
void YM2203Shutdown(void *chip)
{
	YM2203 *FM2203 = (YM2203*)chip;

	FMCloseTable();
	free(FM2203);
}

/* YM2203 I/O interface */
int YM2203Write(void *chip,int a,UINT8 v)
{
	YM2203 *F2203 = (YM2203*)chip;
	FM_OPN *OPN = &F2203->OPN;
	FM_STATE *state = &F2203->State;

	if( !(a&1) )
	{	/* address port */
		OPN->ST.address = (v &= 0xff);

		/* Write register to SSG emulator */
		//if( v < 16 ) (*OPN->ST.SSG->write)(OPN->ST.param,0,v);

		/* prescaler select : 2d,2e,2f  */
		if( v >= 0x2d && v <= 0x2f )
			OPNPrescaler_w(OPN , v , 1);
	}
	else
	{	/* data port */
		int addr = OPN->ST.address;
		F2203->REGS[addr] = v;
		switch( addr & 0xf0 )
		{
		case 0x00:	/* 0x00-0x0f : SSG section */
			/* Write data to SSG emulator */
			//(*OPN->ST.SSG->write)(OPN->ST.param,a,v);
			break;
		case 0x20:	/* 0x20-0x2f : Mode section */
	//		YM2203UpdateReq(OPN->ST.param);
			/* write register */
			OPNWriteMode(OPN,addr,v);
			break;
		default:	/* 0x30-0xff : OPN section */
	//		YM2203UpdateReq(OPN->ST.param);
			/* write register */
			OPNWriteReg(state, OPN,addr,v);
		}
		FM_BUSY_SET(&OPN->ST,1);
	}
	return OPN->ST.irq;
}



int YM2203TimerOver(void *chip,int c)
{
	YM2203 *F2203 = (YM2203*)chip;

	if( c )
	{	/* Timer B */
		TimerBOver( &(F2203->OPN.ST) );
	}
	else
	{	/* Timer A */
	//	YM2203UpdateReq(F2203->OPN.ST.param);
		/* timer update */
		TimerAOver( &(F2203->OPN.ST) );
		/* CSM mode key,TL control */
		if( F2203->OPN.ST.mode & 0x80 )
		{	/* CSM mode auto key on */
			CSMKeyControll( &(F2203->CH[2]) );
		}
	}
	return F2203->OPN.ST.irq;
}


void YM2203SetMute(void *chip,int mask)
{
	YM2203 *F2203=(YM2203*)chip;
	F2203->mute=mask;
}

int GetFreq(UINT64 clock, UINT8 regHi, UINT8 regLo, unsigned scale)
{
  const unsigned counter = (unsigned(regHi & 7) << 8) | regLo;
  const unsigned octave = (regHi & 0x38) >> 3;
  return clock * 100 * counter * (1 << octave) / (scale << 21);
}

void YM2203GetAllTL(void *chip,int *levels, int* freqs)
{
	const int slotMap[8]={ 0x08,0x08,0x08,0x08,0x0c,0x0e,0x0e,0x0f };
	const int opn_pres[4] = { 2*12 , 2*12 , 6*12 , 3*12 };
	int aa,vol,algo,div;
	YM2203 *F2203;
	
	F2203=(YM2203*)chip;
	
  const unsigned scale = opn_pres[F2203->OPN.ST.prescaler_sel & 3];
	for(aa=0;aa<3;aa++)
	{
		algo=slotMap[F2203->CH[aa].ALGO&7];
		vol=0;
		div=0;

    const int freq = GetFreq(F2203->OPN.ST.clock, F2203->REGS[0xa4 + aa], F2203->REGS[0xa0 + aa], scale);
		if(algo&1)
		{
		  vol+=F2203->CH[aa].SLOT[SLOT1].vol_out; div++;
		}
		if(algo&2)
		{
		  vol+=F2203->CH[aa].SLOT[SLOT2].vol_out; div++;
		}
		if(algo&4)
		{
		  vol+=F2203->CH[aa].SLOT[SLOT3].vol_out; div++;
		}
		if(algo&8)
		{
		  vol+=F2203->CH[aa].SLOT[SLOT4].vol_out; div++;
		}

		levels[aa]=vol/div;
		freqs[aa] = freq;
	}
}