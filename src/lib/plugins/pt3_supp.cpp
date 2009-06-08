#include "plugin_enumerator.h"

#include "detector.h"
#include "tracking_supp.h"
#include "../devices/data_source.h"
#include "../devices/aym/aym.h"

#include "../io/container.h"
#include "../io/warnings_collector.h"

#include <tools.h>

#include <player_attrs.h>

#include <boost/static_assert.hpp>

#include <cassert>
#include <valarray>

namespace
{
  using namespace ZXTune;

  const String TEXT_PT3_INFO("ProTracker v3 modules support");
  const String TEXT_PT3_VERSION("0.1");

  const std::size_t LIMITER(~std::size_t(0));

  const std::size_t MAX_MODULE_SIZE = 1 << 16;
  const std::size_t MAX_PATTERNS_COUNT = 48;
  const std::size_t MAX_PATTERN_SIZE = 64;
  const std::size_t MAX_SAMPLES_COUNT = 32;

  typedef IO::FastDump<uint8_t> FastDump;


  //pt3
  const std::size_t PT3X_PLAYER_SIZE = 0xe21;
  const std::string PT3XPlayerDetector("21??18?c3??c3+35+f322??22??22??22??01640009");

  const std::size_t PT35X_PLAYER_SIZE = 0x30f;//0xce
  const std::string PT35XPlayerDetector("21??18?c3??c3+37+f3ed73??22??22??22??22??01640009");

  //Table #0 of Pro Tracker 3.3x - 3.4r
  const uint16_t FreqTable_PT_33_34r[96] = {
    0x0c21, 0x0b73, 0x0ace, 0x0a33, 0x09a0, 0x0916, 0x0893, 0x0818, 0x07a4, 0x0736, 0x06ce, 0x066d,
    0x0610, 0x05b9, 0x0567, 0x0519, 0x04d0, 0x048b, 0x0449, 0x040c, 0x03d2, 0x039b, 0x0367, 0x0336,
    0x0308, 0x02dc, 0x02b3, 0x028c, 0x0268, 0x0245, 0x0224, 0x0206, 0x01e9, 0x01cd, 0x01b3, 0x019b,
    0x0184, 0x016e, 0x0159, 0x0146, 0x0134, 0x0122, 0x0112, 0x0103, 0x00f4, 0x00e6, 0x00d9, 0x00cd,
    0x00c2, 0x00b7, 0x00ac, 0x00a3, 0x009a, 0x0091, 0x0089, 0x0081, 0x007a, 0x0073, 0x006c, 0x0066,
    0x0061, 0x005b, 0x0056, 0x0051, 0x004d, 0x0048, 0x0044, 0x0040, 0x003d, 0x0039, 0x0036, 0x0033,
    0x0030, 0x002d, 0x002b, 0x0028, 0x0026, 0x0024, 0x0022, 0x0020, 0x001e, 0x001c, 0x001b, 0x0019,
    0x0018, 0x0016, 0x0015, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f, 0x000e, 0x000d, 0x000c
  };

  //Table #0 of Pro Tracker 3.4x - 3.5x
  const uint16_t FreqTable_PT_34_35[96] = {
    0x0c22, 0x0b73, 0x0acf, 0x0a33, 0x09a1, 0x0917, 0x0894, 0x0819, 0x07a4, 0x0737, 0x06cf, 0x066d,
    0x0611, 0x05ba, 0x0567, 0x051a, 0x04d0, 0x048b, 0x044a, 0x040c, 0x03d2, 0x039b, 0x0367, 0x0337,
    0x0308, 0x02dd, 0x02b4, 0x028d, 0x0268, 0x0246, 0x0225, 0x0206, 0x01e9, 0x01ce, 0x01b4, 0x019b,
    0x0184, 0x016e, 0x015a, 0x0146, 0x0134, 0x0123, 0x0112, 0x0103, 0x00f5, 0x00e7, 0x00da, 0x00ce,
    0x00c2, 0x00b7, 0x00ad, 0x00a3, 0x009a, 0x0091, 0x0089, 0x0082, 0x007a, 0x0073, 0x006d, 0x0067,
    0x0061, 0x005c, 0x0056, 0x0052, 0x004d, 0x0049, 0x0045, 0x0041, 0x003d, 0x003a, 0x0036, 0x0033,
    0x0031, 0x002e, 0x002b, 0x0029, 0x0027, 0x0024, 0x0022, 0x0020, 0x001f, 0x001d, 0x001b, 0x001a,
    0x0018, 0x0017, 0x0016, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f, 0x000e, 0x000d, 0x000c
  };

  //Table #1 of Pro Tracker 3.3x - 3.5x
  const uint16_t FreqTable_ST[96] = {
    0x0ef8, 0x0e10, 0x0d60, 0x0c80, 0x0bd8, 0x0b28, 0x0a88, 0x09f0, 0x0960, 0x08e0, 0x0858, 0x07e0,
    0x077c, 0x0708, 0x06b0, 0x0640, 0x05ec, 0x0594, 0x0544, 0x04f8, 0x04b0, 0x0470, 0x042c, 0x03fd,
    0x03be, 0x0384, 0x0358, 0x0320, 0x02f6, 0x02ca, 0x02a2, 0x027c, 0x0258, 0x0238, 0x0216, 0x01f8,
    0x01df, 0x01c2, 0x01ac, 0x0190, 0x017b, 0x0165, 0x0151, 0x013e, 0x012c, 0x011c, 0x010a, 0x00fc,
    0x00ef, 0x00e1, 0x00d6, 0x00c8, 0x00bd, 0x00b2, 0x00a8, 0x009f, 0x0096, 0x008e, 0x0085, 0x007e,
    0x0077, 0x0070, 0x006b, 0x0064, 0x005e, 0x0059, 0x0054, 0x004f, 0x004b, 0x0047, 0x0042, 0x003f,
    0x003b, 0x0038, 0x0035, 0x0032, 0x002f, 0x002c, 0x002a, 0x0027, 0x0025, 0x0023, 0x0021, 0x001f,
    0x001d, 0x001c, 0x001a, 0x0019, 0x0017, 0x0016, 0x0015, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f
  };

