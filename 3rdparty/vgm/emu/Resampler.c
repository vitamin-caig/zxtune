#include <stddef.h>
#include <stdlib.h>	// for malloc/free
#ifdef _DEBUG
#include <stdio.h>
#endif

#include "../stdtype.h"
#include "EmuStructs.h"
#include "Resampler.h"

static void Resmpl_Exec_Old(RESMPL_STATE* CAA, UINT32 length, WAVE_32BS* retSample);
static void Resmpl_Exec_LinearUp(RESMPL_STATE* CAA, UINT32 length, WAVE_32BS* retSample);
static void Resmpl_Exec_Copy(RESMPL_STATE* CAA, UINT32 length, WAVE_32BS* retSample);
static void Resmpl_Exec_LinearDown(RESMPL_STATE* CAA, UINT32 length, WAVE_32BS* retSample);

// Ensures `CAA->smplBufs[0]` and `CAA->smplBufs[1]` can each contain at least `length` samples.
static void Resmpl_EnsureBuffers(RESMPL_STATE* CAA, UINT32 length)
{
	if (CAA->smplBufSize < length)
	{
		// We don't need the current buffer's contents,
		// so we can avoid calling realloc, which could copy the buffer's contents unnecessarily.
		free(CAA->smplBufs[0]);
		
		CAA->smplBufSize = length;
		CAA->smplBufs[0] = (DEV_SMPL*)malloc(CAA->smplBufSize * 2 * sizeof(DEV_SMPL));
		if (CAA->smplBufs[0] == NULL)
			abort();
		CAA->smplBufs[1] = &CAA->smplBufs[0][CAA->smplBufSize];
	}
}

void Resmpl_DevConnect(RESMPL_STATE* CAA, const DEV_INFO* devInf)
{
	CAA->smpRateSrc = devInf->sampleRate;
	CAA->StreamUpdate = devInf->devDef->Update;
	CAA->su_DataPtr = devInf->dataPtr;
	if (devInf->devDef->SetSRateChgCB != NULL)
		devInf->devDef->SetSRateChgCB(CAA->su_DataPtr, Resmpl_ChangeRate, CAA);
	
	return;
}

void Resmpl_SetVals(RESMPL_STATE* CAA, UINT8 resampleMode, UINT16 volume, UINT32 destSampleRate)
{
	CAA->resampleMode = resampleMode;
	CAA->smpRateDst = destSampleRate;
	CAA->volumeL = volume;	CAA->volumeR = volume;
	
	return;
}

static void Resmpl_ChooseResampler(RESMPL_STATE* CAA)
{
	switch(CAA->resampleMode)
	{
	case RSMODE_LINEAR:	// linear interpolation (good quality)
		if (CAA->smpRateSrc < CAA->smpRateDst)
			CAA->resampler = Resmpl_Exec_LinearUp;
		else if (CAA->smpRateSrc == CAA->smpRateDst)
			CAA->resampler = Resmpl_Exec_Copy;
		else if (CAA->smpRateSrc > CAA->smpRateDst)
			CAA->resampler = Resmpl_Exec_LinearDown;
		break;
	case RSMODE_NEAREST:	// nearest-neighbour (low quality)
		if (CAA->smpRateSrc < CAA->smpRateDst)
			CAA->resampler = Resmpl_Exec_Old;
		else if (CAA->smpRateSrc == CAA->smpRateDst)
			CAA->resampler = Resmpl_Exec_Copy;
		else if (CAA->smpRateSrc > CAA->smpRateDst)
			CAA->resampler = Resmpl_Exec_Old;
		break;
	case RSMODE_LUP_NDWN:	// nearest-neighbour downsampling, interpolation upsampling
		if (CAA->smpRateSrc < CAA->smpRateDst)
			CAA->resampler = Resmpl_Exec_LinearUp;
		else if (CAA->smpRateSrc == CAA->smpRateDst)
			CAA->resampler = Resmpl_Exec_Copy;
		else if (CAA->smpRateSrc > CAA->smpRateDst)
			CAA->resampler = Resmpl_Exec_Old;
		break;
	default:
#ifdef _DEBUG
		printf("Invalid resampler mode 0x%02X used!\n", CAA->resampleMode);
#endif
		CAA->resampler = NULL;
		break;
	}
}

