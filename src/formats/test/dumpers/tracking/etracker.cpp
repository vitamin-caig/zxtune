/**
 *
 * @file
 *
 * @brief  ETracker tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/saa/etracker.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::ETracker
{
  using namespace Formats::Chiptune::ETracker;

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {
  public:
    void SetAttenuation(uint_t vol) override
    {
      Command("Attenuation"sv, vol);
    }

    void SetSwapSampleChannels(bool swapChannels) override
    {
      Command("SwapSampleChannels"sv, swapChannels);
    }

    void SetEnvelope(uint_t value) override
    {
      Command("Envelope"sv, value);
    }

    void SetNoise(uint_t type) override
    {
      Command("Noise"sv, type);
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

    void SetInitialTempo(uint_t tempo) override
    {
      Dump::Number(0, "initial_tempo"sv, tempo);
    }

    void SetSample(uint_t /*index*/, Sample /*sample*/) override {}

    void SetOrnament(uint_t /*index*/, Ornament /*ornament*/) override {}

    void SetPositions(Positions positions) override
    {
      Dump::DumpPositions(positions, [](const PositionEntry& entry) { Dump::DumpPositionEntry(entry); });
    }

    PatternBuilder& StartPattern(uint_t index) override
    {
      return Patterns.Start(index);
    }

  private:
    Dump::MetaDumper Meta;
    Dump::PatternDumper<ChannelBuilder, ChannelDumper> Patterns;
  };
}  // namespace Dump::ETracker

bool DumpETracker(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::ETracker::Dumper>(data, &Formats::Chiptune::CreateETrackerDecoder,
                                                    &Formats::Chiptune::ETracker::Parse);
}