  //Table #2 of Pro Tracker 3.4r
  const uint16_t FreqTable_ASM_34r[96] = {
    0x0d3e, 0x0c80, 0x0bcc, 0x0b22, 0x0a82, 0x09ec, 0x095c, 0x08d6, 0x0858, 0x07e0, 0x076e, 0x0704,
    0x069f, 0x0640, 0x05e6, 0x0591, 0x0541, 0x04f6, 0x04ae, 0x046b, 0x042c, 0x03f0, 0x03b7, 0x0382,
    0x034f, 0x0320, 0x02f3, 0x02c8, 0x02a1, 0x027b, 0x0257, 0x0236, 0x0216, 0x01f8, 0x01dc, 0x01c1,
    0x01a8, 0x0190, 0x0179, 0x0164, 0x0150, 0x013d, 0x012c, 0x011b, 0x010b, 0x00fc, 0x00ee, 0x00e0,
    0x00d4, 0x00c8, 0x00bd, 0x00b2, 0x00a8, 0x009f, 0x0096, 0x008d, 0x0085, 0x007e, 0x0077, 0x0070,
    0x006a, 0x0064, 0x005e, 0x0059, 0x0054, 0x0050, 0x004b, 0x0047, 0x0043, 0x003f, 0x003c, 0x0038,
    0x0035, 0x0032, 0x002f, 0x002d, 0x002a, 0x0028, 0x0026, 0x0024, 0x0022, 0x0020, 0x001e, 0x001d,
    0x001b, 0x001a, 0x0019, 0x0018, 0x0015, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f, 0x000e
  };

  //Table #2 of Pro Tracker 3.4x - 3.5x
  const uint16_t FreqTable_ASM_34_35[96] = {
    0x0d10, 0x0c55, 0x0ba4, 0x0afc, 0x0a5f, 0x09ca, 0x093d, 0x08b8, 0x083b, 0x07c5, 0x0755, 0x06ec,
    0x0688, 0x062a, 0x05d2, 0x057e, 0x052f, 0x04e5, 0x049e, 0x045c, 0x041d, 0x03e2, 0x03ab, 0x0376,
    0x0344, 0x0315, 0x02e9, 0x02bf, 0x0298, 0x0272, 0x024f, 0x022e, 0x020f, 0x01f1, 0x01d5, 0x01bb,
    0x01a2, 0x018b, 0x0174, 0x0160, 0x014c, 0x0139, 0x0128, 0x0117, 0x0107, 0x00f9, 0x00eb, 0x00dd,
    0x00d1, 0x00c5, 0x00ba, 0x00b0, 0x00a6, 0x009d, 0x0094, 0x008c, 0x0084, 0x007c, 0x0075, 0x006f,
    0x0069, 0x0063, 0x005d, 0x0058, 0x0053, 0x004e, 0x004a, 0x0046, 0x0042, 0x003e, 0x003b, 0x0037,
    0x0034, 0x0031, 0x002f, 0x002c, 0x0029, 0x0027, 0x0025, 0x0023, 0x0021, 0x001f, 0x001d, 0x001c,
    0x001a, 0x0019, 0x0017, 0x0016, 0x0015, 0x0014, 0x0012, 0x0011, 0x0010, 0x000f, 0x000e, 0x000d
  };

  //Table #3 of Pro Tracker 3.4r
  const uint16_t FreqTable_REAL_34r[96] = {
    0x0cda, 0x0c22, 0x0b73, 0x0acf, 0x0a33, 0x09a1, 0x0917, 0x0894, 0x0819, 0x07a4, 0x0737, 0x06cf,
    0x066d, 0x0611, 0x05ba, 0x0567, 0x051a, 0x04d0, 0x048b, 0x044a, 0x040c, 0x03d2, 0x039b, 0x0367,
    0x0337, 0x0308, 0x02dd, 0x02b4, 0x028d, 0x0268, 0x0246, 0x0225, 0x0206, 0x01e9, 0x01ce, 0x01b4,
    0x019b, 0x0184, 0x016e, 0x015a, 0x0146, 0x0134, 0x0123, 0x0113, 0x0103, 0x00f5, 0x00e7, 0x00da,
    0x00ce, 0x00c2, 0x00b7, 0x00ad, 0x00a3, 0x009a, 0x0091, 0x0089, 0x0082, 0x007a, 0x0073, 0x006d,
    0x0067, 0x0061, 0x005c, 0x0056, 0x0052, 0x004d, 0x0049, 0x0045, 0x0041, 0x003d, 0x003a, 0x0036,
    0x0033, 0x0031, 0x002e, 0x002b, 0x0029, 0x0027, 0x0024, 0x0022, 0x0020, 0x001f, 0x001d, 0x001b,
    0x001a, 0x0018, 0x0017, 0x0016, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f, 0x000e, 0x000d
  };

