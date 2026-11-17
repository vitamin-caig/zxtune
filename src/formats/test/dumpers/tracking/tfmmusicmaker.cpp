/**
 *
 * @file
 *
 * @brief  TFM Music Maker tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/fm/tfmmusicmaker.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::TFMMusicMaker
{
  using namespace Formats::Chiptune::TFMMusicMaker;

  void WriteDate(StringView key, const Date& date)
  {
    Dump::Indent(0);
    StdOut << key << ": {year: " << date.Year << ", month: " << date.Month << ", day: " << date.Day << "}\n";
  }

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {
  public:
    void SetKeyOff() override
    {
      Command("KeyOff"sv);
    }

    void SetInstrument(uint_t ins) override
    {
      Command("Instrument"sv, ins);
    }

    void SetArpeggio(uint_t add1, uint_t add2) override
    {
      Command("Arpeggio"sv, "add1"sv, add1, "add2"sv, add2);
    }

    void SetSlide(int_t step) override
    {
      Command("Slide"sv, step);
    }

    void SetPortamento(int_t step) override
    {
      Command("Portamento"sv, step);
    }

    void SetVibrato(uint_t speed, uint_t depth) override
    {
      Command("Vibrato"sv, "speed"sv, speed, "depth"sv, depth);
    }

    void SetTotalLevel(uint_t op, uint_t value) override
    {
      Command("TotalLevel"sv, "op"sv, op, "value"sv, value);
    }

    void SetVolumeSlide(uint_t up, uint_t down) override
    {
      Command("VolumeSlide"sv, "up"sv, up, "down"sv, down);
    }

    void SetSpecialMode(bool on) override
    {
      Command("SpecialMode"sv, on);
    }

    void SetToneOffset(uint_t op, uint_t offset) override
    {
      Command("ToneOffset"sv, "op"sv, op, "offset"sv, offset);
    }

    void SetMultiple(uint_t op, uint_t val) override
    {
      Command("Multiple"sv, "op"sv, op, "value"sv, val);
    }

    void SetOperatorsMixing(uint_t mask) override
    {
      Command("OperatorsMixing"sv, mask);
    }

    void SetLoopStart() override
    {
      Command("LoopStart"sv);
    }

    void SetLoopEnd(uint_t additionalCount) override
    {
      Command("LoopEnd"sv, additionalCount);
    }

    void SetPane(uint_t pane) override
    {
      Command("Pane"sv, pane);
    }

    void SetNoteRetrig(uint_t period) override
    {
      Command("NoteRetrig"sv, period);
    }

    void SetNoteCut(uint_t quirk) override
    {
      Command("NoteCut"sv, quirk);
    }

    void SetNoteDelay(uint_t quirk) override
    {
      Command("NoteDelay"sv, quirk);
    }

    void SetDropEffects() override
    {
      Command("DropEffects"sv);
    }

    void SetFeedback(uint_t val) override
    {
      Command("Feedback"sv, val);
    }

    void SetTempoInterleave(uint_t val) override
    {
      Command("TempoInterleave"sv, val);
    }

    void SetTempoValues(uint_t even, uint_t odd) override
    {
      Command("TempoValues"sv, "even"sv, even, "odd"sv, odd);
    }
  };

  class Dumper : public Builder
  {
  public:
    explicit Dumper(StringView decoder)
    {
      Dump::Header(decoder, 6);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetTempo(uint_t evenTempo, uint_t oddTempo, uint_t interleavePeriod) override
    {
      Dump::Map(0, "tempo"sv);
      Dump::Number(1, "even"sv, evenTempo);
      Dump::Number(1, "odd"sv, oddTempo);
      Dump::Number(1, "interleave"sv, interleavePeriod);
    }

    void SetDate(const Date& created, const Date& saved) override
    {
      WriteDate("created"sv, created);
      WriteDate("saved"sv, saved);
    }

    void SetInstrument(uint_t /*index*/, Instrument /*instrument*/) override {}

    void SetPositions(Positions positions) override
    {
      Dump::DumpPositions(positions);
    }

    PatternBuilder& StartPattern(uint_t index) override
    {
      return Patterns.Start(index);
    }

  private:
    Dump::MetaDumper Meta;
    Dump::PatternDumper<ChannelBuilder, ChannelDumper> Patterns;
  };
}  // namespace Dump::TFMMusicMaker

bool DumpTFMMusicMaker05(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::TFMMusicMaker::Dumper>(data, &Formats::Chiptune::CreateTFMMusicMaker05Decoder,
                                                         &Formats::Chiptune::TFMMusicMaker::Ver05::Parse);
}

bool DumpTFMMusicMaker13(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::TFMMusicMaker::Dumper>(data, &Formats::Chiptune::CreateTFMMusicMaker13Decoder,
                                                         &Formats::Chiptune::TFMMusicMaker::Ver13::Parse);
}
