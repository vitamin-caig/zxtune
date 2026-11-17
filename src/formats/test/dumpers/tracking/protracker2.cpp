/**
 *
 * @file
 *
 * @brief  ProTracker2 tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/protracker2.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::ProTracker2
{
  using namespace Formats::Chiptune::ProTracker2;

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {
  public:
    void SetGlissade(int_t val) override
    {
      Command("Glissade"sv, val);
    }

    void SetNoteGliss(int_t val, uint_t limit) override
    {
      Command("NoteGliss"sv, "value"sv, val, "limit"sv, limit);
    }

    void SetNoGliss() override
    {
      Command("NoGliss"sv);
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      Command("Envelope"sv, "type"sv, type, "value"sv, value);
    }

    void SetNoEnvelope() override
    {
      Command("NoEnvelope"sv);
    }

    void SetNoiseAddon(int_t val) override
    {
      Command("NoiseAddon"sv, val);
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
}  // namespace Dump::ProTracker2

bool DumpProTracker2(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::ProTracker2::Dumper>(data, &Formats::Chiptune::CreateProTracker2Decoder,
                                                       &Formats::Chiptune::ProTracker2::Parse);
}