  //Table #3 of Pro Tracker 3.4x - 3.5x
  const uint16_t FreqTable_REAL_34_35[96] = {
    0x0cda, 0x0c22, 0x0b73, 0x0acf, 0x0a33, 0x09a1, 0x0917, 0x0894, 0x0819, 0x07a4, 0x0737, 0x06cf,
    0x066d, 0x0611, 0x05ba, 0x0567, 0x051a, 0x04d0, 0x048b, 0x044a, 0x040c, 0x03d2, 0x039b, 0x0367,
    0x0337, 0x0308, 0x02dd, 0x02b4, 0x028d, 0x0268, 0x0246, 0x0225, 0x0206, 0x01e9, 0x01ce, 0x01b4,
    0x019b, 0x0184, 0x016e, 0x015a, 0x0146, 0x0134, 0x0123, 0x0112, 0x0103, 0x00f5, 0x00e7, 0x00da,
    0x00ce, 0x00c2, 0x00b7, 0x00ad, 0x00a3, 0x009a, 0x0091, 0x0089, 0x0082, 0x007a, 0x0073, 0x006d,
    0x0067, 0x0061, 0x005c, 0x0056, 0x0052, 0x004d, 0x0049, 0x0045, 0x0041, 0x003d, 0x003a, 0x0036,
    0x0033, 0x0031, 0x002e, 0x002b, 0x0029, 0x0027, 0x0024, 0x0022, 0x0020, 0x001f, 0x001d, 0x001b,
    0x001a, 0x0018, 0x0017, 0x0016, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f, 0x000e, 0x000d
  };

  //Volume table of Pro Tracker 3.3x - 3.4x
  const uint8_t VolumeTable_33_34[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09,
    0x00, 0x00, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0a,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x09, 0x0a, 0x0b,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
  };

  //Volume table of Pro Tracker 3.5x
  const uint8_t VolumeTable_35[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07,
    0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08,
    0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x08, 0x09,
    0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x09, 0x0a,
    0x00, 0x01, 0x01, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b,
    0x00, 0x01, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b, 0x0c,
    0x00, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b, 0x0c, 0x0d,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
  };

  //////////////////////////////////////////////////////////////////////////
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  enum
  {
    FREQ_TABLE_PT,
    FREQ_TABLE_ST,
    FREQ_TABLE_ASM,
    FREQ_TABLE_REAL
  };

  PACK_PRE struct PT3Header
  {
    uint8_t Id[13];        //'ProTracker 3.'
    uint8_t Subversion;
    uint8_t Optional1[16]; //' compilation of '
    uint8_t TrackName[32];
    uint8_t Optional2[4]; //' by '
    uint8_t TrackAuthor[32];
    uint8_t Optional3;
    uint8_t FreqTableNum;
    uint8_t Tempo;
    uint8_t Lenght;
    uint8_t Loop;
    uint16_t PatternsOffset;
    uint16_t SamplesOffsets[32];
    uint16_t OrnamentsOffsets[16];
    uint8_t Positions[1]; //finished by marker
  } PACK_POST;

  PACK_PRE struct PT3Pattern
  {
    uint16_t Offsets[3];

    operator bool () const
    {
      return Offsets[0] && Offsets[1] && Offsets[2];
    }
  } PACK_POST;

  PACK_PRE struct PT3Sample
  {
    uint8_t Loop;
    uint8_t Size;
    PACK_PRE struct Line
    {
      uint8_t EnvMask : 1;
      uint8_t NoiseOrEnvOffset : 5;
      uint8_t VolSlideUp : 1;
      uint8_t VolSlide : 1;
      uint8_t Level : 4;
      uint8_t ToneMask : 1;
      uint8_t KeepNoiseOrEnvOffset : 1;
      uint8_t KeepToneOffset : 1;
      uint8_t NoiseMask : 1;
      int16_t ToneOffset;
    } PACK_POST;
    Line Data[1];
  } PACK_POST;

  PACK_PRE struct PT3Ornament
  {
    uint8_t Loop;
    uint8_t Size;
    int8_t Data[1];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(PT3Header) == 202);
  BOOST_STATIC_ASSERT(sizeof(PT3Pattern) == 6);
  BOOST_STATIC_ASSERT(sizeof(PT3Sample) == 6);
  BOOST_STATIC_ASSERT(sizeof(PT3Ornament) == 3);

  struct Sample
  {
    Sample() : Loop(), Data()
    {
    }

    Sample(std::size_t size, std::size_t loop) : Loop(loop), Data(size)
    {
    }

    std::size_t Loop;

    struct Line
    {
      unsigned Level;//0-15
      signed VolSlideAddon;

      bool ToneMask;
      signed ToneOffset;
      bool KeepToneOffset;

      bool NoiseMask;
      unsigned NoiseOffset;
      bool KeepNoiseOffset;

      bool EnvMask;
      signed EnvOffset;
      bool KeepEnvOffset;
    };
    std::vector<Line> Data;
  };

  class SampleCreator : public std::unary_function<uint16_t, Sample>
  {
  public:
    explicit SampleCreator(const FastDump& data) : Data(data)
    {
    }

    SampleCreator(const SampleCreator& rh) : Data(rh.Data)
    {
    }

