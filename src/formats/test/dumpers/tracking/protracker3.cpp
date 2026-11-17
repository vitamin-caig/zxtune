/**
 *
 * @file
 *
 * @brief  ProTracker3/VortexTracker2 tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/protracker3.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::ProTracker3
{
  using namespace Formats::Chiptune::ProTracker3;

  StringView NoteTableName(NoteTable table)
  {
    switch (table)
    {
    case PROTRACKER:
      return "PROTRACKER"sv;
    case SOUNDTRACKER:
      return "SOUNDTRACKER"sv;
    case ASM:
      return "ASM"sv;
    case REAL:
      return "REAL"sv;
    case NATURAL:
      return "NATURAL"sv;
    default:
      return "UNKNOWN"sv;
    }
  }

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {
  public:
    void SetGlissade(uint_t period, int_t val) override
    {
      Command("Glissade"sv, "period"sv, period, "value"sv, val);
    }

    void SetNoteGliss(uint_t period, int_t val, uint_t limit) override
    {
      Command("NoteGliss"sv, "period"sv, period, "value"sv, val, "limit"sv, limit);
    }

    void SetSampleOffset(uint_t offset) override
    {
      Command("SampleOffset"sv, offset);
    }

    void SetOrnamentOffset(uint_t offset) override
    {
      Command("OrnamentOffset"sv, offset);
    }

    void SetVibrate(uint_t ontime, uint_t offtime) override
    {
      Command("Vibrate"sv, "ontime"sv, ontime, "offtime"sv, offtime);
    }

    void SetEnvelopeSlide(uint_t period, int_t val) override
    {
      Command("EnvelopeSlide"sv, "period"sv, period, "value"sv, val);
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      Command("Envelope"sv, "type"sv, type, "value"sv, value);
    }

    void SetNoEnvelope() override
    {
      Command("NoEnvelope"sv);
    }

    void SetNoiseBase(uint_t val) override
    {
      Command("NoiseBase"sv, val);
    }
  };

  class Dumper : public Builder
  {
  public:
    explicit Dumper(StringView decoder)
    {
      Dump::Header(decoder, 3);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetVersion(uint_t version) override
    {
      Dump::Number(0, "version"sv, version);
    }

    void SetNoteTable(NoteTable table) override
    {
      Dump::Scalar(0, "note_table"sv, NoteTableName(table));
    }

    void SetMode(uint_t mode) override
    {
      Dump::Number(0, "mode"sv, mode);
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Dump::Number(0, "initial_tempo"sv, tempo);
    }

    void SetSample(uint_t /*index*/, Sample /*sample*/) override {}

    void SetOrnament(uint_t /*index*/, Ornament /*ornament*/) override {}

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
}  // namespace Dump::ProTracker3

bool DumpProTracker3(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::ProTracker3::Dumper>(data, &Formats::Chiptune::CreateProTracker3Decoder,
                                                       &Formats::Chiptune::ProTracker3::Parse);
}

bool DumpVortexTracker2(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::ProTracker3::Dumper>(data, &Formats::Chiptune::CreateVortexTracker2Decoder,
                                                       &Formats::Chiptune::ProTracker3::VortexTracker2::Parse);
}