void Resmpl_Init(RESMPL_STATE* CAA)
{
	if (! CAA->smpRateSrc)
	{
		CAA->resampler = NULL;
		return;
	}
	
	Resmpl_ChooseResampler(CAA);
	
	CAA->smplBufSize = 0;
	CAA->smplBufs[0] = NULL;
	CAA->smplBufs[1] = NULL;
	Resmpl_EnsureBuffers(CAA, CAA->smpRateSrc / 1); // reserve initial buffer for 1 second of samples
	
	CAA->smpP = 0x00;
	CAA->smpLast = 0x00;
	CAA->smpNext = 0x00;
	CAA->lSmpl.L = 0x00;
	CAA->lSmpl.R = 0x00;
	if (CAA->resampler == Resmpl_Exec_LinearUp)
	{
		// Pregenerate first Sample (the upsampler is always one too late)
		CAA->StreamUpdate(CAA->su_DataPtr, 1, CAA->smplBufs);
		CAA->nSmpl.L = CAA->smplBufs[0][0];
		CAA->nSmpl.R = CAA->smplBufs[1][0];
	}
	else
	{
		CAA->nSmpl.L = 0x00;
		CAA->nSmpl.R = 0x00;
	}
	
	return;
}

void Resmpl_Deinit(RESMPL_STATE* CAA)
{
	CAA->smplBufSize = 0;
	free(CAA->smplBufs[0]);
	CAA->smplBufs[0] = NULL;
	CAA->smplBufs[1] = NULL;
	
	return;
}

void Resmpl_ChangeRate(void* DataPtr, UINT32 newSmplRate)
{
	RESMPL_STATE* CAA = (RESMPL_STATE*)DataPtr;
	
	if (CAA->smpRateSrc == newSmplRate)
		return;
	
	// quick and dirty hack to make sample rate changes work
	CAA->smpRateSrc = newSmplRate;
	Resmpl_ChooseResampler(CAA);
	CAA->smpP = 1;
	CAA->smpNext -= CAA->smpLast;
	CAA->smpLast = 0x00;
	
	return;
}

// I recommend 11 bits as it's fast and accurate
#define FIXPNT_BITS		11
#define FIXPNT_FACT		(1 << FIXPNT_BITS)
#if (FIXPNT_BITS <= 11)	// allows for chip sample rates of about 10 MHz without overflow
	typedef UINT32	SLINT;	// 32-bit is a lot faster
	#define SLI_BITS	32
#else
	typedef UINT64	SLINT;
	#define SLI_BITS	64
#endif
#define FIXPNT_MASK		(FIXPNT_FACT - 1)
#define FIXPNT_OFLW_BIT	(SLI_BITS - FIXPNT_BITS)

#define getfraction(x)	((x) & FIXPNT_MASK)
#define getnfraction(x)	((FIXPNT_FACT - (x)) & FIXPNT_MASK)
#define fpi_floor(x)	((x) & ~FIXPNT_MASK)
#define fpi_ceil(x)		((x + FIXPNT_MASK) & ~FIXPNT_MASK)
#define fp2i_floor(x)	((x) / FIXPNT_FACT)
#define fp2i_ceil(x)	((x + FIXPNT_MASK) / FIXPNT_FACT)

