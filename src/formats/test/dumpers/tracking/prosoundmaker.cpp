/**
 *
 * @file
 *
 * @brief  ProSoundMaker tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/prosoundmaker.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::ProSoundMaker
{
  using namespace Formats::Chiptune::ProSoundMaker;

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {
  public:
    void DisableOrnament() override
    {
      Command("DisableOrnament"sv);
    }

    void SetEnvelopeReinit(bool enabled) override
    {
      Command("EnvelopeReinit"sv, enabled);
    }

    void SetEnvelopeTone(uint_t type, uint_t tone) override
    {
      Command("EnvelopeTone"sv, "type"sv, type, "tone"sv, tone);
    }

    void SetEnvelopeNote(uint_t type, uint_t note) override
    {
      Command("EnvelopeNote"sv, "type"sv, type, "note"sv, note);
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
}  // namespace Dump::ProSoundMaker

bool DumpProSoundMaker(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::ProSoundMaker::Dumper>(data, &Formats::Chiptune::CreateProSoundMakerCompiledDecoder,
                                                         &Formats::Chiptune::ProSoundMaker::ParseCompiled);
}
