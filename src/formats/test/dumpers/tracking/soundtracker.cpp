/**
 *
 * @file
 *
 * @brief  SoundTracker tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/soundtracker.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::SoundTracker
{
  using namespace Formats::Chiptune::SoundTracker;

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
}  // namespace Dump::SoundTracker

bool DumpSoundTracker1(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::SoundTracker::Dumper>(data, &Formats::Chiptune::CreateSoundTrackerDecoder,
                                                        &Formats::Chiptune::SoundTracker::Ver1::Parse);
}

bool DumpSoundTrackerCompiled(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::SoundTracker::Dumper>(data, &Formats::Chiptune::CreateSoundTrackerCompiledDecoder,
                                                        &Formats::Chiptune::SoundTracker::Ver1::ParseCompiled);
}

bool DumpSoundTracker3(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::SoundTracker::Dumper>(data, &Formats::Chiptune::CreateSoundTracker3Decoder,
                                                        &Formats::Chiptune::SoundTracker::Ver3::Parse);
}
