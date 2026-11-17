/**
 *
 * @file
 *
 * @brief  SQDigitalTracker tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/sqdigitaltracker.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::SQDigitalTracker
{
  using namespace Formats::Chiptune::SQDigitalTracker;

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {
  public:
    void SetVolume(uint_t volume, int_t slideDirection) override
    {
      Command("Volume"sv, "volume"sv, volume, "slideDirection"sv, slideDirection);
    }

    void SetVolumeSlidePeriod(uint_t period) override
    {
      Command("VolumeSlidePeriod"sv, period);
    }
  };

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
}  // namespace Dump::SQDigitalTracker

bool DumpSQDigitalTracker(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::SQDigitalTracker::Dumper>(data, &Formats::Chiptune::CreateSQDigitalTrackerDecoder,
                                                            &Formats::Chiptune::SQDigitalTracker::Parse);
}
