/**
 *
 * @file
 *
 * @brief  ProSoundCreator support interface
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
  namespace ProSoundCreator
  {
    struct SampleLine
    {
      SampleLine() = default;

      uint_t Level = 0;  // 0-15
      uint_t Tone = 0;
      bool ToneMask = true;
      bool NoiseMask = true;
      int_t Adding = 0;
      bool EnableEnvelope = false;
      int_t VolumeDelta = 0;  // 0/+1/-1
      bool LoopBegin = false;
      bool LoopEnd = false;
    };

    typedef LinesObject<SampleLine> Sample;

    struct OrnamentLine
    {
      OrnamentLine() = default;
      int_t NoteAddon = 0;
      int_t NoiseAddon = 0;
      bool LoopBegin = false;
      bool LoopEnd = false;
    };

    typedef LinesObject<OrnamentLine> Ornament;

    typedef LinesObject<uint_t> Positions;

    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;
      // common properties
      virtual void SetInitialTempo(uint_t tempo) = 0;
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
      virtual void SetVolume(uint_t vol) = 0;
      // cmds
      virtual void SetEnvelope(uint_t type, uint_t tone) = 0;
      virtual void SetEnvelope() = 0;
      virtual void SetNoEnvelope() = 0;
      virtual void SetNoiseBase(uint_t val) = 0;
      virtual void SetBreakSample() = 0;
      virtual void SetBreakOrnament() = 0;
      virtual void SetNoOrnament() = 0;
      virtual void SetGliss(uint_t absStep) = 0;
      virtual void SetSlide(int_t delta) = 0;
      virtual void SetVolumeSlide(uint_t period, int_t delta) = 0;
    };

    Builder& GetStubBuilder();
    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
  }  // namespace ProSoundCreator

  Decoder::Ptr CreateProSoundCreatorDecoder();
}  // namespace Formats::Chiptune