static void Resmpl_Exec_Old(RESMPL_STATE* CAA, UINT32 length, WAVE_32BS* retSample)
{
	// RESALGO_OLD: old, but very fast resampler
	// This is effectively nearest-neighbour resampling, but with small improvements for downsampling.
	DEV_SMPL* CurBufL;
	DEV_SMPL* CurBufR;
	UINT32 OutPos;
	INT32 TempS32L;
	INT32 TempS32R;
	INT32 SmpCnt;	// must be signed, else I'm getting calculation errors
	INT32 CurSmpl;
	
	for (OutPos = 0; OutPos < length; OutPos ++)
	{
		CAA->smpLast = CAA->smpNext;
		CAA->smpP ++;
		CAA->smpNext = (UINT32)((UINT64)CAA->smpP * CAA->smpRateSrc / CAA->smpRateDst);
		if (CAA->smpLast >= CAA->smpNext)
		{
			// duplicate last sample (nearest-neighbour resampling)
			retSample[OutPos].L += CAA->lSmpl.L * CAA->volumeL;
			retSample[OutPos].R += CAA->lSmpl.R * CAA->volumeR;
		}
		else //if (CAA->smpLast < CAA->smpNext)
		{
			SmpCnt = CAA->smpNext - CAA->smpLast;
			
			Resmpl_EnsureBuffers(CAA, SmpCnt);
			CurBufL = CAA->smplBufs[0];
			CurBufR = CAA->smplBufs[1];
			
			CAA->StreamUpdate(CAA->su_DataPtr, SmpCnt, CAA->smplBufs);
			
			// This is a mix between nearest-neighbour resampling and interpolation.
			// If only 1 sample is rendered by the sound core, the sample is copied over as-is.
			// If multiple samples are rendered, they are averaged.
			if (SmpCnt == 1)
			{
				retSample[OutPos].L += CurBufL[0] * CAA->volumeL;
				retSample[OutPos].R += CurBufR[0] * CAA->volumeR;
				CAA->lSmpl.L = CurBufL[0];
				CAA->lSmpl.R = CurBufR[0];
			}
			else if (SmpCnt == 2)
			{
				retSample[OutPos].L += (CurBufL[0] + CurBufL[1]) * CAA->volumeL / 2;
				retSample[OutPos].R += (CurBufR[0] + CurBufR[1]) * CAA->volumeR / 2;
				CAA->lSmpl.L = CurBufL[1];
				CAA->lSmpl.R = CurBufR[1];
			}
			else
			{
				TempS32L = CurBufL[0];
				TempS32R = CurBufR[0];
				for (CurSmpl = 1; CurSmpl < SmpCnt; CurSmpl ++)
				{
					TempS32L += CurBufL[CurSmpl];
					TempS32R += CurBufR[CurSmpl];
				}
				retSample[OutPos].L += TempS32L * CAA->volumeL / SmpCnt;
				retSample[OutPos].R += TempS32R * CAA->volumeR / SmpCnt;
				CAA->lSmpl.L = CurBufL[SmpCnt - 1];
				CAA->lSmpl.R = CurBufR[SmpCnt - 1];
			}
		}
	}
	
	if (CAA->smpLast >= CAA->smpRateSrc)
	{
		CAA->smpLast -= CAA->smpRateSrc;
		CAA->smpNext -= CAA->smpRateSrc;
		CAA->smpP -= CAA->smpRateDst;
	}
	
	return;
}

