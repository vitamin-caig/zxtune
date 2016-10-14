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

//local includes
#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/builder_pattern.h"
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace FastTracker
    {
      struct Sample
      {
        struct Line
        {
          Line()
            : Level(), VolSlide()
            , Noise(), AccumulateNoise(), NoiseMask(true)
            , Tone(), AccumulateTone(), ToneMask(true)
            , EnvelopeAddon(), AccumulateEnvelope(), EnableEnvelope()
          {
          }

          uint_t Level;//0-15
          int_t VolSlide;//0/+1/-1
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

        Sample()
          : Loop()
        {
        }

        uint_t Loop;
        std::vector<Line> Lines;
      };

      struct Ornament
      {
        struct Line
        {
          Line()
            : NoteAddon(), KeepNoteAddon(), NoiseAddon(), KeepNoiseAddon()
          {
          }

          int_t NoteAddon;
          bool KeepNoteAddon;
          int_t NoiseAddon;
          bool KeepNoiseAddon;
        };

        Ornament()
          : Loop()
        {
        }

        uint_t Loop;
        std::vector<Line> Lines;
      };

      struct PositionEntry
      {
        PositionEntry() : PatternIndex(), Transposition()
        {
        }

        uint_t PatternIndex;
        int_t Transposition;
      };

      class Builder
      {
      public:
        virtual ~Builder() = default;

        virtual MetaBuilder& GetMetaBuilder() = 0;
        //common properties
        virtual void SetInitialTempo(uint_t tempo) = 0;
        //samples+ornaments
        virtual void SetSample(uint_t index, const Sample& sample) = 0;
        virtual void SetOrnament(uint_t index, const Ornament& ornament) = 0;
        //patterns
        virtual void SetPositions(const std::vector<PositionEntry>& positions, uint_t loop) = 0;

        virtual PatternBuilder& StartPattern(uint_t index) = 0;

        //! @invariant Channels are built sequentally
        virtual void StartChannel(uint_t index) = 0;
        virtual void SetRest() = 0;
        virtual void SetNote(uint_t note) = 0;
        virtual void SetSample(uint_t sample) = 0;
        virtual void SetOrnament(uint_t ornament) = 0;
        virtual void SetVolume(uint_t vol) = 0;
        //cmds
        virtual void SetEnvelope(uint_t type, uint_t tone) = 0;
        virtual void SetNoEnvelope() = 0;
        virtual void SetNoise(uint_t val) = 0;
        virtual void SetSlide(uint_t step) = 0;
        virtual void SetNoteSlide(uint_t step) = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }

    Decoder::Ptr CreateFastTrackerDecoder();
  }
}
