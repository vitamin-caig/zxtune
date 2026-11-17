/**
 *
 * @file
 *
 * @brief  ProDigiTracker tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/prodigitracker.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::ProDigiTracker
{
  using namespace Formats::Chiptune::ProDigiTracker;

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {};

  class Dumper : public Builder
  {
  public:
    explicit Dumper(StringView decoder)
    {
      Dump::Header(decoder, 4);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Dump::Number(0, "initial_tempo"sv, tempo);
    }

    void SetSample(uint_t /*index*/, std::size_t /*loop*/, Binary::View /*sample*/) override {}

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
}  // namespace Dump::ProDigiTracker

bool DumpProDigiTracker(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::ProDigiTracker::Dumper>(data, &Formats::Chiptune::CreateProDigiTrackerDecoder,
                                                          &Formats::Chiptune::ProDigiTracker::Parse);
}