    result_type operator () (const argument_type arg) const
    {
      const PT3Sample* const sample(safe_ptr_cast<const PT3Sample*>(&Data[fromLE(arg)]));
      if (0 == arg || !sample->Size)
      {
        return result_type(1, 0);//safe
      }
      result_type tmp(sample->Size, sample->Loop);
      for (std::size_t idx = 0; idx != sample->Size; ++idx)
      {
        const PT3Sample::Line& src(sample->Data[idx]);
        Sample::Line& dst(tmp.Data[idx]);
        dst.NoiseMask = src.NoiseMask;
        dst.ToneMask = src.ToneMask;
        dst.EnvMask = src.EnvMask;
        dst.KeepToneOffset = src.KeepToneOffset;
        if (src.NoiseMask)
        {
          dst.EnvOffset =  static_cast<int8_t>(src.NoiseOrEnvOffset & 16 ? src.NoiseOrEnvOffset | 0xf8 : src.NoiseOrEnvOffset);
          dst.KeepEnvOffset = src.KeepNoiseOrEnvOffset;
        }
        else
        {
          dst.NoiseOffset = src.NoiseOrEnvOffset;
          dst.KeepNoiseOffset = src.KeepNoiseOrEnvOffset;
        }
        dst.Level = src.Level;
        dst.VolSlideAddon = src.VolSlide ? (src.VolSlideUp ? +1 : -1) : 0;
        dst.ToneOffset = fromLE(src.ToneOffset);
      }
      return tmp;
    }
  private:
    const FastDump& Data;
  };

  enum CmdType
  {
    EMPTY,
    GLISS,        //2p
    GLISS_NOTE,   //3p
    SAMPLEOFFSET, //1p
    ORNAMENTOFFSET,//1p
    VIBRATE,      //2p
    SLIDEENV,     //2p
    NOENVELOPE,   //0p
    ENVELOPE,     //2p
    NOISEBASE,    //1p
    TEMPO,        //1p - pseudo-effect
  };

  void Describing(ModulePlayer::Info& info);

  typedef Log::WarningsCollector::AutoPrefixParam<std::size_t> IndexPrefix;

  class PlayerImpl : public Tracking::TrackPlayer<3, Sample>
  {
    typedef Tracking::TrackPlayer<3, Sample> Parent;

    class OrnamentCreator : public std::unary_function<uint16_t, Parent::Ornament>
    {
    public:
      explicit OrnamentCreator(const FastDump& data) : Data(data)
      {
      }

      OrnamentCreator(const OrnamentCreator& rh) : Data(rh.Data)
      {
      }

      result_type operator () (const argument_type arg) const
      {
        if (0 == arg || arg >= Data.Size())
        {
          return result_type(1, 0);//safe version
        }
        const PT3Ornament* const ornament(safe_ptr_cast<const PT3Ornament*>(&Data[fromLE(arg)]));
        result_type tmp;
        tmp.Loop = ornament->Loop;
        tmp.Data.assign(ornament->Data, ornament->Data + (ornament->Size ? ornament->Size : 1));
        return tmp;
      }
    private:
      const FastDump& Data;
    };

