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

//local includes
#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/builder_pattern.h"
//library includes
#include <formats/chiptune.h>
//boost includes
#include <boost/optional.hpp>

namespace Formats
{
  namespace Chiptune
  {
    namespace ProSoundMaker
    {
      struct Sample
      {
        struct Line
        {
          Line() : Level(), Noise(), ToneMask(true), NoiseMask(true), Gliss()
          {
          }

          uint_t Level;//0-15
          uint_t Noise;//0-31
          bool ToneMask;
          bool NoiseMask;
          int_t Gliss;
        };

        Sample() : VolumeDeltaPeriod(), VolumeDeltaValue()
        {
        }

        Sample(const Sample&) = delete;
        Sample& operator = (const Sample&) = delete;
        Sample(Sample&&) = default;
        Sample& operator = (Sample&&) = default;

        boost::optional<uint_t> Loop;
        uint_t VolumeDeltaPeriod;
        int_t VolumeDeltaValue;
        std::vector<Line> Lines;
      };

      struct Ornament
      {
        Ornament()
        {
        }

        Ornament(const Ornament&) = delete;
        Ornament& operator = (const Ornament&) = delete;
        Ornament(Ornament&&) = default;
        Ornament& operator = (Ornament&&) = default;

        boost::optional<uint_t> Loop;
        std::vector<int_t> Lines;
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
        //samples+ornaments
        virtual void SetSample(uint_t index, Sample sample) = 0;
        virtual void SetOrnament(uint_t index, Ornament ornament) = 0;
        //patterns
        virtual void SetPositions(std::vector<PositionEntry> positions, uint_t loop) = 0;

        virtual PatternBuilder& StartPattern(uint_t index) = 0;

        //! @invariant Channels are built sequentally
        virtual void StartChannel(uint_t index) = 0;
        virtual void SetRest() = 0;
        virtual void SetNote(uint_t note) = 0;
        virtual void SetSample(uint_t sample) = 0;
        virtual void SetOrnament(uint_t ornament) = 0;
        virtual void SetVolume(uint_t volume) = 0;
        virtual void DisableOrnament() = 0;
        virtual void SetEnvelopeType(uint_t type) = 0;
        virtual void SetEnvelopeTone(uint_t tone) = 0;
        virtual void SetEnvelopeNote(uint_t note) = 0;
      };

      Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }

    Decoder::Ptr CreateProSoundMakerCompiledDecoder();
  }
}
