/**
 *
 * @file
 *
 * @brief  DigitalStudio tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/digitalstudio.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::DigitalStudio
{
  using namespace Formats::Chiptune::Digital;
  using namespace Formats::Chiptune::DigitalStudio;

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
}  // namespace Dump::DigitalStudio

bool DumpDigitalStudio(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::DigitalStudio::Dumper>(data, &Formats::Chiptune::CreateDigitalStudioDecoder,
                                                         &Formats::Chiptune::DigitalStudio::Parse);
}