    static void ParsePattern(const FastDump& data, std::vector<std::size_t>& offsets, Parent::Line& line,
      std::valarray<std::size_t>& periods,
      std::valarray<std::size_t>& counters,
      Log::WarningsCollector& warner)
    {
      for (std::size_t chan = 0; chan != line.Channels.size(); ++chan)
      {
        if (counters[chan]--)
        {
          continue;//has to skip
        }
        IndexPrefix pfx(warner, "Channel %1%: ", chan);
        Line::Chan& channel(line.Channels[chan]);
        for (;;)
        {
          const uint8_t cmd(data[offsets[chan]++]);
          if (cmd == 1)//gliss
          {
            channel.Commands.push_back(Parent::Command(GLISS));
          }
          else if (cmd == 2)//portamento
          {
            channel.Commands.push_back(Parent::Command(GLISS_NOTE));
          }
          else if (cmd == 3)//sample offset
          {
            channel.Commands.push_back(Parent::Command(SAMPLEOFFSET, -1));
          }
          else if (cmd == 4)//ornament offset
          {
            channel.Commands.push_back(Parent::Command(ORNAMENTOFFSET, -1));
          }
          else if (cmd == 5)//vibrate
          {
            channel.Commands.push_back(Parent::Command(VIBRATE));
          }
          else if (cmd == 8)//slide envelope
          {
            channel.Commands.push_back(Parent::Command(SLIDEENV));
          }
          else if (cmd == 9)//tempo
          {
            channel.Commands.push_back(Parent::Command(TEMPO));
          }
          else if (cmd == 0x10 || cmd >= 0xf0)
          {
            const uint8_t doubleSampNum(data[offsets[chan]++]);
            warner.Assert(doubleSampNum <= MAX_SAMPLES_COUNT * 2 && 0 == (doubleSampNum & 1), "invalid sample index");
            warner.Assert(!channel.SampleNum, "duplicated sample");
            channel.SampleNum = doubleSampNum / 2;
            if (cmd != 0x10)
            {
              warner.Assert(!channel.OrnamentNum, "duplicated ornament");
              channel.OrnamentNum = cmd - 0xf0;
            }
            else
            {
              channel.Commands.push_back(Parent::Command(ORNAMENTOFFSET, 0));
            }
            channel.Commands.push_back(Parent::Command(NOENVELOPE));
          }
          else if ((cmd >= 0x11 && cmd <= 0x1f) || (cmd >= 0xb2 && cmd <= 0xbf))
          {
            const uint16_t envPeriod(data[offsets[chan] + 1] + (uint16_t(data[offsets[chan]]) << 8));
            offsets[chan] += 2;
            if (cmd >= 0x11 && cmd <= 0x1f)
            {
              channel.Commands.push_back(Parent::Command(ENVELOPE, cmd - 0x10, envPeriod));
              const uint8_t doubleSampNum(data[offsets[chan]++]);
              warner.Assert(doubleSampNum <= MAX_SAMPLES_COUNT * 2 && 0 == (doubleSampNum & 1), "invalid sample index");
              warner.Assert(!channel.SampleNum, "invalid sample");
              channel.SampleNum = doubleSampNum / 2;
            }
            else
            {
              channel.Commands.push_back(Parent::Command(ENVELOPE, cmd - 0xb1, envPeriod));
            }
            channel.Commands.push_back(Parent::Command(ORNAMENTOFFSET, 0));
          }
          else if (cmd >= 0x20 && cmd <= 0x3f)
          {
            channel.Commands.push_back(Parent::Command(NOISEBASE, cmd - 0x20));
            //warning
            warner.Assert(chan == 2, "noise base in invalid channel");
          }
          else if (cmd >= 0x40 && cmd <= 0x4f)
          {
            warner.Assert(!channel.OrnamentNum, "duplicated ornament");
            channel.OrnamentNum = cmd - 0x40;
          }
          else if (cmd >= 0x50 && cmd <= 0xaf)
          {
            Parent::CommandsArray::iterator it(std::find(channel.Commands.begin(), channel.Commands.end(), GLISS_NOTE));
            if (channel.Commands.end() != it)
            {
              it->Param3 = cmd - 0x50;
            }
            else
            {
              warner.Assert(!channel.Note, "duplicated note");
              channel.Note = cmd - 0x50;
            }
            warner.Assert(!channel.Enabled, "duplicated channel state");
            channel.Enabled = true;
            break;
          }
          else if (cmd == 0xb0)
          {
            channel.Commands.push_back(Parent::Command(NOENVELOPE));
            channel.Commands.push_back(Parent::Command(ORNAMENTOFFSET, 0));
          }
          else if (cmd == 0xb1)
          {
            periods[chan] = data[offsets[chan]++] - 1;
          }
          else if (cmd == 0xc0)
          {
            warner.Assert(!channel.Enabled, "duplicated channel state");
            channel.Enabled = false;
            break;
          }
          else if (cmd >= 0xc1 && cmd <= 0xcf)
          {
            warner.Assert(!channel.Volume, "duplicated volume");
            channel.Volume = cmd - 0xc0;
          }
          else if (cmd == 0xd0)
          {
            break;
          }
          else if (cmd >= 0xd1 && cmd <= 0xef)
          {
            warner.Assert(!channel.SampleNum, "duplicated sample");
            channel.SampleNum = cmd - 0xd0;
          }
        }
        //parse parameters
        for (Parent::CommandsArray::reverse_iterator it = channel.Commands.rbegin(), lim = channel.Commands.rend();
          it != lim; ++it)
        {
          switch (it->Type)
          {
          case TEMPO:
            //warning
            warner.Assert(!line.Tempo, "duplicated tempo");
            line.Tempo = data[offsets[chan]++];
            break;
          case SLIDEENV:
          case GLISS:
            it->Param1 = data[offsets[chan]++];
            it->Param2 = data[offsets[chan]] + (int16_t(static_cast<int8_t>(data[offsets[chan] + 1])) << 8);
            offsets[chan] += 2;
            break;
          case VIBRATE:
            it->Param1 = data[offsets[chan]++];
            it->Param2 = data[offsets[chan]++];
            break;
          case ORNAMENTOFFSET:
          case SAMPLEOFFSET:
            if (-1 == it->Param1)
            {
              it->Param1 = data[offsets[chan]++];
            }
            break;
          case GLISS_NOTE:
            it->Param1 = data[offsets[chan]++];
            it->Param2 = data[offsets[chan]] + (int16_t(static_cast<int8_t>(data[offsets[chan] + 1])) << 8);
            offsets[chan] += 4;
            break;
          }
        }
        counters[chan] = periods[chan];
      }
    }

  private:
    struct Slider
    {
      Slider() : Period(), Value(), Counter(), Delta()
      {
      }
      std::size_t Period;
      signed Value;
      std::size_t Counter;
      signed Delta;

      bool Update()
      {
        if (Counter && !--Counter)
        {
          Value += Delta;
          Counter = Period;
          return true;
        }
        return false;
      }

      void Reset()
      {
        Counter = 0;
        Value = 0;
      }
    };
    struct ChannelState
    {
      ChannelState()
        : Enabled(false), Envelope(false)
        , Note(), SampleNum(0), PosInSample(0)
        , OrnamentNum(0), PosInOrnament(0)
        , Volume(15), VolSlide(0)
        , ToneSlider(), SlidingTargetNote(LIMITER), ToneAccumulator(0)
        , EnvSliding(), NoiseSliding()
        , VibrateCounter(0), VibrateOn(), VibrateOff()
      {
      }

