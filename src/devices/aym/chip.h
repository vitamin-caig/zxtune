/**
 *
 * @file
 *
 * @brief  AY/YM sound chip interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <devices/aym.h>
#include <sound/mixer.h>

// supporting for AY/YM-based modules
namespace Devices::AYM
{
  // Describes real device
  class Chip : public Device
  {
  public:
    using Ptr = std::shared_ptr<Chip>;

    virtual Sound::Chunk RenderTill(Stamp till) = 0;
  };

  enum ChannelMasks
  {
    CHANNEL_MASK_A = 1,
    CHANNEL_MASK_B = 2,
    CHANNEL_MASK_C = 4,
  };

  enum LayoutType
  {
    LAYOUT_ABC = 0,
    LAYOUT_ACB = 1,
    LAYOUT_BAC = 2,
    LAYOUT_BCA = 3,
    LAYOUT_CAB = 4,
    LAYOUT_CBA = 5,
    LAYOUT_MONO = 6,

    LAYOUT_LAST
  };

  enum InterpolationType
  {
    INTERPOLATION_NONE = 0,
    INTERPOLATION_LQ = 1,
    INTERPOLATION_HQ = 2
  };

  enum ChipType
  {
    TYPE_AY38910 = 0,
    TYPE_YM2149F = 1
  };

  class ChipParameters
  {
  public:
    typedef std::shared_ptr<const ChipParameters> Ptr;

    virtual ~ChipParameters() = default;

    virtual uint_t Version() const = 0;

    virtual uint64_t ClockFreq() const = 0;
    virtual uint_t SoundFreq() const = 0;
    virtual ChipType Type() const = 0;
    virtual InterpolationType Interpolation() const = 0;
    virtual uint_t DutyCycleValue() const = 0;
    virtual uint_t DutyCycleMask() const = 0;
    virtual LayoutType Layout() const = 0;
  };

  const uint_t SOUND_CHANNELS = 3;
  typedef Sound::ThreeChannelsMixer MixerType;

  /// Virtual constructors
  Chip::Ptr CreateChip(ChipParameters::Ptr params, MixerType::Ptr mixer);
}  // namespace Devices::AYM
