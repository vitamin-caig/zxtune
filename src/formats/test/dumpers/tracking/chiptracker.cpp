/**
 *
 * @file
 *
 * @brief  ChipTracker tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/chiptracker.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::ChipTracker
{
  using namespace Formats::Chiptune::ChipTracker;

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {
  public:
    void SetSlide(int_t step) override
    {
      Command("Slide"sv, step);
    }

    void SetSampleOffset(uint_t offset) override
    {
      Command("SampleOffset"sv, offset);
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
}  // namespace Dump::ChipTracker

bool DumpChipTracker(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::ChipTracker::Dumper>(data, &Formats::Chiptune::CreateChipTrackerDecoder,
                                                       &Formats::Chiptune::ChipTracker::Parse);
}
