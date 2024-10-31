/*

  Originally taken from MAME sources
  Extracted and adapted by Shiru
  Copyright (C) 2001, 2002, 2003 Jarek Burczynski (bujar at mame dot net)
  Copyright (C) 1998 Tatsuyuki Satoh , MultiArcadeMachineEmulator development

  Get rid of all but YM2203 emulation related
*/


#define YM2610B_WARNING

/* this is not part of the C/C++ standards and is not present on */
/* strict ANSI compilers or when compiling under GCC with -ansi */
#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#include "Ym2203_Emu.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>


#define FREQ_SH			16  /* 16.16 fixed point (frequency calculations) */
#define EG_SH			16  /* 16.16 fixed point (envelope generator timing) */
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
#define SC(db) (uint32_t) ( db * (4.0/ENV_STEP) )
static const uint32_t sl_table[16]={
 SC( 0),SC( 1),SC( 2),SC(3 ),SC(4 ),SC(5 ),SC(6 ),SC( 7),
 SC( 8),SC( 9),SC(10),SC(11),SC(12),SC(13),SC(14),SC(31)
};
#undef SC


#define RATE_STEPS (8)
static const uint8_t eg_inc[19*RATE_STEPS]={

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
static const uint_t eg_rate_select[32+64+32]={	/* Envelope Generator rates (32 + 64 rates + 32 RKS) */
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
static const uint_t eg_rate_shift[32+64+32]={	/* Envelope Generator counter shifts (32 + 64 rates + 32 RKS) */
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

static const uint_t dt_tab[4 * 32]={
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
static const uint_t opn_fktable[16] = {0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3};

/* register number to channel number , slot offset */
#define OPN_CHAN(N) (N&3)
#define OPN_SLOT(N) ((N>>2)&3)

/* slot number */
#define SLOT1 0
#define SLOT2 2
#define SLOT3 1
#define SLOT4 3

/* struct describing a single operator (SLOT) */
using FM_SLOT = struct
{
  int32_t* DT;  /* detune          :dt_tab[DT] */
  uint_t KSR;   /* key scale rate  :3-KSR */
  uint32_t ar;  /* attack rate  */
  uint32_t d1r; /* decay rate   */
  uint32_t d2r; /* sustain rate */
  uint32_t rr;  /* release rate */
  uint_t ksr;   /* key scale rate  :kcode>>(3-KSR) */
  uint32_t mul; /* multiple        :ML_TABLE[ML] */

  /* Phase Generator */
  uint32_t phase; /* phase counter */
  uint32_t Incr;  /* phase step */

  /* Envelope Generator */
  uint_t state;     /* phase type */
  uint32_t tl;      /* total level: TL << 3 */
  int32_t volume;   /* envelope counter */
  uint32_t sl;      /* sustain level:sl_table[SL] */
  uint32_t vol_out; /* current output from EG circuit (without AM from LFO) */

  uint_t eg_sh_ar;   /*  (attack state) */
  uint_t eg_sel_ar;  /*  (attack state) */
  uint_t eg_sh_d1r;  /*  (decay state) */
  uint_t eg_sel_d1r; /*  (decay state) */
  uint_t eg_sh_d2r;  /*  (sustain state) */
  uint_t eg_sel_d2r; /*  (sustain state) */
  uint_t eg_sh_rr;   /*  (release state) */
  uint_t eg_sel_rr;  /*  (release state) */

  uint_t ssg;  /* SSG-EG waveform */
  uint_t ssgn; /* SSG-EG negated output */

  uint32_t key; /* 0=last key was KEY OFF, 1=KEY ON */
};

using FM_CH = struct
{
  FM_SLOT SLOT[4]; /* four SLOTs (operators) */

  uint_t ALGO;        /* algorithm */
  uint_t FB;          /* feedback shift */
  int32_t op1_out[2]; /* op1 output for feedback */

  int32_t* connect1; /* SLOT1 output pointer */
  int32_t* connect3; /* SLOT3 output pointer */
  int32_t* connect2; /* SLOT2 output pointer */
  int32_t* connect4; /* SLOT4 output pointer */

  int32_t* mem_connect; /* where to put the delayed sample (MEM) */
  int32_t mem_value;    /* delayed sample (MEM) value */

  uint32_t fc;  /* fnum,blk:adjusted to sample rate */
  uint_t kcode; /* key code:                        */
};

using FM_ST = struct
{
  uint64_t clock;       /* master clock  (Hz)   */
  int rate;             /* sampling rate (Hz)   */
  double freqbase;      /* frequency base       */
  uint32_t mode;        /* mode  CSM / 3SLOT    */
  uint_t prescaler_sel; /* prescaler selector */
  uint_t fn_h;          /* freq latch           */
  /* local time tables */
  int32_t dt_tab[8][32]; /* DeTune table       */
};

/***********************************************************/
/* OPN unit                                                */
/***********************************************************/

/* OPN 3slot struct */
using FM_3SLOT = struct
{
  uint32_t fc[3];  /* fnum3,blk3: calculated */
  uint_t fn_h;     /* freq3 latch */
  uint_t kcode[3]; /* key code */
};

/* OPN/A/B common state */
using FM_OPN = struct
{
  FM_ST ST;     /* general state */
  FM_3SLOT SL3; /* 3 slot mode state */
  FM_CH* P_CH;  /* pointer of CH */

  uint32_t eg_cnt;            /* global envelope generator counter */
  uint32_t eg_timer;          /* global envelope generator counter works at frequency = chipclock/64/3 */
  uint32_t eg_timer_add;      /* step of eg_timer */
  uint32_t eg_timer_overflow; /* envelope generator timer overlfows every 3 samples (on real chip) */

  /* there are 2048 FNUMs that can be generated using FNUM/BLK registers
  but LFO works with one more bit of a precision so we really need 4096 elements */

  uint32_t fn_table[4096]; /* fnumber->increment counter */
};

/* current chip state */
using FM_STATE = struct
{
  int32_t m2, c1, c2; /* Phase Modulation input for operators 2,3,4 */
  int32_t mem;        /* one sample delay memory */

  int32_t out_fm[8]; /* outputs of working channels */
};

/* OPN Mode Register Write */
inline void set_timers( FM_ST *ST, int v )
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
}

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
	int32_t *carrier = &state->out_fm[ch];

	int32_t **om1 = &CH->connect1;
	int32_t **om2 = &CH->connect3;
	int32_t **oc1 = &CH->connect2;

	int32_t **memc = &CH->mem_connect;

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
inline void set_tl(FM_SLOT *SLOT , int v)
{
	SLOT->tl = (v&0x7f)<<(ENV_BITS-7); /* 7bit TL */
}

/* set attack rate & key scale  */
inline void set_ar_ksr(FM_CH *CH,FM_SLOT *SLOT,int v)
{
	uint_t old_KSR = SLOT->KSR;

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



inline signed int op_calc(uint32_t phase, unsigned int env, signed int pm)
{
	uint32_t p;

	p = (env<<3) + sin_tab[ ( ((signed int)((phase & ~FREQ_MASK) + (pm<<15))) >> FREQ_SH ) & SIN_MASK ];

	if (p >= TL_TAB_LEN)
		return 0;
	return tl_tab[p];
}

inline signed int op_calc1(uint32_t phase, unsigned int env, signed int pm)
{
	uint32_t p;

	p = (env<<3) + sin_tab[ ( ((signed int)((phase & ~FREQ_MASK) + pm      )) >> FREQ_SH ) & SIN_MASK ];

	if (p >= TL_TAB_LEN)
		return 0;
	return tl_tab[p];
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

					if ( SLOT->volume >= (int32_t)(SLOT->sl) )
						SLOT->state = EG_SUS;
				}
			}
			else
			{
				if ( !(OPN->eg_cnt & ((1<<SLOT->eg_sh_d1r)-1) ) )
				{
					SLOT->volume += eg_inc[SLOT->eg_sel_d1r + ((OPN->eg_cnt>>SLOT->eg_sh_d1r)&7)];

					if ( SLOT->volume >= (int32_t)(SLOT->sl) )
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

		out = SLOT->tl + ((uint32_t)SLOT->volume);

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



#define volume_calc(OP) ((OP)->vol_out)

inline void chan_calc(FM_STATE* state, FM_CH *CH)
{
	unsigned int eg_out;

	state->m2 = state->c1 = state->c2 = state->mem = 0;

	*CH->mem_connect = CH->mem_value;	/* restore delayed sample (MEM) value to m2 or c2 */

	eg_out = volume_calc(&CH->SLOT[SLOT1]);
	{
		int32_t out = CH->op1_out[0] + CH->op1_out[1];
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

	CH->SLOT[SLOT1].phase += CH->SLOT[SLOT1].Incr;
	CH->SLOT[SLOT2].phase += CH->SLOT[SLOT2].Incr;
	CH->SLOT[SLOT3].phase += CH->SLOT[SLOT3].Incr;
	CH->SLOT[SLOT4].phase += CH->SLOT[SLOT4].Incr;
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
static void init_timetables( FM_ST *ST)
{
	int i,d;
	double rate;

	/* DeTune table */
	for (d = 0;d <= 3;d++){
		for (i = 0;i <= 31;i++){
			rate = ((double)dt_tab[d*32 + i]) * SIN_LEN  * ST->freqbase  * (1<<FREQ_SH) / ((double)(1<<20));
			ST->dt_tab[d][i]   = (int32_t) rate;
			ST->dt_tab[d+4][i] = -ST->dt_tab[d][i];
		}
	}

}


static void reset_channels( FM_ST *ST , FM_CH *CH , int num )
{
	int c,s;

	ST->mode   = 0;	/* normal mode */

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
  //TODO: fix
  static bool initialized = false;

  if (initialized)
  {
    return 1;
  }

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
	}


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
	}
	initialized = true;
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
static void OPNSetPres(FM_OPN *OPN , int pres)
{
	int i;

	/* frequency base */
	OPN->ST.freqbase = (OPN->ST.rate) ? ((double)OPN->ST.clock / OPN->ST.rate) / pres : 0;

	OPN->eg_timer_add  = (uint32_t)((1<<EG_SH)  *  OPN->ST.freqbase);
	OPN->eg_timer_overflow = ( 3 ) * (1<<EG_SH);

	/* make time tables */
	init_timetables( &OPN->ST);

	/* there are 2048 FNUMs that can be generated using FNUM/BLK registers
        but LFO works with one more bit of a precision so we really need 4096 elements */
	/* calculate fnumber -> increment counter table */
	for(i = 0; i < 4096; i++)
	{
		/* freq table for octave 7 */
		/* OPN phase increment counter = 20bit */
		OPN->fn_table[i] = (uint32_t)( (double)i * 32 * OPN->ST.freqbase * (1<<(FREQ_SH-10)) ); /* -10 because chip works with 10.10 fixed point, while we use 16.16 */
	}
}



/* write a OPN mode register 0x20-0x2f */
static void OPNWriteMode(FM_OPN *OPN, int r, int v)
{
	uint_t c;
	FM_CH *CH;

	switch(r){
	case 0x21:	/* Test */
		break;
	case 0x27:	/* mode, timer control */
		set_timers( &(OPN->ST), v);
		break;
	case 0x28:	/* key on / off */
		c = v & 0x03;
		if( c == 3 ) break;
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

	const uint_t c = OPN_CHAN(r);

	if (c == 3) return; /* 0xX3,0xX7,0xXB,0xXF */

	CH = OPN->P_CH;
	CH = &CH[c];

	SLOT = &(CH->SLOT[OPN_SLOT(r)]);

	switch( r & 0xf0 ) {
	case 0x30:	/* DET , MUL */
		set_det_mul(&OPN->ST,CH,SLOT,v);
		break;

	case 0x40:	/* TL */
		set_tl(SLOT,v);
		break;

	case 0x50:	/* KS, AR */
		set_ar_ksr(CH,SLOT,v);
		break;

	case 0x60:	/* bit7 = AM ENABLE, DR */
		set_dr(SLOT,v);
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
				const uint32_t fn = (((uint32_t)( (OPN->ST.fn_h)&7))<<8) + v;
				const uint_t blk = OPN->ST.fn_h>>3;
				/* keyscale code */
				CH->kcode = (blk<<2) | opn_fktable[fn >> 7];
				/* phase increment counter */
				CH->fc = OPN->fn_table[fn*2]>>(7-blk);

				CH->SLOT[SLOT1].Incr=-1;
			}
			break;
		case 1:		/* 0xa4-0xa6 : FNUM2,BLK */
			OPN->ST.fn_h = v&0x3f;
			break;
		case 2:		/* 0xa8-0xaa : 3CH FNUM1 */
			if(r < 0x100)
			{
				const uint32_t fn = (((uint32_t)(OPN->SL3.fn_h&7))<<8) + v;
				const uint_t blk = OPN->SL3.fn_h>>3;
				/* keyscale code */
				OPN->SL3.kcode[c]= (blk<<2) | opn_fktable[fn >> 7];
				/* phase increment counter */
				OPN->SL3.fc[c] = OPN->fn_table[fn*2]>>(7-blk);
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
		}
		break;
	}
}

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
	OPNSetPres( OPN,	opn_pres[sel]*pre_divider);
}

/* here's the virtual YM2203(OPN) */
using YM2203 = struct
{
  uint8_t REGS[256]; /* registers         */
  FM_STATE State;
  FM_OPN OPN;  /* OPN state         */
  FM_CH CH[3]; /* channel state     */
};

/* Generate samples for one of the YM2203s */
void YM2203UpdateOne(void *chip, int32_t *buffer, int length)
{
  auto* F2203 = (YM2203*)chip;
  FM_OPN* OPN = &F2203->OPN;
  FM_STATE* state = &F2203->State;
  FM_CH* cch[3];

  cch[0] = &F2203->CH[0];
  cch[1] = &F2203->CH[1];
  cch[2] = &F2203->CH[2];

  /* refresh PG and EG */
  refresh_fc_eg_chan(cch[0]);
  refresh_fc_eg_chan(cch[1]);
  if ((F2203->OPN.ST.mode & 0xc0))
  {
    /* 3SLOT MODE */
    if (cch[2]->SLOT[SLOT1].Incr == -1)
    {
      refresh_fc_eg_slot(&cch[2]->SLOT[SLOT1], OPN->SL3.fc[1], OPN->SL3.kcode[1]);
      refresh_fc_eg_slot(&cch[2]->SLOT[SLOT2], OPN->SL3.fc[2], OPN->SL3.kcode[2]);
      refresh_fc_eg_slot(&cch[2]->SLOT[SLOT3], OPN->SL3.fc[0], OPN->SL3.kcode[0]);
      refresh_fc_eg_slot(&cch[2]->SLOT[SLOT4], cch[2]->fc, cch[2]->kcode);
    }
	}else refresh_fc_eg_chan( cch[2] );

	/* buffering */
	for (int32_t* buf = buffer, *lim = buffer + length; buf != lim; ++buf)
	{
	  state->out_fm[0] = state->out_fm[1] = state->out_fm[2] = 0;

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
		chan_calc(state, cch[0] );
		chan_calc(state, cch[1] );
		chan_calc(state, cch[2] );

		*buf += state->out_fm[0] + state->out_fm[1] + state->out_fm[2];
	}
}

/* ---------- reset one of chip ---------- */
void YM2203ResetChip(void *chip)
{
	int i;
        auto* F2203 = (YM2203*)chip;
        FM_OPN *OPN = &F2203->OPN;
	FM_STATE *state = &F2203->State;

	/* Reset Prescaler */
	OPNPrescaler_w(OPN, 0 , 1 );
	OPNWriteMode(OPN,0x27,0x30); /* mode 0 , timer reset */

	OPN->eg_timer = 0;
	OPN->eg_cnt   = 0;

	reset_channels( &OPN->ST , F2203->CH , 3 );
	/* reset OPerator paramater */
	for(i = 0xb2 ; i >= 0x30 ; i-- ) OPNWriteReg(state, OPN,i,0);
	for(i = 0x26 ; i >= 0x20 ; i-- ) OPNWriteReg(state, OPN,i,0);
}


/* ----------  Initialize YM2203 emulator(s) ----------
   'clock' is the chip clock in Hz
   'rate' is sampling rate
*/
void * YM2203Init(uint64_t clock, int rate)
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

	F2203->OPN.P_CH = F2203->CH;
	F2203->OPN.ST.clock = clock;
	F2203->OPN.ST.rate = rate;

	YM2203ResetChip(F2203);

	return F2203;
}

/* shut down emulator */
void YM2203Shutdown(void *chip)
{
  auto* FM2203 = (YM2203*)chip;

  FMCloseTable();
  free(FM2203);
}

void YM2203WriteRegs(void *chip, int reg, unsigned char val)
{
  auto* F2203 = (YM2203*)chip;
  FM_OPN* OPN = &F2203->OPN;
  if (reg >= 0x2d && reg <= 0x2f)
  {
    OPNPrescaler_w(OPN, reg, 1);
	}
	F2203->REGS[reg] = val;
	if (0x20 == (reg & 0xf0))
	{
	  OPNWriteMode(OPN, reg, val);
	}
	else
	{
	  OPNWriteReg(&F2203->State, OPN, reg, val);
	}
}


uint_t GetPeriod(uint8_t regHi, uint8_t regLo, unsigned scale)
{
  if (const unsigned counter = (unsigned(regHi & 7) << 8) | regLo)
  {
    const unsigned octave = (regHi & 0x38) >> 3;
    return (scale << 21) / (counter << octave);
  }
  else
  {
    return scale << 21;
  }
}

void YM2203GetState(void *chip, uint_t *attenuations, uint_t *periods)
{
	static const uint_t slotMap[8]={ 0x08,0x08,0x08,0x08,0x0c,0x0e,0x0e,0x0f };
	static const uint_t opn_pres[4] = { 2*12 , 2*12 , 6*12 , 3*12 };
	YM2203 *F2203;
	
	F2203=(YM2203*)chip;
	
  const uint_t scale = opn_pres[F2203->OPN.ST.prescaler_sel & 3];
	for (uint_t c = 0; c < 3; ++c)
	{
		const uint_t algo = slotMap[F2203->CH[c].ALGO&7];
		uint_t att = 0;
		uint_t div = 0;
		if(algo&1)
		{
		  att += F2203->CH[c].SLOT[SLOT1].vol_out; div++;
		}
		if(algo&2)
		{
		  att += F2203->CH[c].SLOT[SLOT2].vol_out; div++;
		}
		if(algo&4)
		{
		  att += F2203->CH[c].SLOT[SLOT3].vol_out; div++;
		}
		if(algo&8)
		{
		  att += F2203->CH[c].SLOT[SLOT4].vol_out; div++;
		}
		att /= div;
    attenuations[c] = att;
		if (att < 1024)
		{
		  periods[c] = GetPeriod(F2203->REGS[0xa4 + c], F2203->REGS[0xa0 + c], scale);
		}
	}
}