      bool Enabled;
      bool Envelope;
      std::size_t Note;
      std::size_t SampleNum;
      std::size_t PosInSample;
      std::size_t OrnamentNum;
      std::size_t PosInOrnament;
      std::size_t Volume;
      signed VolSlide;
      Slider ToneSlider;
      std::size_t SlidingTargetNote;
      signed ToneAccumulator;
      signed EnvSliding;
      signed NoiseSliding;
      std::size_t VibrateCounter;
      std::size_t VibrateOn;
      std::size_t VibrateOff;
    };
    struct CommonState
    {
      std::size_t EnvBase;
      Slider EnvSlider;
      std::size_t NoiseBase;
      signed NoiseAddon;
    };
  public:
    PlayerImpl(const String& filename, const FastDump& data)
      : Device(AYM::CreateChip())//TODO: put out
      , Version(0), FreqTable(0), VolumeTable(0)
    {
      //assume all data is correct
      const PT3Header* const header(safe_ptr_cast<const PT3Header*>(&data[0]));
      Information.Statistic.Tempo = header->Tempo;
      Information.Statistic.Position = header->Lenght;
      Information.Loop = header->Loop;
      Information.Properties.insert(StringMap::value_type(Module::ATTR_FILENAME, filename));
      Information.Properties.insert(StringMap::value_type(Module::ATTR_TITLE, String(header->TrackName, ArrayEnd(header->TrackName))));
      Information.Properties.insert(StringMap::value_type(Module::ATTR_AUTHOR, String(header->TrackAuthor, ArrayEnd(header->TrackAuthor))));
      Information.Properties.insert(StringMap::value_type(Module::ATTR_PROGRAM, String(header->Id, 1 + ArrayEnd(header->Id))));

      //version-dependent operations
      Version = isdigit(header->Subversion) ? header->Subversion - '0' : 6;
      VolumeTable = Version <= 4 ? VolumeTable_33_34 : VolumeTable_35;
      switch (header->FreqTableNum)
      {
      case FREQ_TABLE_PT:
        FreqTable = Version <= 3 ? FreqTable_PT_33_34r : FreqTable_PT_34_35;
        break;
      case FREQ_TABLE_ST:
        FreqTable = FreqTable_ST;
        break;
      case FREQ_TABLE_ASM:
        FreqTable = Version <= 3 ? FreqTable_ASM_34r : FreqTable_ASM_34_35;
        break;
      default:
        FreqTable = Version <= 3 ? FreqTable_REAL_34r : FreqTable_REAL_34_35;
      }

      assert(VolumeTable && FreqTable);

      Log::WarningsCollector warner;

      //fill samples
      std::transform(header->SamplesOffsets, ArrayEnd(header->SamplesOffsets),
        std::back_inserter(Data.Samples), SampleCreator(data));
      //fill ornaments
      std::transform(header->OrnamentsOffsets, ArrayEnd(header->OrnamentsOffsets),
        std::back_inserter(Data.Ornaments), OrnamentCreator(data));
      //fill order
      Data.Positions.resize(header->Lenght);
      std::transform(header->Positions, header->Positions + header->Lenght,
        Data.Positions.begin(), std::bind2nd(std::divides<uint8_t>(), 3));

      //fill patterns
      Data.Patterns.resize(1 + *std::max_element(Data.Positions.begin(), Data.Positions.end()));
      const PT3Pattern* patPos(safe_ptr_cast<const PT3Pattern*>(&data[fromLE(header->PatternsOffset)]));
      std::size_t index(0);
      for (std::vector<Pattern>::iterator it = Data.Patterns.begin(), lim = Data.Patterns.end();
        it != lim;
        ++it, ++patPos, ++index)
      {
        IndexPrefix patPfx(warner, "Pattern %1%: ", index);
        Pattern& pat(*it);
        std::vector<std::size_t> offsets(ArraySize(patPos->Offsets));
        std::valarray<std::size_t> periods(std::size_t(0), ArraySize(patPos->Offsets));
        std::valarray<std::size_t> counters(std::size_t(0), ArraySize(patPos->Offsets));
        std::transform(patPos->Offsets, ArrayEnd(patPos->Offsets), offsets.begin(), &fromLE<uint16_t>);
        pat.reserve(MAX_PATTERN_SIZE);
        do
        {
          IndexPrefix notePfx(warner, "Line %1%: ", pat.size());
          pat.push_back(Line());
          Line& line(pat.back());
          ParsePattern(data, offsets, line, periods, counters, warner);
          //skip lines
          if (const std::size_t linesToSkip = counters.min())
          {
            counters -= linesToSkip;
            pat.resize(pat.size() + linesToSkip);//add dummies
          }
        }
        while (data[offsets[0]] || counters[0]);
        //as warnings
        warner.Assert(0 == counters.max(), "not all channel periods are reached");
        warner.Assert(pat.size() <= MAX_PATTERN_SIZE, "too long");
      }
      Information.Statistic.Pattern = Data.Patterns.size();
      Information.Statistic.Channels = 3;

      const String& warnings(warner.GetWarnings());
      if (!warnings.empty())
      {
        Information.Properties.insert(StringMap::value_type(Module::ATTR_WARNINGS, warnings));
      }
      InitTime();
    }

