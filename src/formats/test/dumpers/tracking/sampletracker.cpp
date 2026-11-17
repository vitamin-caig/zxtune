/**
 *
 * @file
 *
 * @brief  SampleTracker tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/sampletracker.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::SampleTracker
{
  using namespace Formats::Chiptune::Digital;
  using namespace Formats::Chiptune::SampleTracker;

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {};

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

    void SetSamplesFrequency(uint_t freq) override
    {
      Dump::Number(0, "samples_frequency"sv, freq);
    }

    void SetSample(uint_t /*index*/, std::size_t /*loop*/, Binary::View /*sample*/, bool /*is4Bit*/) override {}

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
}  // namespace Dump::SampleTracker

bool DumpSampleTracker(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::SampleTracker::Dumper>(data, &Formats::Chiptune::CreateSampleTrackerDecoder,
                                                         &Formats::Chiptune::SampleTracker::Parse);
}
