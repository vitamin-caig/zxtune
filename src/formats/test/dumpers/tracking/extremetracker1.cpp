/**
 *
 * @file
 *
 * @brief  ExtremeTracker1 tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/extremetracker1.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::ExtremeTracker1
{
  using namespace Formats::Chiptune::ExtremeTracker1;

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {
  public:
    void SetGliss(int_t gliss) override
    {
      Command("Gliss"sv, gliss);
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

    void SetSamplesFrequency(uint_t freq) override
    {
      Dump::Number(0, "samples_frequency"sv, freq);
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
}  // namespace Dump::ExtremeTracker1

bool DumpExtremeTracker1(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::ExtremeTracker1::Dumper>(data, &Formats::Chiptune::CreateExtremeTracker1Decoder,
                                                           &Formats::Chiptune::ExtremeTracker1::Parse);
}
