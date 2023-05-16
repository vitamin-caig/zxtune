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

// local includes
#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/builder_pattern.h"
#include "formats/chiptune/objects.h"
// library includes
#include <formats/chiptune.h>

namespace Formats::Chiptune
{
  namespace FastTracker
  {
    struct SampleLine
    {
      SampleLine()
        : Level()
        , VolSlide()
        , Noise()
        , AccumulateNoise()
        , NoiseMask(true)
        , Tone()
        , AccumulateTone()
        , ToneMask(true)
        , EnvelopeAddon()
        , AccumulateEnvelope()
        , EnableEnvelope()
      {}

      uint_t Level;    // 0-15
      int_t VolSlide;  // 0/+1/-1
      uint_t Noise;
      bool AccumulateNoise;
      bool NoiseMask;
      uint_t Tone;
      bool AccumulateTone;
      bool ToneMask;
      int_t EnvelopeAddon;
      bool AccumulateEnvelope;
      bool EnableEnvelope;
    };

    typedef LinesObject<SampleLine> Sample;

    struct OrnamentLine
    {
      OrnamentLine()
        : NoteAddon()
        , KeepNoteAddon()
        , NoiseAddon()
        , KeepNoiseAddon()
      {}

      int_t NoteAddon;
      bool KeepNoteAddon;
      int_t NoiseAddon;
      bool KeepNoiseAddon;
    };

    typedef LinesObject<OrnamentLine> Ornament;

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
