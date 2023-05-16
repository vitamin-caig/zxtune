/**
 *
 * @file
 *
 * @brief  ProSoundMaker support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/builder_pattern.h"
#include "formats/chiptune/objects.h"
// library includes
#include <formats/chiptune.h>

namespace Formats::Chiptune
{
  namespace ProSoundMaker
  {
    struct SampleLine
    {
      SampleLine()
        : Level()
        , Noise()
        , ToneMask(true)
        , NoiseMask(true)
        , Gliss()
      {}

      uint_t Level;  // 0-15
      uint_t Noise;  // 0-31
      bool ToneMask;
      bool NoiseMask;
      int_t Gliss;
    };

    class Sample : public LinesObject<SampleLine>
    {
    public:
      Sample()
        : LinesObject<SampleLine>()
        , VolumeDeltaPeriod()
        , VolumeDeltaValue()
      {}

      Sample(const Sample&) = delete;
      Sample& operator=(const Sample&) = delete;

      Sample(Sample&& rh) noexcept  // = default
        : LinesObject<SampleLine>(std::move(rh))
        , VolumeDeltaPeriod(rh.VolumeDeltaPeriod)
        , VolumeDeltaValue(rh.VolumeDeltaValue)
      {}

      Sample& operator=(Sample&& rh) noexcept  // = default
      {
        VolumeDeltaPeriod = rh.VolumeDeltaPeriod;
        VolumeDeltaValue = rh.VolumeDeltaValue;
        LinesObject<SampleLine>::operator=(std::move(rh));
        return *this;
      }

      uint_t VolumeDeltaPeriod;
      uint_t VolumeDeltaValue;
    };

    typedef LinesObject<int_t> Ornament;

    struct PositionEntry
    {
      PositionEntry()
        : PatternIndex()
        , Transposition()
      {}

      uint_t PatternIndex;
      int_t Transposition;
    };

    typedef LinesObject<PositionEntry> Positions;

    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;
      // samples+ornaments
      virtual void SetSample(uint_t index, Sample sample) = 0;
      virtual void SetOrnament(uint_t index, Ornament ornament) = 0;
      // patterns
      virtual void SetPositions(Positions positions) = 0;

      virtual PatternBuilder& StartPattern(uint_t index) = 0;

      //! @invariant Channels are built sequentally
      virtual void StartChannel(uint_t index) = 0;
      virtual void SetRest() = 0;
      virtual void SetNote(uint_t note) = 0;
      virtual void SetSample(uint_t sample) = 0;
      virtual void SetOrnament(uint_t ornament) = 0;
      virtual void SetVolume(uint_t volume) = 0;
      virtual void DisableOrnament() = 0;
      virtual void SetEnvelopeReinit(bool enabled) = 0;
      virtual void SetEnvelopeTone(uint_t type, uint_t tone) = 0;
      virtual void SetEnvelopeNote(uint_t type, uint_t note) = 0;
    };

    Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace ProSoundMaker

  Decoder::Ptr CreateProSoundMakerCompiledDecoder();
}  // namespace Formats::Chiptune
