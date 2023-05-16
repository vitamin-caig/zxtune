/**
 *
 * @file
 *
 * @brief  SoundTracker support interface
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
  namespace SoundTracker
  {
    struct SampleLine
    {
      SampleLine() {}

      uint_t Level = 0;  // 0-15
      uint_t Noise = 0;  // 0-31
      bool NoiseMask = true;
      bool EnvelopeMask = true;
      int_t Effect = 0;
    };

    typedef LinesObjectWithLoopLimit<SampleLine> Sample;
    typedef LinesObject<int_t> Ornament;

    struct PositionEntry
    {
      PositionEntry() {}

      uint_t PatternIndex = 0;
      int_t Transposition = 0;
    };

    typedef LinesObject<PositionEntry> Positions;

    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;
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
      virtual void SetEnvelope(uint_t type, uint_t value) = 0;
      virtual void SetNoEnvelope() = 0;
    };

    Builder& GetStubBuilder();

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      typedef std::shared_ptr<const Decoder> Ptr;

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const = 0;
    };

    namespace Ver1
    {
      Decoder::Ptr CreateUncompiledDecoder();
      Decoder::Ptr CreateCompiledDecoder();
    }  // namespace Ver1

    namespace Ver3
    {
      Decoder::Ptr CreateDecoder();

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Binary::Container::Ptr InsertMetainformation(const Binary::Container& rawData, Binary::View info);
    }  // namespace Ver3
  }    // namespace SoundTracker

  Decoder::Ptr CreateSoundTrackerDecoder();
  Decoder::Ptr CreateSoundTrackerCompiledDecoder();
  Decoder::Ptr CreateSoundTracker3Decoder();
}  // namespace Formats::Chiptune
