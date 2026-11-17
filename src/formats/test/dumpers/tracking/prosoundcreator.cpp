/**
 *
 * @file
 *
 * @brief  ProSoundCreator tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/prosoundcreator.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::ProSoundCreator
{
  using namespace Formats::Chiptune::ProSoundCreator;

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {
  public:
    void SetEnvelope(uint_t type, uint_t tone) override
    {
      Command("Envelope"sv, "type"sv, type, "tone"sv, tone);
    }

    void SetEnvelope() override
    {
      Command("EnableEnvelope"sv);
    }

    void SetNoEnvelope() override
    {
      Command("NoEnvelope"sv);
    }

    void SetNoiseBase(uint_t val) override
    {
      Command("NoiseBase"sv, val);
    }

    void SetBreakSample() override
    {
      Command("BreakSample"sv);
    }

    void SetBreakOrnament() override
    {
      Command("BreakOrnament"sv);
    }

    void SetNoOrnament() override
    {
      Command("NoOrnament"sv);
    }

    void SetGliss(uint_t absStep) override
    {
      Command("Gliss"sv, absStep);
    }

    void SetSlide(int_t delta) override
    {
      Command("Slide"sv, delta);
    }

    void SetVolumeSlide(uint_t period, int_t delta) override
    {
      Command("VolumeSlide"sv, "period"sv, period, "delta"sv, delta);
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
}  // namespace Dump::ProSoundCreator

bool DumpProSoundCreator(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::ProSoundCreator::Dumper>(data, &Formats::Chiptune::CreateProSoundCreatorDecoder,
                                                           &Formats::Chiptune::ProSoundCreator::Parse);
}
