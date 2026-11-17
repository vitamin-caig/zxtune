/**
 *
 * @file
 *
 * @brief  ASC Sound Master tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/ascsoundmaster.h"

#include "formats/test/dumpers/tracking/dumpers.h"

namespace Dump::ASCSoundMaster
{
  using namespace Formats::Chiptune::ASCSoundMaster;

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {
  public:
    void SetEnvelopeType(uint_t type) override
    {
      Command("EnvelopeType"sv, type);
    }

    void SetEnvelopeTone(uint_t tone) override
    {
      Command("EnvelopeTone"sv, tone);
    }

    void SetEnvelope() override
    {
      Command("Envelope"sv);
    }

    void SetNoEnvelope() override
    {
      Command("NoEnvelope"sv);
    }

    void SetNoise(uint_t val) override
    {
      Command("Noise"sv, val);
    }

    void SetContinueSample() override
    {
      Command("ContinueSample"sv);
    }

    void SetContinueOrnament() override
    {
      Command("ContinueOrnament"sv);
    }

    void SetGlissade(int_t val) override
    {
      Command("Glissade"sv, val);
    }

    void SetSlide(int_t steps) override
    {
      Command("Slide"sv, steps);
    }

    void SetNoteSlide(int_t steps, bool useToneSliding) override
    {
      Command("NoteSlide"sv, "steps"sv, steps, "useToneSliding"sv, useToneSliding);
    }

    void SetVolumeSlide(uint_t period, int_t delta) override
    {
      Command("VolumeSlide"sv, "period"sv, period, "delta"sv, delta);
    }

    void SetBreakSample() override
    {
      Command("BreakSample"sv);
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
}  // namespace Dump::ASCSoundMaster

bool DumpASCSoundMaster0x(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::ASCSoundMaster::Dumper>(data, &Formats::Chiptune::CreateASCSoundMaster0xDecoder,
                                                          &Formats::Chiptune::ASCSoundMaster::Ver0::Parse);
}

bool DumpASCSoundMaster1x(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::ASCSoundMaster::Dumper>(data, &Formats::Chiptune::CreateASCSoundMaster1xDecoder,
                                                          &Formats::Chiptune::ASCSoundMaster::Ver1::Parse);
}
