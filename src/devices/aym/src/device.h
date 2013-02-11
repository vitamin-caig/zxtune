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
        , GeneratorIndex(0)
      {
        UpdateGenerator();
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
        GeneratorIndex = (GeneratorIndex & ~mixMask) | (~mixer & mixMask);
        UpdateGenerator();
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
        UpdateGenerator();
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
        GeneratorIndex = (GeneratorIndex & ~envMask) | envFlags;
        UpdateGenerator();
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
        GeneratorIndex = 0;
        UpdateGenerator();
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
        (*GetLevelsFunc)(*this, result);
      }
    private:
      void UpdateGenerator()
      {
        static const GetLevelsFuncType FUNCTIONS[] =
        {
          &GetLevels000,
          &GetLevels001,
          &GetLevels002,
          &GetLevels003,
          &GetLevels004,
          &GetLevels005,
          &GetLevels006,
          &GetLevels007,
          &GetLevels010,
          &GetLevels011,
          &GetLevels012,
          &GetLevels013,
          &GetLevels014,
          &GetLevels015,
          &GetLevels016,
          &GetLevels017,
          &GetLevels020,
          &GetLevels021,
          &GetLevels022,
          &GetLevels023,
          &GetLevels024,
          &GetLevels025,
          &GetLevels026,
          &GetLevels027,
          &GetLevels030,
          &GetLevels031,
          &GetLevels032,
          &GetLevels033,
          &GetLevels034,
          &GetLevels035,
          &GetLevels036,
          &GetLevels037,
          &GetLevels040,
          &GetLevels041,
          &GetLevels042,
          &GetLevels043,
          &GetLevels044,
          &GetLevels045,
          &GetLevels046,
          &GetLevels047,
          &GetLevels050,
          &GetLevels051,
          &GetLevels052,
          &GetLevels053,
          &GetLevels054,
          &GetLevels055,
          &GetLevels056,
          &GetLevels057,
          &GetLevels060,
          &GetLevels061,
          &GetLevels062,
          &GetLevels063,
          &GetLevels064,
          &GetLevels065,
          &GetLevels066,
          &GetLevels067,
          &GetLevels070,
          &GetLevels071,
          &GetLevels072,
          &GetLevels073,
          &GetLevels074,
          &GetLevels075,
          &GetLevels076,
          &GetLevels077,
          &GetLevels100,
          &GetLevels101,
          &GetLevels102,
          &GetLevels103,
          &GetLevels104,
          &GetLevels105,
          &GetLevels106,
          &GetLevels107,
          &GetLevels110,
          &GetLevels111,
          &GetLevels112,
          &GetLevels113,
          &GetLevels114,
          &GetLevels115,
          &GetLevels116,
          &GetLevels117,
          &GetLevels120,
          &GetLevels121,
          &GetLevels122,
          &GetLevels123,
          &GetLevels124,
          &GetLevels125,
          &GetLevels126,
          &GetLevels127,
          &GetLevels130,
          &GetLevels131,
          &GetLevels132,
          &GetLevels133,
          &GetLevels134,
          &GetLevels135,
          &GetLevels136,
          &GetLevels137,
          &GetLevels140,
          &GetLevels141,
          &GetLevels142,
          &GetLevels143,
          &GetLevels144,
          &GetLevels145,
          &GetLevels146,
          &GetLevels147,
          &GetLevels150,
          &GetLevels151,
          &GetLevels152,
          &GetLevels153,
          &GetLevels154,
          &GetLevels155,
          &GetLevels156,
          &GetLevels157,
          &GetLevels160,
          &GetLevels161,
          &GetLevels162,
          &GetLevels163,
          &GetLevels164,
          &GetLevels165,
          &GetLevels166,
          &GetLevels167,
          &GetLevels170,
          &GetLevels171,
          &GetLevels172,
          &GetLevels173,
          &GetLevels174,
          &GetLevels175,
          &GetLevels176,
          &GetLevels177,
          &GetLevels200,
          &GetLevels201,
          &GetLevels202,
          &GetLevels203,
          &GetLevels204,
          &GetLevels205,
          &GetLevels206,
          &GetLevels207,
          &GetLevels210,
          &GetLevels211,
          &GetLevels212,
          &GetLevels213,
          &GetLevels214,
          &GetLevels215,
          &GetLevels216,
          &GetLevels217,
          &GetLevels220,
          &GetLevels221,
          &GetLevels222,
          &GetLevels223,
          &GetLevels224,
          &GetLevels225,
          &GetLevels226,
          &GetLevels227,
          &GetLevels230,
          &GetLevels231,
          &GetLevels232,
          &GetLevels233,
          &GetLevels234,
          &GetLevels235,
          &GetLevels236,
          &GetLevels237,
          &GetLevels240,
          &GetLevels241,
          &GetLevels242,
          &GetLevels243,
          &GetLevels244,
          &GetLevels245,
          &GetLevels246,
          &GetLevels247,
          &GetLevels250,
          &GetLevels251,
          &GetLevels252,
          &GetLevels253,
          &GetLevels254,
          &GetLevels255,
          &GetLevels256,
          &GetLevels257,
          &GetLevels260,
          &GetLevels261,
          &GetLevels262,
          &GetLevels263,
          &GetLevels264,
          &GetLevels265,
          &GetLevels266,
          &GetLevels267,
          &GetLevels270,
          &GetLevels271,
          &GetLevels272,
          &GetLevels273,
          &GetLevels274,
          &GetLevels275,
          &GetLevels276,
          &GetLevels277,
          &GetLevels300,
          &GetLevels301,
          &GetLevels302,
          &GetLevels303,
          &GetLevels304,
          &GetLevels305,
          &GetLevels306,
          &GetLevels307,
          &GetLevels310,
          &GetLevels311,
          &GetLevels312,
          &GetLevels313,
          &GetLevels314,
          &GetLevels315,
          &GetLevels316,
          &GetLevels317,
          &GetLevels320,
          &GetLevels321,
          &GetLevels322,
          &GetLevels323,
          &GetLevels324,
          &GetLevels325,
          &GetLevels326,
          &GetLevels327,
          &GetLevels330,
          &GetLevels331,
          &GetLevels332,
          &GetLevels333,
          &GetLevels334,
          &GetLevels335,
          &GetLevels336,
          &GetLevels337,
          &GetLevels340,
          &GetLevels341,
          &GetLevels342,
          &GetLevels343,
          &GetLevels344,
          &GetLevels345,
          &GetLevels346,
          &GetLevels347,
          &GetLevels350,
          &GetLevels351,
          &GetLevels352,
          &GetLevels353,
          &GetLevels354,
          &GetLevels355,
          &GetLevels356,
          &GetLevels357,
          &GetLevels360,
          &GetLevels361,
          &GetLevels362,
          &GetLevels363,
          &GetLevels364,
          &GetLevels365,
          &GetLevels366,
          &GetLevels367,
          &GetLevels370,
          &GetLevels371,
          &GetLevels372,
          &GetLevels373,
          &GetLevels374,
          &GetLevels375,
          &GetLevels376,
          &GetLevels377,
          &GetLevels400,
          &GetLevels401,
          &GetLevels402,
          &GetLevels403,
          &GetLevels404,
          &GetLevels405,
          &GetLevels406,
          &GetLevels407,
          &GetLevels410,
          &GetLevels411,
          &GetLevels412,
          &GetLevels413,
          &GetLevels414,
          &GetLevels415,
          &GetLevels416,
          &GetLevels417,
          &GetLevels420,
          &GetLevels421,
          &GetLevels422,
          &GetLevels423,
          &GetLevels424,
          &GetLevels425,
          &GetLevels426,
          &GetLevels427,
          &GetLevels430,
          &GetLevels431,
          &GetLevels432,
          &GetLevels433,
          &GetLevels434,
          &GetLevels435,
          &GetLevels436,
          &GetLevels437,
          &GetLevels440,
          &GetLevels441,
          &GetLevels442,
          &GetLevels443,
          &GetLevels444,
          &GetLevels445,
          &GetLevels446,
          &GetLevels447,
          &GetLevels450,
          &GetLevels451,
          &GetLevels452,
          &GetLevels453,
          &GetLevels454,
          &GetLevels455,
          &GetLevels456,
          &GetLevels457,
          &GetLevels460,
          &GetLevels461,
          &GetLevels462,
          &GetLevels463,
          &GetLevels464,
          &GetLevels465,
          &GetLevels466,
          &GetLevels467,
          &GetLevels470,
          &GetLevels471,
          &GetLevels472,
          &GetLevels473,
          &GetLevels474,
          &GetLevels475,
          &GetLevels476,
          &GetLevels477,
          &GetLevels500,
          &GetLevels501,
          &GetLevels502,
          &GetLevels503,
          &GetLevels504,
          &GetLevels505,
          &GetLevels506,
          &GetLevels507,
          &GetLevels510,
          &GetLevels511,
          &GetLevels512,
          &GetLevels513,
          &GetLevels514,
          &GetLevels515,
          &GetLevels516,
          &GetLevels517,
          &GetLevels520,
          &GetLevels521,
          &GetLevels522,
          &GetLevels523,
          &GetLevels524,
          &GetLevels525,
          &GetLevels526,
          &GetLevels527,
          &GetLevels530,
          &GetLevels531,
          &GetLevels532,
          &GetLevels533,
          &GetLevels534,
          &GetLevels535,
          &GetLevels536,
          &GetLevels537,
          &GetLevels540,
          &GetLevels541,
          &GetLevels542,
          &GetLevels543,
          &GetLevels544,
          &GetLevels545,
          &GetLevels546,
          &GetLevels547,
          &GetLevels550,
          &GetLevels551,
          &GetLevels552,
          &GetLevels553,
          &GetLevels554,
          &GetLevels555,
          &GetLevels556,
          &GetLevels557,
          &GetLevels560,
          &GetLevels561,
          &GetLevels562,
          &GetLevels563,
          &GetLevels564,
          &GetLevels565,
          &GetLevels566,
          &GetLevels567,
          &GetLevels570,
          &GetLevels571,
          &GetLevels572,
          &GetLevels573,
          &GetLevels574,
          &GetLevels575,
          &GetLevels576,
          &GetLevels577,
          &GetLevels600,
          &GetLevels601,
          &GetLevels602,
          &GetLevels603,
          &GetLevels604,
          &GetLevels605,
          &GetLevels606,
          &GetLevels607,
          &GetLevels610,
          &GetLevels611,
          &GetLevels612,
          &GetLevels613,
          &GetLevels614,
          &GetLevels615,
          &GetLevels616,
          &GetLevels617,
          &GetLevels620,
          &GetLevels621,
          &GetLevels622,
          &GetLevels623,
          &GetLevels624,
          &GetLevels625,
          &GetLevels626,
          &GetLevels627,
          &GetLevels630,
          &GetLevels631,
          &GetLevels632,
          &GetLevels633,
          &GetLevels634,
          &GetLevels635,
          &GetLevels636,
          &GetLevels637,
          &GetLevels640,
          &GetLevels641,
          &GetLevels642,
          &GetLevels643,
          &GetLevels644,
          &GetLevels645,
          &GetLevels646,
          &GetLevels647,
          &GetLevels650,
          &GetLevels651,
          &GetLevels652,
          &GetLevels653,
          &GetLevels654,
          &GetLevels655,
          &GetLevels656,
          &GetLevels657,
          &GetLevels660,
          &GetLevels661,
          &GetLevels662,
          &GetLevels663,
          &GetLevels664,
          &GetLevels665,
          &GetLevels666,
          &GetLevels667,
          &GetLevels670,
          &GetLevels671,
          &GetLevels672,
          &GetLevels673,
          &GetLevels674,
          &GetLevels675,
          &GetLevels676,
          &GetLevels677,
          &GetLevels700,
          &GetLevels701,
          &GetLevels702,
          &GetLevels703,
          &GetLevels704,
          &GetLevels705,
          &GetLevels706,
          &GetLevels707,
          &GetLevels710,
          &GetLevels711,
          &GetLevels712,
          &GetLevels713,
          &GetLevels714,
          &GetLevels715,
          &GetLevels716,
          &GetLevels717,
          &GetLevels720,
          &GetLevels721,
          &GetLevels722,
          &GetLevels723,
          &GetLevels724,
          &GetLevels725,
          &GetLevels726,
          &GetLevels727,
          &GetLevels730,
          &GetLevels731,
          &GetLevels732,
          &GetLevels733,
          &GetLevels734,
          &GetLevels735,
          &GetLevels736,
          &GetLevels737,
          &GetLevels740,
          &GetLevels741,
          &GetLevels742,
          &GetLevels743,
          &GetLevels744,
          &GetLevels745,
          &GetLevels746,
          &GetLevels747,
          &GetLevels750,
          &GetLevels751,
          &GetLevels752,
          &GetLevels753,
          &GetLevels754,
          &GetLevels755,
          &GetLevels756,
          &GetLevels757,
          &GetLevels760,
          &GetLevels761,
          &GetLevels762,
          &GetLevels763,
          &GetLevels764,
          &GetLevels765,
          &GetLevels766,
          &GetLevels767,
          &GetLevels770,
          &GetLevels771,
          &GetLevels772,
          &GetLevels773,
          &GetLevels774,
          &GetLevels775,
          &GetLevels776,
          &GetLevels777,
          
        };
        const uint_t realGenerator = GenE.Stopped() ? (GeneratorIndex & 63) : GeneratorIndex;
        GetLevelsFunc = FUNCTIONS[realGenerator];
      }

      
      static void GetLevels000(const AYMDevice& self, MultiSample& result)
      {
        
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels001(const AYMDevice& self, MultiSample& result)
      {
        
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels002(const AYMDevice& self, MultiSample& result)
      {
        
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels003(const AYMDevice& self, MultiSample& result)
      {
        
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels004(const AYMDevice& self, MultiSample& result)
      {
        
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels005(const AYMDevice& self, MultiSample& result)
      {
        
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels006(const AYMDevice& self, MultiSample& result)
      {
        
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels007(const AYMDevice& self, MultiSample& result)
      {
        
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels010(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels011(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels012(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels013(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels014(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels015(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels016(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels017(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels020(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels021(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels022(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels023(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels024(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels025(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels026(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels027(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels030(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels031(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels032(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels033(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels034(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels035(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels036(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels037(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels040(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels041(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels042(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels043(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels044(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels045(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels046(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels047(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels050(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels051(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels052(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels053(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels054(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels055(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels056(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels057(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels060(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels061(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels062(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels063(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels064(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels065(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels066(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels067(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels070(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels071(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels072(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels073(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels074(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels075(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels076(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels077(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels100(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels101(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels102(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels103(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels104(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels105(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels106(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels107(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels110(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels111(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels112(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels113(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels114(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels115(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels116(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels117(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels120(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels121(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels122(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels123(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels124(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels125(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels126(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels127(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels130(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels131(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels132(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels133(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels134(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels135(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels136(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels137(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels140(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels141(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels142(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels143(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels144(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels145(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels146(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels147(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels150(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels151(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels152(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels153(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels154(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels155(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels156(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels157(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels160(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels161(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels162(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels163(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels164(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels165(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels166(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels167(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels170(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels171(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels172(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels173(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels174(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels175(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels176(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels177(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels200(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels201(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels202(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels203(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels204(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels205(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels206(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels207(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels210(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels211(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels212(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels213(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels214(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels215(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels216(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels217(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels220(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels221(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels222(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels223(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels224(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels225(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels226(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels227(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels230(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels231(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels232(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels233(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels234(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels235(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels236(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels237(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels240(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels241(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels242(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels243(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels244(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels245(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels246(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels247(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels250(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels251(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels252(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels253(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels254(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels255(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels256(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels257(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels260(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels261(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels262(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels263(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels264(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels265(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels266(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels267(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels270(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels271(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels272(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels273(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels274(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels275(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels276(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels277(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels300(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels301(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels302(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels303(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels304(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels305(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels306(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels307(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels310(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels311(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels312(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels313(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels314(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels315(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels316(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels317(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels320(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels321(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels322(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels323(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels324(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels325(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels326(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels327(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels330(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels331(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels332(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels333(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels334(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels335(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels336(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels337(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels340(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels341(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels342(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels343(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels344(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels345(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels346(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels347(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels350(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels351(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels352(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels353(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels354(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels355(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels356(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels357(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels360(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels361(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels362(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels363(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels364(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels365(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels366(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels367(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels370(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels371(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels372(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels373(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels374(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels375(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels376(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels377(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = self.LevelC & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels400(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels401(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels402(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels403(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels404(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels405(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels406(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels407(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels410(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels411(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels412(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels413(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels414(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels415(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels416(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels417(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels420(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels421(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels422(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels423(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels424(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels425(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels426(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels427(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels430(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels431(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels432(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels433(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels434(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels435(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels436(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels437(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels440(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels441(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels442(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels443(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels444(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels445(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels446(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels447(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels450(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels451(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels452(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels453(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels454(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels455(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels456(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels457(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels460(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels461(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels462(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels463(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels464(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels465(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels466(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels467(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels470(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels471(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels472(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels473(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels474(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels475(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels476(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels477(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels500(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels501(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels502(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels503(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels504(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels505(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels506(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels507(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels510(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels511(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels512(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels513(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels514(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels515(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels516(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels517(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels520(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels521(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels522(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels523(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels524(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels525(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels526(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels527(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels530(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels531(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels532(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels533(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels534(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels535(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels536(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels537(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels540(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels541(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels542(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels543(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels544(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels545(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels546(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels547(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels550(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels551(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels552(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels553(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels554(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels555(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels556(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels557(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels560(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels561(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels562(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels563(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels564(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels565(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels566(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels567(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels570(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels571(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels572(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels573(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels574(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels575(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels576(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels577(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = self.LevelB & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels600(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels601(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels602(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels603(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels604(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels605(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels606(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels607(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels610(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels611(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels612(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels613(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels614(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels615(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels616(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels617(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels620(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels621(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels622(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels623(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels624(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels625(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels626(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels627(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels630(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels631(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels632(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels633(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels634(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels635(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels636(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels637(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels640(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels641(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels642(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels643(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels644(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels645(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels646(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels647(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels650(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels651(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels652(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels653(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels654(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels655(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels656(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels657(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels660(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels661(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels662(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels663(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels664(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels665(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels666(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels667(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels670(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels671(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels672(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels673(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels674(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels675(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels676(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels677(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = self.LevelA & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels700(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels701(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels702(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels703(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels704(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels705(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels706(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels707(const AYMDevice& self, MultiSample& result)
      {
        
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels710(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels711(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels712(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels713(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels714(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels715(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels716(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels717(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels720(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels721(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels722(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels723(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels724(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels725(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels726(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels727(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels730(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels731(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels732(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels733(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels734(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels735(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels736(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels737(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels740(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels741(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels742(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels743(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels744(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels745(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels746(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels747(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels750(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels751(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels752(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels753(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels754(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels755(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels756(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels757(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels760(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels761(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels762(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels763(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels764(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels765(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels766(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels767(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels770(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels771(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels772(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels773(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise;
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels774(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels775(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise;
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels776(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise;
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
        result[0] = table[outA];
        result[1] = table[outB];
        result[2] = table[outC];
      }
      
      static void GetLevels777(const AYMDevice& self, MultiSample& result)
      {
        const uint_t noise = self.GenN.GetLevel();
        const uint_t envelope = self.GenE.GetLevel();
        const uint_t outA = envelope & noise & self.GenA.GetLevel();
        const uint_t outB = envelope & noise & self.GenB.GetLevel();
        const uint_t outC = envelope & noise & self.GenC.GetLevel();
        const VolTable& table = *self.VolumeTable;
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
      uint_t GeneratorIndex;
      typedef void (*GetLevelsFuncType)(const AYMDevice& self, MultiSample& result);
      GetLevelsFuncType GetLevelsFunc;
    };
  }
}

#endif //DEVICES_AYM_DEVICE_H_DEFINED
