/*
Abstract:
  AY/YM chips device class

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  Based on sources of UnrealSpeccy by SMT and Xpeccy sources by SamStyle
*/

#pragma once
#ifndef DEVICES_AYM_DEVICE_H_DEFINED
#define DEVICES_AYM_DEVICE_H_DEFINED

//local includes
#include "generators.h"
//library includes
#include <devices/aym/chip.h>

namespace Devices
{
  namespace AYM
  {
    class AYMDevice
    {
    public:
      AYMDevice()
        : LevelA(), LevelB(), LevelC()
        , VolumeTable(&GetAY38910VolTable())
        , State()
        , GetLevelsFunc(&AYMDevice::GetLevels000)
      {
      }

      void SetVolumeTable(const VolTable& table)
      {
        VolumeTable = &table;
      }

      void SetDutyCycle(uint_t value, uint_t mask)
      {
        GenA.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_A) ? value : NO_DUTYCYCLE);
        GenB.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_B) ? value : NO_DUTYCYCLE);
        GenC.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_C) ? value : NO_DUTYCYCLE);
        GenN.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_N) ? value : NO_DUTYCYCLE);
        GenE.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_E) ? value : NO_DUTYCYCLE);
      }

      void SetMixer(uint_t mixer)
      {
        const uint_t mixMask = DataChunk::REG_MASK_TONEA | DataChunk::REG_MASK_TONEB | DataChunk::REG_MASK_TONEC | 
          DataChunk::REG_MASK_NOISEA | DataChunk::REG_MASK_NOISEB | DataChunk::REG_MASK_NOISEC;
        State = (State & ~mixMask) | (~mixer & mixMask);
        ApplyState();
      }

      void SetPeriods(uint_t toneA, uint_t toneB, uint_t toneC, uint_t toneN, uint_t toneE)
      {
        GenA.SetPeriod(toneA);
        GenB.SetPeriod(toneB);
        GenC.SetPeriod(toneC);
        GenN.SetPeriod(toneN);
        GenE.SetPeriod(toneE);
      }

      void SetEnvType(uint_t type)
      {
        GenE.SetType(type);
      }

      void SetLevel(uint_t levelA, uint_t levelB, uint_t levelC)
      {
        LevelA = (((levelA & DataChunk::REG_MASK_VOL) << 1) + 1);
        LevelB = (((levelB & DataChunk::REG_MASK_VOL) << 1) + 1);
        LevelC = (((levelC & DataChunk::REG_MASK_VOL) << 1) + 1);
        const uint_t envMask = 64 | 128 | 256;
        const uint_t envFlags = 
          (0 != (levelA & DataChunk::REG_MASK_ENV) ? 64 : 0) |
          (0 != (levelB & DataChunk::REG_MASK_ENV) ? 128 : 0) |
          (0 != (levelC & DataChunk::REG_MASK_ENV) ? 256 : 0)
        ;
        State = (State & ~envMask) | envFlags;
        ApplyState();
      }

      void Reset()
      {
        GenA.Reset();
        GenB.Reset();
        GenC.Reset();
        GenN.Reset();
        GenE.Reset();
        LevelA = LevelB = LevelC = 0;
        VolumeTable = &GetAY38910VolTable();
        State = 0;
        GetLevelsFunc = &AYMDevice::GetLevels000;
      }

      void Tick(uint_t ticks)
      {
        GenA.Tick(ticks);
        GenB.Tick(ticks);
        GenC.Tick(ticks);
        GenN.Tick(ticks);
        GenE.Tick(ticks);
      }

      void GetLevels(MultiSample& result) const
      {
        (this->*GetLevelsFunc)(result);
      }
    private:
      void ApplyState()
      {
        static const GetLevelsFuncType FUNCTIONS[] =
        {
          &AYMDevice::GetLevels000,
          &AYMDevice::GetLevels001,
          &AYMDevice::GetLevels002,
          &AYMDevice::GetLevels003,
          &AYMDevice::GetLevels004,
          &AYMDevice::GetLevels005,
          &AYMDevice::GetLevels006,
          &AYMDevice::GetLevels007,
          &AYMDevice::GetLevels010,
          &AYMDevice::GetLevels011,
          &AYMDevice::GetLevels012,
          &AYMDevice::GetLevels013,
          &AYMDevice::GetLevels014,
          &AYMDevice::GetLevels015,
          &AYMDevice::GetLevels016,
          &AYMDevice::GetLevels017,
          &AYMDevice::GetLevels020,
          &AYMDevice::GetLevels021,
          &AYMDevice::GetLevels022,
          &AYMDevice::GetLevels023,
          &AYMDevice::GetLevels024,
          &AYMDevice::GetLevels025,
          &AYMDevice::GetLevels026,
          &AYMDevice::GetLevels027,
          &AYMDevice::GetLevels030,
          &AYMDevice::GetLevels031,
          &AYMDevice::GetLevels032,
          &AYMDevice::GetLevels033,
          &AYMDevice::GetLevels034,
          &AYMDevice::GetLevels035,
          &AYMDevice::GetLevels036,
          &AYMDevice::GetLevels037,
          &AYMDevice::GetLevels040,
          &AYMDevice::GetLevels041,
          &AYMDevice::GetLevels042,
          &AYMDevice::GetLevels043,
          &AYMDevice::GetLevels044,
          &AYMDevice::GetLevels045,
          &AYMDevice::GetLevels046,
          &AYMDevice::GetLevels047,
          &AYMDevice::GetLevels050,
          &AYMDevice::GetLevels051,
          &AYMDevice::GetLevels052,
          &AYMDevice::GetLevels053,
          &AYMDevice::GetLevels054,
          &AYMDevice::GetLevels055,
          &AYMDevice::GetLevels056,
          &AYMDevice::GetLevels057,
          &AYMDevice::GetLevels060,
          &AYMDevice::GetLevels061,
          &AYMDevice::GetLevels062,
          &AYMDevice::GetLevels063,
          &AYMDevice::GetLevels064,
          &AYMDevice::GetLevels065,
          &AYMDevice::GetLevels066,
          &AYMDevice::GetLevels067,
          &AYMDevice::GetLevels070,
          &AYMDevice::GetLevels071,
          &AYMDevice::GetLevels072,
          &AYMDevice::GetLevels073,
          &AYMDevice::GetLevels074,
          &AYMDevice::GetLevels075,
          &AYMDevice::GetLevels076,
          &AYMDevice::GetLevels077,
          &AYMDevice::GetLevels100,
          &AYMDevice::GetLevels101,
          &AYMDevice::GetLevels102,
          &AYMDevice::GetLevels103,
          &AYMDevice::GetLevels104,
          &AYMDevice::GetLevels105,
          &AYMDevice::GetLevels106,
          &AYMDevice::GetLevels107,
          &AYMDevice::GetLevels110,
          &AYMDevice::GetLevels111,
          &AYMDevice::GetLevels112,
          &AYMDevice::GetLevels113,
          &AYMDevice::GetLevels114,
          &AYMDevice::GetLevels115,
          &AYMDevice::GetLevels116,
          &AYMDevice::GetLevels117,
          &AYMDevice::GetLevels120,
          &AYMDevice::GetLevels121,
          &AYMDevice::GetLevels122,
          &AYMDevice::GetLevels123,
          &AYMDevice::GetLevels124,
          &AYMDevice::GetLevels125,
          &AYMDevice::GetLevels126,
          &AYMDevice::GetLevels127,
          &AYMDevice::GetLevels130,
          &AYMDevice::GetLevels131,
          &AYMDevice::GetLevels132,
          &AYMDevice::GetLevels133,
          &AYMDevice::GetLevels134,
          &AYMDevice::GetLevels135,
          &AYMDevice::GetLevels136,
          &AYMDevice::GetLevels137,
          &AYMDevice::GetLevels140,
          &AYMDevice::GetLevels141,
          &AYMDevice::GetLevels142,
          &AYMDevice::GetLevels143,
          &AYMDevice::GetLevels144,
          &AYMDevice::GetLevels145,
          &AYMDevice::GetLevels146,
          &AYMDevice::GetLevels147,
          &AYMDevice::GetLevels150,
          &AYMDevice::GetLevels151,
          &AYMDevice::GetLevels152,
          &AYMDevice::GetLevels153,
          &AYMDevice::GetLevels154,
          &AYMDevice::GetLevels155,
          &AYMDevice::GetLevels156,
          &AYMDevice::GetLevels157,
          &AYMDevice::GetLevels160,
          &AYMDevice::GetLevels161,
          &AYMDevice::GetLevels162,
          &AYMDevice::GetLevels163,
          &AYMDevice::GetLevels164,
          &AYMDevice::GetLevels165,
          &AYMDevice::GetLevels166,
          &AYMDevice::GetLevels167,
          &AYMDevice::GetLevels170,
          &AYMDevice::GetLevels171,
          &AYMDevice::GetLevels172,
          &AYMDevice::GetLevels173,
          &AYMDevice::GetLevels174,
          &AYMDevice::GetLevels175,
          &AYMDevice::GetLevels176,
          &AYMDevice::GetLevels177,
          &AYMDevice::GetLevels200,
          &AYMDevice::GetLevels201,
          &AYMDevice::GetLevels202,
          &AYMDevice::GetLevels203,
          &AYMDevice::GetLevels204,
          &AYMDevice::GetLevels205,
          &AYMDevice::GetLevels206,
          &AYMDevice::GetLevels207,
          &AYMDevice::GetLevels210,
          &AYMDevice::GetLevels211,
          &AYMDevice::GetLevels212,
          &AYMDevice::GetLevels213,
          &AYMDevice::GetLevels214,
          &AYMDevice::GetLevels215,
          &AYMDevice::GetLevels216,
          &AYMDevice::GetLevels217,
          &AYMDevice::GetLevels220,
          &AYMDevice::GetLevels221,
          &AYMDevice::GetLevels222,
          &AYMDevice::GetLevels223,
          &AYMDevice::GetLevels224,
          &AYMDevice::GetLevels225,
          &AYMDevice::GetLevels226,
          &AYMDevice::GetLevels227,
          &AYMDevice::GetLevels230,
          &AYMDevice::GetLevels231,
          &AYMDevice::GetLevels232,
          &AYMDevice::GetLevels233,
          &AYMDevice::GetLevels234,
          &AYMDevice::GetLevels235,
          &AYMDevice::GetLevels236,
          &AYMDevice::GetLevels237,
          &AYMDevice::GetLevels240,
          &AYMDevice::GetLevels241,
          &AYMDevice::GetLevels242,
          &AYMDevice::GetLevels243,
          &AYMDevice::GetLevels244,
          &AYMDevice::GetLevels245,
          &AYMDevice::GetLevels246,
          &AYMDevice::GetLevels247,
          &AYMDevice::GetLevels250,
          &AYMDevice::GetLevels251,
          &AYMDevice::GetLevels252,
          &AYMDevice::GetLevels253,
          &AYMDevice::GetLevels254,
          &AYMDevice::GetLevels255,
          &AYMDevice::GetLevels256,
          &AYMDevice::GetLevels257,
          &AYMDevice::GetLevels260,
          &AYMDevice::GetLevels261,
          &AYMDevice::GetLevels262,
          &AYMDevice::GetLevels263,
          &AYMDevice::GetLevels264,
          &AYMDevice::GetLevels265,
          &AYMDevice::GetLevels266,
          &AYMDevice::GetLevels267,
          &AYMDevice::GetLevels270,
          &AYMDevice::GetLevels271,
          &AYMDevice::GetLevels272,
          &AYMDevice::GetLevels273,
          &AYMDevice::GetLevels274,
          &AYMDevice::GetLevels275,
          &AYMDevice::GetLevels276,
          &AYMDevice::GetLevels277,
          &AYMDevice::GetLevels300,
          &AYMDevice::GetLevels301,
          &AYMDevice::GetLevels302,
          &AYMDevice::GetLevels303,
          &AYMDevice::GetLevels304,
          &AYMDevice::GetLevels305,
          &AYMDevice::GetLevels306,
          &AYMDevice::GetLevels307,
          &AYMDevice::GetLevels310,
          &AYMDevice::GetLevels311,
          &AYMDevice::GetLevels312,
          &AYMDevice::GetLevels313,
          &AYMDevice::GetLevels314,
          &AYMDevice::GetLevels315,
          &AYMDevice::GetLevels316,
          &AYMDevice::GetLevels317,
          &AYMDevice::GetLevels320,
          &AYMDevice::GetLevels321,
          &AYMDevice::GetLevels322,
          &AYMDevice::GetLevels323,
          &AYMDevice::GetLevels324,
          &AYMDevice::GetLevels325,
          &AYMDevice::GetLevels326,
          &AYMDevice::GetLevels327,
          &AYMDevice::GetLevels330,
          &AYMDevice::GetLevels331,
          &AYMDevice::GetLevels332,
          &AYMDevice::GetLevels333,
          &AYMDevice::GetLevels334,
          &AYMDevice::GetLevels335,
          &AYMDevice::GetLevels336,
          &AYMDevice::GetLevels337,
          &AYMDevice::GetLevels340,
          &AYMDevice::GetLevels341,
          &AYMDevice::GetLevels342,
          &AYMDevice::GetLevels343,
          &AYMDevice::GetLevels344,
          &AYMDevice::GetLevels345,
          &AYMDevice::GetLevels346,
          &AYMDevice::GetLevels347,
          &AYMDevice::GetLevels350,
          &AYMDevice::GetLevels351,
          &AYMDevice::GetLevels352,
          &AYMDevice::GetLevels353,
          &AYMDevice::GetLevels354,
          &AYMDevice::GetLevels355,
          &AYMDevice::GetLevels356,
          &AYMDevice::GetLevels357,
          &AYMDevice::GetLevels360,
          &AYMDevice::GetLevels361,
          &AYMDevice::GetLevels362,
          &AYMDevice::GetLevels363,
          &AYMDevice::GetLevels364,
          &AYMDevice::GetLevels365,
          &AYMDevice::GetLevels366,
          &AYMDevice::GetLevels367,
          &AYMDevice::GetLevels370,
          &AYMDevice::GetLevels371,
          &AYMDevice::GetLevels372,
          &AYMDevice::GetLevels373,
          &AYMDevice::GetLevels374,
          &AYMDevice::GetLevels375,
          &AYMDevice::GetLevels376,
          &AYMDevice::GetLevels377,
          &AYMDevice::GetLevels400,
          &AYMDevice::GetLevels401,
          &AYMDevice::GetLevels402,
          &AYMDevice::GetLevels403,
          &AYMDevice::GetLevels404,
          &AYMDevice::GetLevels405,
          &AYMDevice::GetLevels406,
          &AYMDevice::GetLevels407,
          &AYMDevice::GetLevels410,
          &AYMDevice::GetLevels411,
          &AYMDevice::GetLevels412,
          &AYMDevice::GetLevels413,
          &AYMDevice::GetLevels414,
          &AYMDevice::GetLevels415,
          &AYMDevice::GetLevels416,
          &AYMDevice::GetLevels417,
          &AYMDevice::GetLevels420,
          &AYMDevice::GetLevels421,
          &AYMDevice::GetLevels422,
          &AYMDevice::GetLevels423,
          &AYMDevice::GetLevels424,
          &AYMDevice::GetLevels425,
          &AYMDevice::GetLevels426,
          &AYMDevice::GetLevels427,
          &AYMDevice::GetLevels430,
          &AYMDevice::GetLevels431,
          &AYMDevice::GetLevels432,
          &AYMDevice::GetLevels433,
          &AYMDevice::GetLevels434,
          &AYMDevice::GetLevels435,
          &AYMDevice::GetLevels436,
          &AYMDevice::GetLevels437,
          &AYMDevice::GetLevels440,
          &AYMDevice::GetLevels441,
          &AYMDevice::GetLevels442,
          &AYMDevice::GetLevels443,
          &AYMDevice::GetLevels444,
          &AYMDevice::GetLevels445,
          &AYMDevice::GetLevels446,
          &AYMDevice::GetLevels447,
          &AYMDevice::GetLevels450,
          &AYMDevice::GetLevels451,
          &AYMDevice::GetLevels452,
          &AYMDevice::GetLevels453,
          &AYMDevice::GetLevels454,
          &AYMDevice::GetLevels455,
          &AYMDevice::GetLevels456,
          &AYMDevice::GetLevels457,
          &AYMDevice::GetLevels460,
          &AYMDevice::GetLevels461,
          &AYMDevice::GetLevels462,
          &AYMDevice::GetLevels463,
          &AYMDevice::GetLevels464,
          &AYMDevice::GetLevels465,
          &AYMDevice::GetLevels466,
          &AYMDevice::GetLevels467,
          &AYMDevice::GetLevels470,
          &AYMDevice::GetLevels471,
          &AYMDevice::GetLevels472,
          &AYMDevice::GetLevels473,
          &AYMDevice::GetLevels474,
          &AYMDevice::GetLevels475,
          &AYMDevice::GetLevels476,
          &AYMDevice::GetLevels477,
          &AYMDevice::GetLevels500,
          &AYMDevice::GetLevels501,
          &AYMDevice::GetLevels502,
          &AYMDevice::GetLevels503,
          &AYMDevice::GetLevels504,
          &AYMDevice::GetLevels505,
          &AYMDevice::GetLevels506,
          &AYMDevice::GetLevels507,
          &AYMDevice::GetLevels510,
          &AYMDevice::GetLevels511,
          &AYMDevice::GetLevels512,
          &AYMDevice::GetLevels513,
          &AYMDevice::GetLevels514,
          &AYMDevice::GetLevels515,
          &AYMDevice::GetLevels516,
          &AYMDevice::GetLevels517,
          &AYMDevice::GetLevels520,
          &AYMDevice::GetLevels521,
          &AYMDevice::GetLevels522,
          &AYMDevice::GetLevels523,
          &AYMDevice::GetLevels524,
          &AYMDevice::GetLevels525,
          &AYMDevice::GetLevels526,
          &AYMDevice::GetLevels527,
          &AYMDevice::GetLevels530,
          &AYMDevice::GetLevels531,
          &AYMDevice::GetLevels532,
          &AYMDevice::GetLevels533,
          &AYMDevice::GetLevels534,
          &AYMDevice::GetLevels535,
          &AYMDevice::GetLevels536,
          &AYMDevice::GetLevels537,
          &AYMDevice::GetLevels540,
          &AYMDevice::GetLevels541,
          &AYMDevice::GetLevels542,
          &AYMDevice::GetLevels543,
          &AYMDevice::GetLevels544,
          &AYMDevice::GetLevels545,
          &AYMDevice::GetLevels546,
          &AYMDevice::GetLevels547,
          &AYMDevice::GetLevels550,
          &AYMDevice::GetLevels551,
          &AYMDevice::GetLevels552,
          &AYMDevice::GetLevels553,
          &AYMDevice::GetLevels554,
          &AYMDevice::GetLevels555,
          &AYMDevice::GetLevels556,
          &AYMDevice::GetLevels557,
          &AYMDevice::GetLevels560,
          &AYMDevice::GetLevels561,
          &AYMDevice::GetLevels562,
          &AYMDevice::GetLevels563,
          &AYMDevice::GetLevels564,
          &AYMDevice::GetLevels565,
          &AYMDevice::GetLevels566,
          &AYMDevice::GetLevels567,
          &AYMDevice::GetLevels570,
          &AYMDevice::GetLevels571,
          &AYMDevice::GetLevels572,
          &AYMDevice::GetLevels573,
          &AYMDevice::GetLevels574,
          &AYMDevice::GetLevels575,
          &AYMDevice::GetLevels576,
          &AYMDevice::GetLevels577,
          &AYMDevice::GetLevels600,
          &AYMDevice::GetLevels601,
          &AYMDevice::GetLevels602,
          &AYMDevice::GetLevels603,
          &AYMDevice::GetLevels604,
          &AYMDevice::GetLevels605,
          &AYMDevice::GetLevels606,
          &AYMDevice::GetLevels607,
          &AYMDevice::GetLevels610,
          &AYMDevice::GetLevels611,
          &AYMDevice::GetLevels612,
          &AYMDevice::GetLevels613,
          &AYMDevice::GetLevels614,
          &AYMDevice::GetLevels615,
          &AYMDevice::GetLevels616,
          &AYMDevice::GetLevels617,
          &AYMDevice::GetLevels620,
          &AYMDevice::GetLevels621,
          &AYMDevice::GetLevels622,
          &AYMDevice::GetLevels623,
          &AYMDevice::GetLevels624,
          &AYMDevice::GetLevels625,
          &AYMDevice::GetLevels626,
          &AYMDevice::GetLevels627,
          &AYMDevice::GetLevels630,
          &AYMDevice::GetLevels631,
          &AYMDevice::GetLevels632,
          &AYMDevice::GetLevels633,
          &AYMDevice::GetLevels634,
          &AYMDevice::GetLevels635,
          &AYMDevice::GetLevels636,
          &AYMDevice::GetLevels637,
          &AYMDevice::GetLevels640,
          &AYMDevice::GetLevels641,
          &AYMDevice::GetLevels642,
          &AYMDevice::GetLevels643,
          &AYMDevice::GetLevels644,
          &AYMDevice::GetLevels645,
          &AYMDevice::GetLevels646,
          &AYMDevice::GetLevels647,
          &AYMDevice::GetLevels650,
          &AYMDevice::GetLevels651,
          &AYMDevice::GetLevels652,
          &AYMDevice::GetLevels653,
          &AYMDevice::GetLevels654,
          &AYMDevice::GetLevels655,
          &AYMDevice::GetLevels656,
          &AYMDevice::GetLevels657,
          &AYMDevice::GetLevels660,
          &AYMDevice::GetLevels661,
          &AYMDevice::GetLevels662,
          &AYMDevice::GetLevels663,
          &AYMDevice::GetLevels664,
          &AYMDevice::GetLevels665,
          &AYMDevice::GetLevels666,
          &AYMDevice::GetLevels667,
          &AYMDevice::GetLevels670,
          &AYMDevice::GetLevels671,
          &AYMDevice::GetLevels672,
          &AYMDevice::GetLevels673,
          &AYMDevice::GetLevels674,
          &AYMDevice::GetLevels675,
          &AYMDevice::GetLevels676,
          &AYMDevice::GetLevels677,
          &AYMDevice::GetLevels700,
          &AYMDevice::GetLevels701,
          &AYMDevice::GetLevels702,
          &AYMDevice::GetLevels703,
          &AYMDevice::GetLevels704,
          &AYMDevice::GetLevels705,
          &AYMDevice::GetLevels706,
          &AYMDevice::GetLevels707,
          &AYMDevice::GetLevels710,
          &AYMDevice::GetLevels711,
          &AYMDevice::GetLevels712,
          &AYMDevice::GetLevels713,
          &AYMDevice::GetLevels714,
          &AYMDevice::GetLevels715,
          &AYMDevice::GetLevels716,
          &AYMDevice::GetLevels717,
          &AYMDevice::GetLevels720,
          &AYMDevice::GetLevels721,
          &AYMDevice::GetLevels722,
          &AYMDevice::GetLevels723,
          &AYMDevice::GetLevels724,
          &AYMDevice::GetLevels725,
          &AYMDevice::GetLevels726,
          &AYMDevice::GetLevels727,
          &AYMDevice::GetLevels730,
          &AYMDevice::GetLevels731,
          &AYMDevice::GetLevels732,
          &AYMDevice::GetLevels733,
          &AYMDevice::GetLevels734,
          &AYMDevice::GetLevels735,
          &AYMDevice::GetLevels736,
          &AYMDevice::GetLevels737,
          &AYMDevice::GetLevels740,
          &AYMDevice::GetLevels741,
          &AYMDevice::GetLevels742,
          &AYMDevice::GetLevels743,
          &AYMDevice::GetLevels744,
          &AYMDevice::GetLevels745,
          &AYMDevice::GetLevels746,
          &AYMDevice::GetLevels747,
          &AYMDevice::GetLevels750,
          &AYMDevice::GetLevels751,
          &AYMDevice::GetLevels752,
          &AYMDevice::GetLevels753,
          &AYMDevice::GetLevels754,
          &AYMDevice::GetLevels755,
          &AYMDevice::GetLevels756,
          &AYMDevice::GetLevels757,
          &AYMDevice::GetLevels760,
          &AYMDevice::GetLevels761,
          &AYMDevice::GetLevels762,
          &AYMDevice::GetLevels763,
          &AYMDevice::GetLevels764,
          &AYMDevice::GetLevels765,
          &AYMDevice::GetLevels766,
          &AYMDevice::GetLevels767,
          &AYMDevice::GetLevels770,
          &AYMDevice::GetLevels771,
          &AYMDevice::GetLevels772,
          &AYMDevice::GetLevels773,
          &AYMDevice::GetLevels774,
          &AYMDevice::GetLevels775,
          &AYMDevice::GetLevels776,
          &AYMDevice::GetLevels777,
          
        };
        GetLevelsFunc = FUNCTIONS[State];
      }

      
      void GetLevels000(MultiSample& result) const
      {
        
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels001(MultiSample& result) const
      {
        
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels002(MultiSample& result) const
      {
        
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels003(MultiSample& result) const
      {
        
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels004(MultiSample& result) const
      {
        
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels005(MultiSample& result) const
      {
        
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels006(MultiSample& result) const
      {
        
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels007(MultiSample& result) const
      {
        
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels010(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels011(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels012(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels013(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels014(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels015(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels016(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels017(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels020(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels021(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels022(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels023(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels024(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels025(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels026(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels027(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels030(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels031(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels032(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels033(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels034(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels035(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels036(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels037(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels040(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels041(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels042(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels043(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels044(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels045(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels046(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels047(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels050(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels051(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels052(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels053(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels054(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels055(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels056(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels057(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels060(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels061(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels062(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels063(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels064(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels065(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels066(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels067(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels070(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels071(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels072(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels073(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels074(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels075(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels076(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels077(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels100(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels101(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels102(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels103(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels104(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels105(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels106(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels107(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels110(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels111(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels112(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels113(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels114(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels115(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels116(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels117(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels120(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels121(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels122(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels123(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels124(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels125(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels126(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels127(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels130(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels131(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels132(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels133(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels134(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels135(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels136(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels137(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels140(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels141(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels142(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels143(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels144(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels145(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels146(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels147(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels150(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels151(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels152(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels153(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels154(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels155(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels156(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels157(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels160(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels161(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels162(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels163(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels164(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels165(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels166(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels167(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels170(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels171(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels172(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels173(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels174(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels175(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels176(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels177(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels200(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels201(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels202(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels203(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels204(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels205(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels206(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels207(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels210(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels211(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels212(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels213(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels214(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels215(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels216(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels217(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels220(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels221(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels222(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels223(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels224(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels225(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels226(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels227(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels230(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels231(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels232(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels233(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels234(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels235(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels236(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels237(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels240(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels241(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels242(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels243(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels244(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels245(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels246(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels247(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels250(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels251(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels252(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels253(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels254(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels255(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels256(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels257(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels260(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels261(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels262(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels263(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels264(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels265(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels266(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels267(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels270(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels271(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels272(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels273(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels274(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels275(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels276(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels277(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels300(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels301(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels302(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels303(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels304(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels305(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels306(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels307(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels310(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels311(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels312(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels313(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels314(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels315(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels316(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels317(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels320(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels321(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels322(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels323(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels324(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels325(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels326(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels327(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels330(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels331(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels332(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels333(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels334(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels335(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels336(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels337(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels340(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels341(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels342(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels343(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels344(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels345(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels346(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels347(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels350(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels351(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels352(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels353(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels354(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels355(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels356(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels357(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels360(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels361(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels362(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels363(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels364(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels365(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels366(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels367(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels370(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels371(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels372(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels373(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels374(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels375(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels376(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels377(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = LevelC & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels400(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels401(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels402(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels403(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels404(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels405(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels406(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels407(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels410(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels411(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels412(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels413(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels414(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels415(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels416(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels417(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels420(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels421(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels422(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels423(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels424(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels425(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels426(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels427(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels430(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels431(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels432(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels433(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels434(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels435(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels436(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels437(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels440(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels441(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels442(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels443(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels444(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels445(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels446(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels447(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels450(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels451(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels452(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels453(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels454(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels455(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels456(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels457(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels460(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels461(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels462(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels463(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels464(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels465(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels466(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels467(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels470(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels471(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels472(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels473(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels474(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels475(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels476(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels477(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels500(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels501(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels502(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels503(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels504(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels505(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels506(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels507(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels510(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels511(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels512(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels513(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels514(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels515(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels516(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels517(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels520(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels521(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels522(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels523(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels524(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels525(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels526(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels527(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels530(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels531(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels532(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels533(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels534(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels535(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels536(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels537(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels540(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels541(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels542(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels543(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels544(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels545(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels546(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels547(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels550(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels551(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels552(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels553(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels554(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels555(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels556(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels557(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels560(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels561(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels562(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels563(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels564(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels565(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels566(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels567(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels570(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels571(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels572(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels573(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels574(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels575(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels576(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels577(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = LevelB & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels600(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels601(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels602(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels603(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels604(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels605(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels606(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels607(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels610(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels611(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels612(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels613(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels614(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels615(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels616(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels617(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels620(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels621(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels622(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels623(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels624(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels625(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels626(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels627(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels630(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels631(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels632(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels633(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels634(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels635(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels636(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels637(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels640(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels641(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels642(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels643(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels644(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels645(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels646(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels647(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels650(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels651(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels652(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels653(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels654(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels655(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels656(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels657(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels660(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels661(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels662(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels663(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels664(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels665(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels666(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels667(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels670(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels671(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels672(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels673(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels674(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels675(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels676(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels677(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = LevelA & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels700(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels701(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels702(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels703(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels704(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels705(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels706(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels707(MultiSample& result) const
      {
        
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels710(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels711(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels712(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels713(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels714(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels715(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels716(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels717(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels720(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels721(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels722(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels723(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels724(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels725(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels726(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels727(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels730(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels731(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels732(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels733(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels734(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels735(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels736(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels737(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels740(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels741(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels742(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels743(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels744(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels745(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels746(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels747(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels750(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels751(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels752(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels753(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels754(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels755(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels756(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels757(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels760(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels761(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels762(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels763(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels764(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels765(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels766(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels767(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels770(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels771(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels772(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels773(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels774(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels775(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels776(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      void GetLevels777(MultiSample& result) const
      {
        const uint_t noise = GenN.GetLevel();
        const uint_t envelope = GenE.GetLevel();
        const uint_t outA = envelope & noise & GenA.GetLevel();
        const uint_t outB = envelope & noise & GenB.GetLevel();
        const uint_t outC = envelope & noise & GenC.GetLevel();
        const VolTable& table = *VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
    private:
      ToneGenerator GenA;
      ToneGenerator GenB;
      ToneGenerator GenC;
      NoiseGenerator GenN;
      EnvelopeGenerator GenE;
      uint_t LevelA;
      uint_t LevelB;
      uint_t LevelC;
      const VolTable* VolumeTable;
      uint_t State;
      typedef void (AYMDevice::*GetLevelsFuncType)(MultiSample& result) const;
      GetLevelsFuncType GetLevelsFunc;
    };
  }
}

#endif //DEVICES_AYM_DEVICE_H_DEFINED
