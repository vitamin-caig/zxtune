/**
 *
 * @file
 *
 * @brief  FastTracker support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/objects.h"

#include "formats/chiptune.h"

#include "types.h"

namespace Formats::Chiptune
{
  class MetaBuilder;
  class PatternBuilder;

  namespace FastTracker
  {
    struct SampleLine
    {
      SampleLine() = default;

      uint_t Level = 0;    // 0-15
      int_t VolSlide = 0;  // 0/+1/-1
      uint_t Noise = 0;
      bool AccumulateNoise = false;
      bool NoiseMask = true;
      uint_t Tone = 0;
      bool AccumulateTone = false;
      bool ToneMask = true;
      int_t EnvelopeAddon = 0;
      bool AccumulateEnvelope = false;
      bool EnableEnvelope = false;
    };

    using Sample = LinesObject<SampleLine>;

    struct OrnamentLine
    {
      OrnamentLine() = default;

      int_t NoteAddon = 0;
      bool KeepNoteAddon = false;
      int_t NoiseAddon = 0;
      bool KeepNoiseAddon = false;
    };

    using Ornament = LinesObject<OrnamentLine>;

    struct PositionEntry
    {
      PositionEntry() = default;

      uint_t PatternIndex = 0;
      int_t Transposition = 0;
    };

    using Positions = LinesObject<PositionEntry>;

    enum class NoteTable
    {
      PROTRACKER2,
      SOUNDTRACKER,
      FASTTRACKER
    };

    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;
      // common properties
      virtual void SetNoteTable(NoteTable table) = 0;
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
      virtual void SetNoEnvelope() = 0;
      virtual void SetNoise(uint_t val) = 0;
      virtual void SetSlide(uint_t step) = 0;
      virtual void SetNoteSlide(uint_t step) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace FastTracker

  Decoder::Ptr CreateFastTrackerDecoder();
}  // namespace Formats::Chiptune