static void Resmpl_Exec_LinearUp(RESMPL_STATE* CAA, UINT32 length, WAVE_32BS* retSample)
{
	// RESALGO_LINEAR_UP: Linear Upsampling
	DEV_SMPL* CurBufL;
	DEV_SMPL* CurBufR;
	DEV_SMPL* StreamPnt[0x02];
	UINT32 InBase;
	UINT32 InPos;
	UINT32 OutPos;
	UINT32 SmpFrc;	// Sample Fraction
	UINT32 InPre;
	UINT32 InNow;
	SLINT InPosL;
	INT64 TempSmpL;
	INT64 TempSmpR;
	INT32 SmpCnt;	// must be signed, else I'm getting calculation errors
	UINT64 ChipSmpRateFP;
	
	ChipSmpRateFP = FIXPNT_FACT * (UINT64)CAA->smpRateSrc;
	// TODO: Make it work *properly* with large blocks. (this is very hackish)
	for (OutPos = 0; OutPos < length; OutPos ++)
	{
		InPosL = (SLINT)(CAA->smpP * ChipSmpRateFP / CAA->smpRateDst);
		InPre = (UINT32)fp2i_floor(InPosL);
		InNow = (UINT32)fp2i_ceil(InPosL);
		
		Resmpl_EnsureBuffers(CAA, InNow - CAA->smpNext + 2);
		CurBufL = CAA->smplBufs[0];
		CurBufR = CAA->smplBufs[1];
		
		CurBufL[0] = CAA->lSmpl.L;
		CurBufR[0] = CAA->lSmpl.R;
		CurBufL[1] = CAA->nSmpl.L;
		CurBufR[1] = CAA->nSmpl.R;
		if (InNow != CAA->smpNext)
		{
			StreamPnt[0] = &CurBufL[2];
			StreamPnt[1] = &CurBufR[2];
			CAA->StreamUpdate(CAA->su_DataPtr, InNow - CAA->smpNext, StreamPnt);
		}
		
		InBase = FIXPNT_FACT + (UINT32)(InPosL - (SLINT)CAA->smpNext * FIXPNT_FACT);
		SmpCnt = FIXPNT_FACT;
		CAA->smpLast = InPre;
		CAA->smpNext = InNow;
		//for (OutPos = 0; OutPos < length; OutPos ++)
		//{
		//InPos = InBase + (UINT32)(OutPos * ChipSmpRateFP / CAA->smpRateDst);
		InPos = InBase;
		
		InPre = fp2i_floor(InPos);
		InNow = fp2i_ceil(InPos);
		SmpFrc = getfraction(InPos);
		
		// Linear interpolation
		TempSmpL = ((INT64)CurBufL[InPre] * (FIXPNT_FACT - SmpFrc)) +
					((INT64)CurBufL[InNow] * SmpFrc);
		TempSmpR = ((INT64)CurBufR[InPre] * (FIXPNT_FACT - SmpFrc)) +
					((INT64)CurBufR[InNow] * SmpFrc);
		retSample[OutPos].L += (INT32)(TempSmpL * CAA->volumeL / SmpCnt);
		retSample[OutPos].R += (INT32)(TempSmpR * CAA->volumeR / SmpCnt);
		//}
		CAA->lSmpl.L = CurBufL[InPre];
		CAA->lSmpl.R = CurBufR[InPre];
		CAA->nSmpl.L = CurBufL[InNow];
		CAA->nSmpl.R = CurBufR[InNow];
		CAA->smpP ++;
	}
	
	if (CAA->smpLast >= CAA->smpRateSrc)
	{
		CAA->smpLast -= CAA->smpRateSrc;
		CAA->smpNext -= CAA->smpRateSrc;
		CAA->smpP -= CAA->smpRateDst;
	}
	
	return;
}

static void Resmpl_Exec_Copy(RESMPL_STATE* CAA, UINT32 length, WAVE_32BS* retSample)
{
	// RESALGO_COPY: Copying
	UINT32 OutPos;
	
	CAA->smpNext = CAA->smpP * CAA->smpRateSrc / CAA->smpRateDst;
	Resmpl_EnsureBuffers(CAA, length);
	CAA->StreamUpdate(CAA->su_DataPtr, length, CAA->smplBufs);
	
	for (OutPos = 0; OutPos < length; OutPos ++)
	{
		retSample[OutPos].L += CAA->smplBufs[0][OutPos] * CAA->volumeL;
		retSample[OutPos].R += CAA->smplBufs[1][OutPos] * CAA->volumeR;
	}
	CAA->smpP += length;
	CAA->smpLast = CAA->smpNext;
	
	if (CAA->smpLast >= CAA->smpRateSrc)
	{
		CAA->smpLast -= CAA->smpRateSrc;
		CAA->smpNext -= CAA->smpRateSrc;
		CAA->smpP -= CAA->smpRateDst;
	}
	
	return;
}

