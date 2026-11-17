/**
 *
 * @file
 *
 * @brief  SQTracker tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/sqtracker.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::SQTracker
{
  using namespace Formats::Chiptune::SQTracker;

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {
  public:
    void SetTempoAddon(uint_t add) override
    {
      Command("TempoAddon"sv, add);
    }

    void SetEnvelope(uint_t type, uint_t value) override
    {
      Command("Envelope"sv, "type"sv, type, "value"sv, value);
    }

    void SetGlissade(int_t step) override
    {
      Command("Glissade"sv, step);
    }

    void SetAttenuation(uint_t att) override
    {
      Command("Attenuation"sv, att);
    }

    void SetAttenuationAddon(int_t add) override
    {
      Command("AttenuationAddon"sv, add);
    }

    void SetGlobalAttenuation(uint_t att) override
    {
      Command("GlobalAttenuation"sv, att);
    }

    void SetGlobalAttenuationAddon(int_t add) override
    {
      Command("GlobalAttenuationAddon"sv, add);
    }
  };

  class Dumper : public Builder
  {
  public:
    explicit Dumper(StringView decoder)
    {
      Dump::Header(decoder, 1);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetSample(uint_t /*index*/, Sample /*sample*/) override {}

    void SetOrnament(uint_t /*index*/, Ornament /*ornament*/) override {}

    void SetPositions(Positions positions) override
    {
      Dump::Map(0, "order"sv);
      Dump::Indent(1);
      StdOut << "positions: [";
      for (uint_t idx = 0, lim = positions.GetSize(); idx != lim; ++idx)
      {
        if (idx)
        {
          StdOut << ", ";
        }
        const auto& entry = positions.GetLine(idx);
        StdOut << "{tempo: " << entry.Tempo << ", channels: [";
        for (uint_t chan = 0; chan != entry.Channels.size(); ++chan)
        {
          if (chan)
          {
            StdOut << ", ";
          }
          const auto& channel = entry.Channels[chan];
          StdOut << "{pattern: " << channel.Pattern << ", transposition: " << channel.Transposition
                 << ", attenuation: " << channel.Attenuation
                 << ", effects: " << (channel.EnabledEffects ? "true" : "false") << '}';
        }
        StdOut << "]}";
      }
      StdOut << "]\n";
      Dump::Number(1, "loop"sv, positions.GetLoop());
    }

    PatternBuilder& StartPattern(uint_t index) override
    {
      return Patterns.Start(index);
    }

  private:
    Dump::MetaDumper Meta;
    Dump::PatternDumper<ChannelBuilder, ChannelDumper> Patterns;
  };
}  // namespace Dump::SQTracker

bool DumpSQTracker(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::SQTracker::Dumper>(data, &Formats::Chiptune::CreateSQTrackerDecoder,
                                                     &Formats::Chiptune::SQTracker::ParseCompiled);
}
