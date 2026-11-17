/**
 *
 * @file
 *
 * @brief  ProTracker1 tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/protracker1.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::ProTracker1
{
  using namespace Formats::Chiptune::ProTracker1;

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {
  public:
    void SetEnvelope(uint_t type, uint_t value) override
    {
      Command("Envelope"sv, "type"sv, type, "value"sv, value);
    }

    void SetNoEnvelope() override
    {
      Command("NoEnvelope"sv);
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
}  // namespace Dump::ProTracker1

bool DumpProTracker1(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::ProTracker1::Dumper>(data, &Formats::Chiptune::CreateProTracker1Decoder,
                                                       &Formats::Chiptune::ProTracker1::Parse);
}