// TODO: The resample is not completely stable.
//	Resampling tiny blocks (1 resulting sample) sometimes causes values to be off-by-one,
//	compared to resampling large blocks.
static void Resmpl_Exec_LinearDown(RESMPL_STATE* CAA, UINT32 length, WAVE_32BS* retSample)
{
	// RESALGO_LINEAR_DOWN: Linear Downsampling
	DEV_SMPL* CurBufL;
	DEV_SMPL* CurBufR;
	DEV_SMPL* StreamPnt[0x02];
	UINT32 InBase;
	UINT32 InPos;
	UINT32 InPosNext;
	UINT32 OutPos;
	UINT32 SmpFrc;	// Sample Fraction
	UINT32 InPre;
	UINT32 InNow;
	SLINT InPosL;
	INT64 TempSmpL;
	INT64 TempSmpR;
	INT32 SmpCnt;	// must be signed, else I'm getting calculation errors
	UINT64 ChipSmpRateFP;
	
	ChipSmpRateFP = FIXPNT_FACT * (UINT64)CAA->smpRateSrc;
	InPosL = (SLINT)((CAA->smpP + length) * ChipSmpRateFP / CAA->smpRateDst);
	CAA->smpNext = (UINT32)fp2i_ceil(InPosL);
#if FIXPNT_OFLW_BIT < 32
	if (CAA->smpNext < CAA->smpLast)
	{
		// work around overflow in InPosL (may happen with extremely high chip sample rates)
		CAA->smpNext |= CAA->smpLast & ~(((UINT32)1 << FIXPNT_OFLW_BIT) - 1);
		if (CAA->smpNext < CAA->smpLast)
			CAA->smpNext += ((UINT32)1 << FIXPNT_OFLW_BIT);
	}
#endif
	
	Resmpl_EnsureBuffers(CAA, CAA->smpNext - CAA->smpLast + 1);
	CurBufL = CAA->smplBufs[0];
	CurBufR = CAA->smplBufs[1];
	CurBufL[0] = CAA->lSmpl.L;
	CurBufR[0] = CAA->lSmpl.R;
	StreamPnt[0] = &CurBufL[1];
	StreamPnt[1] = &CurBufR[1];
	CAA->StreamUpdate(CAA->su_DataPtr, CAA->smpNext - CAA->smpLast, StreamPnt);
	
	InPosL = (SLINT)(CAA->smpP * ChipSmpRateFP / CAA->smpRateDst);
	// I'm adding 1.0 to avoid negative indexes
	InBase = FIXPNT_FACT + (UINT32)(InPosL - (SLINT)CAA->smpLast * FIXPNT_FACT);
	InPosNext = InBase;
	for (OutPos = 0; OutPos < length; OutPos ++)
	{
		//InPos = InBase + (UINT32)(OutPos * ChipSmpRateFP / CAA->smpRateDst);
		InPos = InPosNext;
		InPosNext = InBase + (UINT32)((OutPos+1) * ChipSmpRateFP / CAA->smpRateDst);
		
		// first fractional Sample
		SmpFrc = getnfraction(InPos);
		if (SmpFrc)
		{
			InPre = fp2i_floor(InPos);
			TempSmpL = (INT64)CurBufL[InPre] * SmpFrc;
			TempSmpR = (INT64)CurBufR[InPre] * SmpFrc;
		}
		else
		{
			TempSmpL = TempSmpR = 0;
		}
		SmpCnt = SmpFrc;
		
		// last fractional Sample
		SmpFrc = getfraction(InPosNext);
		InPre = fp2i_floor(InPosNext);
		if (SmpFrc)
		{
			TempSmpL += (INT64)CurBufL[InPre] * SmpFrc;
			TempSmpR += (INT64)CurBufR[InPre] * SmpFrc;
			SmpCnt += SmpFrc;
		}
		
		// whole Samples in between
		//InPre = fp2i_floor(InPosNext);
		InNow = fp2i_ceil(InPos);
		SmpCnt += (InPre - InNow) * FIXPNT_FACT;	// this is faster
		while(InNow < InPre)
		{
			TempSmpL += (INT64)CurBufL[InNow] * FIXPNT_FACT;
			TempSmpR += (INT64)CurBufR[InNow] * FIXPNT_FACT;
			//SmpCnt ++;
			InNow ++;
		}
		
		retSample[OutPos].L += (INT32)(TempSmpL * CAA->volumeL / SmpCnt);
		retSample[OutPos].R += (INT32)(TempSmpR * CAA->volumeR / SmpCnt);
	}
	
	CAA->lSmpl.L = CurBufL[InPre];
	CAA->lSmpl.R = CurBufR[InPre];
	CAA->smpP += length;
	CAA->smpLast = CAA->smpNext;
	
	if (CAA->smpLast >= CAA->smpRateSrc)
	{
		CAA->smpLast -= CAA->smpRateSrc;
		CAA->smpNext -= CAA->smpRateSrc;
		CAA->smpP -= CAA->smpRateDst;
	}
	
	return;
}

void Resmpl_Execute(RESMPL_STATE* CAA, UINT32 smplCount, WAVE_32BS* smplBuffer)
{
	if (! smplCount)
		return;
	
	if (CAA->resampler != NULL)
		CAA->resampler(CAA, smplCount, smplBuffer);
	else
		CAA->smpP += CAA->smpRateDst;	// just skip the samples and do nothing else
	return;
}
