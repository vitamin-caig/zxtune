/**
 *
 * @file
 *
 * @brief  TFMMusicMaker support interface
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
// std includes
#include <array>

namespace Formats::Chiptune
{
  namespace TFMMusicMaker
  {
    struct Instrument
    {
      struct Operator
      {
        Operator() = default;

        uint_t Multiple = 0;
        int_t Detune = 0;
        uint_t TotalLevel = 0;
        uint_t RateScaling = 0;
        uint_t Attack = 0;
        uint_t Decay = 0;
        uint_t Sustain = 0;
        uint_t Release = 0;
        uint_t SustainLevel = 0;
        uint_t EnvelopeType = 0;
      };

      Instrument() = default;

      uint_t Algorithm = 0;
      uint_t Feedback = 0;
      std::array<Operator, 4> Operators;
    };

    struct Date
    {
      Date() = default;

      uint_t Year = 0;
      uint_t Month = 0;
      uint_t Day = 0;
    };

    typedef LinesObject<uint_t> Positions;

    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;
      virtual void SetTempo(uint_t evenTempo, uint_t oddTempo, uint_t interleavePeriod) = 0;
      virtual void SetDate(const Date& created, const Date& saved) = 0;

      virtual void SetInstrument(uint_t index, Instrument instrument) = 0;
      // patterns
      virtual void SetPositions(Positions positions) = 0;

      virtual PatternBuilder& StartPattern(uint_t index) = 0;

      //! @invariant Channels are built sequentally
      virtual void StartChannel(uint_t index) = 0;
      virtual void SetKeyOff() = 0;
      virtual void SetNote(uint_t note) = 0;
      virtual void SetVolume(uint_t vol) = 0;
      virtual void SetInstrument(uint_t ins) = 0;
      virtual void SetArpeggio(uint_t add1, uint_t add2) = 0;
      virtual void SetSlide(int_t step) = 0;
      virtual void SetPortamento(int_t step) = 0;
      virtual void SetVibrato(uint_t speed, uint_t depth) = 0;
      virtual void SetTotalLevel(uint_t op, uint_t value) = 0;
      virtual void SetVolumeSlide(uint_t up, uint_t down) = 0;
      virtual void SetSpecialMode(bool on) = 0;
      virtual void SetToneOffset(uint_t op, uint_t offset) = 0;
      virtual void SetMultiple(uint_t op, uint_t val) = 0;
      virtual void SetOperatorsMixing(uint_t mask) = 0;
      virtual void SetLoopStart() = 0;
      virtual void SetLoopEnd(uint_t additionalCount) = 0;
      virtual void SetPane(uint_t pane) = 0;
      virtual void SetNoteRetrig(uint_t period) = 0;
      virtual void SetNoteCut(uint_t quirk) = 0;
      virtual void SetNoteDelay(uint_t quirk) = 0;
      virtual void SetDropEffects() = 0;
      virtual void SetFeedback(uint_t val) = 0;
      virtual void SetTempoInterleave(uint_t val) = 0;
      virtual void SetTempoValues(uint_t even, uint_t odd) = 0;
    };

    Builder& GetStubBuilder();

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      typedef std::shared_ptr<const Decoder> Ptr;

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const = 0;
    };

    namespace Ver05
    {
      Decoder::Ptr CreateDecoder();
    }

    namespace Ver13
    {
      Decoder::Ptr CreateDecoder();
    }
  }  // namespace TFMMusicMaker

  Decoder::Ptr CreateTFMMusicMaker05Decoder();
  Decoder::Ptr CreateTFMMusicMaker13Decoder();
}  // namespace Formats::Chiptune
