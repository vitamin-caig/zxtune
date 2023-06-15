/**
 *
 * @file
 *
 * @brief  Volume lookup table
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "devices/aym/src/generators.h"
// library includes
#include <devices/aym/chip.h>
// std includes
#include <cassert>

namespace Devices::AYM
{
  class MultiVolumeTable
  {
  public:
    MultiVolumeTable() = default;

    void SetParameters(ChipType type, LayoutType layout, const MixerType& mixer)
    {
      static const MultiSample IN_A = {{Sound::Sample::MAX, Sound::Sample::MID, Sound::Sample::MID}};
      static const MultiSample IN_B = {{Sound::Sample::MID, Sound::Sample::MAX, Sound::Sample::MID}};
      static const MultiSample IN_C = {{Sound::Sample::MID, Sound::Sample::MID, Sound::Sample::MAX}};

      const Sound::Sample::Type* const newTable = GetVolumeTable(type);
      const LayoutData* const newLayout = GetLayout(layout);
      if (Table != newTable
          || Layout != newLayout
          // simple mixer fingerprint
          || !(Lookup[HIGH_LEVEL_A] == Mix(IN_A, mixer)) || !(Lookup[HIGH_LEVEL_B] == Mix(IN_B, mixer))
          || !(Lookup[HIGH_LEVEL_C] == Mix(IN_C, mixer)))
      {
        Table = newTable;
        Layout = newLayout;
        FillLookupTable(mixer);
      }
    }

    Sound::Sample Get(uint_t in) const
    {
      return Lookup[in];
    }

  private:
    static const Sound::Sample::Type* GetVolumeTable(ChipType type)
    {
      static const Sound::Sample::Type AYVolumeTab[32] = {
          ToSample(0x0000), ToSample(0x0000), ToSample(0x0340), ToSample(0x0340), ToSample(0x04C0), ToSample(0x04C0),
          ToSample(0x06F2), ToSample(0x06F2), ToSample(0x0A44), ToSample(0x0A44), ToSample(0x0F13), ToSample(0x0F13),
          ToSample(0x1510), ToSample(0x1510), ToSample(0x227E), ToSample(0x227E), ToSample(0x289F), ToSample(0x289F),
          ToSample(0x414E), ToSample(0x414E), ToSample(0x5B21), ToSample(0x5B21), ToSample(0x7258), ToSample(0x7258),
          ToSample(0x905E), ToSample(0x905E), ToSample(0xB550), ToSample(0xB550), ToSample(0xD7A0), ToSample(0xD7A0),
          ToSample(0xFFFF), ToSample(0xFFFF)};
      static const Sound::Sample::Type YMVolumeTab[32] = {
          ToSample(0x0000), ToSample(0x0000), ToSample(0x00EF), ToSample(0x01D0), ToSample(0x0290), ToSample(0x032A),
          ToSample(0x03EE), ToSample(0x04D2), ToSample(0x0611), ToSample(0x0782), ToSample(0x0912), ToSample(0x0A36),
          ToSample(0x0C31), ToSample(0x0EB6), ToSample(0x1130), ToSample(0x13A0), ToSample(0x1751), ToSample(0x1BF5),
          ToSample(0x20E2), ToSample(0x2594), ToSample(0x2CA1), ToSample(0x357F), ToSample(0x3E45), ToSample(0x475E),
          ToSample(0x5502), ToSample(0x6620), ToSample(0x7730), ToSample(0x8844), ToSample(0xA1D2), ToSample(0xC102),
          ToSample(0xE0A2), ToSample(0xFFFF)};
      switch (type)
      {
      case TYPE_YM2149F:
        return YMVolumeTab;
      default:
        return AYVolumeTab;
      }
    }

    static Sound::Sample::Type ToSample(uint_t val)
    {
      return Sound::Sample::MID
             + val * (Sound::Sample::MAX - Sound::Sample::MID) / (Sound::Sample::MAX - Sound::Sample::MIN);
    }

    using LayoutData = std::array<uint_t, SOUND_CHANNELS>;

    static const LayoutData* GetLayout(LayoutType type)
    {
      static const LayoutData LAYOUTS[] = {
          {{0, 1, 2}},  // ABC
          {{0, 2, 1}},  // ACB
          {{1, 0, 2}},  // BAC
          {{1, 2, 0}},  // BCA
          {{2, 1, 0}},  // CBA
          {{2, 0, 1}},  // CAB
      };
      return type == LAYOUT_MONO ? nullptr : LAYOUTS + type;
    }

    using MultiSample = MixerType::InDataType;

    void FillLookupTable(const MixerType& mixer)
    {
      for (uint_t idx = 0; idx != Lookup.size(); ++idx)
      {
        const MultiSample res = {{Table[idx & HIGH_LEVEL_A], Table[(idx >> BITS_PER_LEVEL) & HIGH_LEVEL_A],
                                  Table[idx >> 2 * BITS_PER_LEVEL]}};
        Lookup[idx] = Mix(res, mixer);
      }
    }

    Sound::Sample Mix(const MultiSample& in, const MixerType& mixer) const
    {
      if (Layout)
      {
        const MultiSample out = {{in[Layout->at(0)], in[Layout->at(1)], in[Layout->at(2)]}};
        return mixer.ApplyData(out);
      }
      else  // mono
      {
        const Sound::Sample::Type avg = (int_t(in[0]) + in[1] + in[2]) / SOUND_CHANNELS;
        const MultiSample out = {{avg, avg, avg}};
        return mixer.ApplyData(out);
      }
    }

  private:
    const Sound::Sample::Type* Table = nullptr;
    const LayoutData* Layout = nullptr;
    std::array<Sound::Sample, 1 << SOUND_CHANNELS * BITS_PER_LEVEL> Lookup;
  };
}  // namespace Devices::AYM
