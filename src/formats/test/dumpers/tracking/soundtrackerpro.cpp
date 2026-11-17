/**
 *
 * @file
 *
 * @brief  SoundTrackerPro tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/soundtrackerpro.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::SoundTrackerPro
{
  using namespace Formats::Chiptune::SoundTrackerPro;

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {
  public:
    void SetEnvelope(uint_t type, uint_t value) override
    {
      Command("Envelope"sv, "type"sv, type, "value"sv, value);
    }

    void SetGliss(int_t step) override
    {
      Command("Gliss"sv, step);
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
}  // namespace Dump::SoundTrackerPro

bool DumpSoundTrackerPro(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::SoundTrackerPro::Dumper>(data,
                                                           &Formats::Chiptune::CreateSoundTrackerProCompiledDecoder,
                                                           &Formats::Chiptune::SoundTrackerPro::ParseCompiled);
}