    virtual void GetInfo(Info& info) const
    {
      Describing(info);
    }

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::ChannelsState& state) const
    {
      assert(Device.get());
      Device->GetState(state);
      return PlaybackState;
    }

    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver)
    {
      AYM::DataChunk chunk;
      chunk.Tick = (CurrentState.Tick += params.ClocksPerFrame());
      RenderData(chunk);

      Device->RenderData(params, chunk, receiver);

      return Parent::RenderFrame(params, receiver);
    }

    virtual State Reset()
    {
      Device->Reset();
      return Parent::Reset();
    }

    virtual State SetPosition(const uint32_t& /*frame*/)
    {
      return PlaybackState;
    }

  private:
    void RenderData(AYM::DataChunk& chunk)
    {
      const Line& line(Data.Patterns[CurrentState.Position.Pattern][CurrentState.Position.Note]);
      if (0 == CurrentState.Position.Frame)//begin note
      {
        if (0 == CurrentState.Position.Note)//pattern begin
        {
          Commons.NoiseBase = 0;
        }
        for (std::size_t chan = 0; chan != line.Channels.size(); ++chan)
        {
          const Line::Chan& src(line.Channels[chan]);
          ChannelState& dst(Channels[chan]);
          if (src.Enabled)
          {
            dst.PosInSample = dst.PosInOrnament = 0;
            dst.VolSlide = dst.EnvSliding = dst.NoiseSliding = 0;
            dst.ToneSlider.Reset();
            dst.ToneAccumulator = 0;
            dst.VibrateCounter = 0;
            dst.Enabled = *src.Enabled;
          }
          if (src.Note)
          {
            dst.Note = *src.Note;
          }
          if (src.SampleNum)
          {
            dst.SampleNum = *src.SampleNum;
          }
          if (src.OrnamentNum)
          {
            dst.OrnamentNum = *src.OrnamentNum;
            dst.PosInOrnament = 0;
          }
          if (src.Volume)
          {
            dst.Volume = *src.Volume;
          }
          for (CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
          {
            switch (it->Type)
            {
            case GLISS:
              dst.ToneSlider.Period = dst.ToneSlider.Counter = it->Param1;
              dst.ToneSlider.Delta = it->Param2;
              dst.SlidingTargetNote = LIMITER;
              dst.VibrateCounter = 0;
              if (0 == dst.ToneSlider.Counter && Version >= 7)
              {
                ++dst.ToneSlider.Counter;
              }
              break;
            case GLISS_NOTE:
              dst.ToneSlider.Period = dst.ToneSlider.Counter = it->Param1;
              dst.ToneSlider.Delta = it->Param2;
              dst.SlidingTargetNote = it->Param3;
              dst.VibrateCounter = 0;
              //TODO: correct tone counter according to version
              break;
            case SAMPLEOFFSET:
              dst.PosInSample = it->Param1;
              break;
            case ORNAMENTOFFSET:
              dst.PosInOrnament = it->Param1;
              break;
            case VIBRATE:
              dst.VibrateOn = it->Param1;
              dst.VibrateOff = it->Param2;
              dst.VibrateCounter = 0;
              dst.ToneSlider.Value = 0;
              dst.ToneSlider.Counter = 0;
              break;
            case SLIDEENV:
              Commons.EnvSlider.Period = Commons.EnvSlider.Counter = it->Param1;
              Commons.EnvSlider.Delta = it->Param2;
              break;
            case ENVELOPE:
              chunk.Data[AYM::DataChunk::REG_ENV] = uint8_t(it->Param1);
              Commons.EnvBase = it->Param2;
              chunk.Mask |= (1 << AYM::DataChunk::REG_ENV);
              dst.Envelope = true;
              break;
            case NOENVELOPE:
              dst.Envelope = false;
              break;
            case NOISEBASE:
              Commons.NoiseBase = it->Param1;
              break;
            case TEMPO:
              //ignore
              break;
            default:
              assert(!"Invalid command");
            }
          }
        }
      }
      //permanent registers
      chunk.Data[AYM::DataChunk::REG_MIXER] = 0;
      chunk.Mask |= (1 << AYM::DataChunk::REG_MIXER) |
        (1 << AYM::DataChunk::REG_VOLA) | (1 << AYM::DataChunk::REG_VOLB) | (1 << AYM::DataChunk::REG_VOLC);
      std::size_t toneReg = AYM::DataChunk::REG_TONEA_L;
      std::size_t volReg = AYM::DataChunk::REG_VOLA;
      uint8_t toneMsk = AYM::DataChunk::MASK_TONEA;
      uint8_t noiseMsk = AYM::DataChunk::MASK_NOISEA;
      signed envelopeAddon(0);
      for (ChannelState* dst = Channels; dst != ArrayEnd(Channels);
        ++dst, toneReg += 2, ++volReg, toneMsk <<= 1, noiseMsk <<= 1)
      {
        if (dst->Enabled)
        {
          const Sample& curSample(Data.Samples[dst->SampleNum]);
          const Sample::Line& curSampleLine(curSample.Data[dst->PosInSample]);
          const Ornament& curOrnament(Data.Ornaments[dst->OrnamentNum]);

          assert(!curOrnament.Data.empty());
          //calculate tone
          const signed toneAddon(curSampleLine.ToneOffset + dst->ToneAccumulator);
          if (curSampleLine.KeepToneOffset)
          {
            dst->ToneAccumulator = toneAddon;
          }
          const std::size_t halfTone(clamp<std::size_t>(dst->Note + curOrnament.Data[dst->PosInOrnament], 0, 95));
          const uint16_t tone((FreqTable[halfTone] + dst->ToneSlider.Value + toneAddon) & 0xfff);
          if (dst->ToneSlider.Update() && LIMITER != dst->SlidingTargetNote)
          {
            const uint16_t targetTone(FreqTable[dst->SlidingTargetNote]);
            if ((dst->ToneSlider.Delta > 0 && tone + dst->ToneSlider.Delta > targetTone) ||
                (dst->ToneSlider.Delta < 0 && tone + dst->ToneSlider.Delta < targetTone))
            {
              //slided to target note
              dst->Note = dst->SlidingTargetNote;
              dst->SlidingTargetNote = LIMITER;
              dst->ToneSlider.Value = 0;
              dst->ToneSlider.Counter = 0;
            }
          }
          chunk.Data[toneReg] = uint8_t(tone & 0xff);
          chunk.Data[toneReg + 1] = uint8_t(tone >> 8);
          chunk.Mask |= 3 << toneReg;
          dst->VolSlide = clamp(dst->VolSlide + curSampleLine.VolSlideAddon, -15, 15);
          //calculate level
          chunk.Data[volReg] = GetVolume(dst->Volume, clamp<signed>(dst->VolSlide + curSampleLine.Level, 0, 15))
            | uint8_t(dst->Envelope && !curSampleLine.EnvMask ? AYM::DataChunk::MASK_ENV : 0);
          //mixer
          if (curSampleLine.ToneMask)
          {
            chunk.Data[AYM::DataChunk::REG_MIXER] |= toneMsk;
          }
          if (curSampleLine.NoiseMask)
          {
            chunk.Data[AYM::DataChunk::REG_MIXER] |= noiseMsk;
            if (curSampleLine.KeepEnvOffset)
            {
              dst->EnvSliding = curSampleLine.EnvOffset;
            }
            envelopeAddon += curSampleLine.EnvOffset;
          }
          else
          {
            Commons.NoiseAddon = curSampleLine.NoiseOffset + dst->NoiseSliding;
            if (curSampleLine.KeepNoiseOffset)
            {
              dst->NoiseSliding = Commons.NoiseAddon;
            }
          }

          if (++dst->PosInSample >= curSample.Data.size())
          {
            dst->PosInSample = curSample.Loop;
          }
          if (++dst->PosInOrnament >= curOrnament.Data.size())
          {
            dst->PosInOrnament = curOrnament.Loop;
          }
        }
        else
        {
          chunk.Data[volReg] = 0;
          //????
          chunk.Data[AYM::DataChunk::REG_MIXER] |= toneMsk | noiseMsk;
        }
        if (dst->VibrateCounter > 0 && !--dst->VibrateCounter)
        {
          dst->Enabled = !dst->Enabled;
          dst->VibrateCounter = dst->Enabled ? dst->VibrateOn : dst->VibrateOff;
        }
      }
      const signed envPeriod(envelopeAddon + Commons.EnvSlider.Value + Commons.EnvBase);
      chunk.Data[AYM::DataChunk::REG_TONEN] = uint8_t(Commons.NoiseBase + Commons.NoiseAddon) & 0x1f;
      chunk.Data[AYM::DataChunk::REG_TONEE_L] = uint8_t(envPeriod & 0xff);
      chunk.Data[AYM::DataChunk::REG_TONEE_H] = uint8_t(envPeriod >> 8);
      chunk.Mask |= (1 << AYM::DataChunk::REG_TONEN) |
        (1 << AYM::DataChunk::REG_TONEE_L) | (1 << AYM::DataChunk::REG_TONEE_H);
      Commons.EnvSlider.Update();
      CurrentState.Position.Channels = std::count_if(Channels, ArrayEnd(Channels),
        boost::mem_fn(&ChannelState::Enabled));
    }
  private:
    uint8_t GetVolume(std::size_t volume, std::size_t level)
    {
      assert(volume <= 15);
      assert(level <= 15);
      return VolumeTable[volume * 16 + level];
    }
  private:
    AYM::Chip::Ptr Device;
    ChannelState Channels[3];
    CommonState Commons;
    std::size_t Version;
    const uint16_t* FreqTable;
    const uint8_t* VolumeTable;
  };

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_AYM;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_PT3_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_PT3_VERSION));
  }

  bool Check(const uint8_t* data, std::size_t limit)
  {
    const PT3Header* const header(safe_ptr_cast<const PT3Header*>(data));
    const std::size_t patOff(fromLE(header->PatternsOffset));
    if (patOff >= limit ||
      0xff != data[patOff - 1] ||
      &data[patOff - 1] != std::find_if(header->Positions, data + patOff - 1,
      std::bind2nd(std::modulus<uint8_t>(), 3)) ||
      &header->Positions[header->Lenght] != data + patOff - 1 ||
      fromLE(header->OrnamentsOffsets[0]) + sizeof(PT3Ornament) > limit
      )
    {
      return false;
    }
    return true;
  }

  bool Checking(const String& /*filename*/, const IO::DataContainer& source)
  {
    const std::size_t limit(source.Size());
    if (limit < sizeof(PT3Header) || limit >= MAX_MODULE_SIZE)
    {
      return false;
    }

    const uint8_t* const data(static_cast<const uint8_t*>(source.Data()));
    return Check(data, limit) ||
      (Module::Detect(data, limit, PT3XPlayerDetector) && 
       Check(data + PT3X_PLAYER_SIZE, limit - PT3X_PLAYER_SIZE)) ||
      (Module::Detect(data, limit, PT35XPlayerDetector) &&
       Check(data + PT35X_PLAYER_SIZE, limit - PT35X_PLAYER_SIZE));
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data)
  {
    assert(Checking(filename, data) || !"Attempt to create pt3 player on invalid data");
    const uint8_t* const buf(static_cast<const uint8_t*>(data.Data()));
    const std::size_t size(data.Size());
    const std::size_t offset(Module::Detect(buf, size, PT3XPlayerDetector) ?
      PT3X_PLAYER_SIZE
      :
      (Module::Detect(buf, size, PT35XPlayerDetector) ? PT35X_PLAYER_SIZE : 0)
      );
    return ModulePlayer::Ptr(new PlayerImpl(filename, FastDump(data, offset)));
  }

  PluginAutoRegistrator pt3Reg(Checking, Creating, Describing);
}
