/**
 *
 * @file
 *
 * @brief  DigitalMusicMaker tracks dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/digitalmusicmaker.h"

#include "formats/test/dumpers/tracking/dumpers.h"

#include <memory>

namespace Dump::DigitalMusicMaker
{
  using namespace Formats::Chiptune::DigitalMusicMaker;

  class ChannelDumper : public Dump::ChannelDumperBase<ChannelBuilder>
  {
  public:
    void SetFloat(int_t direction) override
    {
      Command("Float"sv, direction);
    }

    void SetFloatParam(uint_t step) override
    {
      Command("FloatParam"sv, step);
    }

    void SetVibrato() override
    {
      Command("Vibrato"sv);
    }

    void SetVibratoParams(uint_t step, uint_t period) override
    {
      Command("VibratoParams"sv, "step"sv, step, "period"sv, period);
    }

    void SetArpeggio() override
    {
      Command("Arpeggio"sv);
    }

    void SetArpeggioParams(uint_t step, uint_t period) override
    {
      Command("ArpeggioParams"sv, "step"sv, step, "period"sv, period);
    }

    void SetSlide(int_t direction) override
    {
      Command("Slide"sv, direction);
    }

    void SetSlideParams(uint_t step, uint_t period) override
    {
      Command("SlideParams"sv, "step"sv, step, "period"sv, period);
    }

    void SetDoubleNote() override
    {
      Command("DoubleNote"sv);
    }

    void SetDoubleNoteParam(uint_t period) override
    {
      Command("DoubleNoteParam"sv, period);
    }

    void SetVolumeAttack() override
    {
      Command("VolumeAttack"sv);
    }

    void SetVolumeAttackParams(uint_t limit, uint_t period) override
    {
      Command("VolumeAttackParams"sv, "limit"sv, limit, "period"sv, period);
    }

    void SetVolumeDecay() override
    {
      Command("VolumeDecay"sv);
    }

    void SetVolumeDecayParams(uint_t limit, uint_t period) override
    {
      Command("VolumeDecayParams"sv, "limit"sv, limit, "period"sv, period);
    }

    void SetMixSample(uint_t idx) override
    {
      Command("MixSample"sv, idx);
    }

    void SetNoEffects() override
    {
      Command("NoEffects"sv);
    }
  };

  class NullChannelDumper : public ChannelBuilder
  {
  public:
    void SetRest() override {}

    void SetNote(uint_t /*note*/) override {}

    void SetSample(uint_t /*sample*/) override {}

    void SetOrnament(uint_t /*ornament*/) override {}

    void SetVolume(uint_t /*volume*/) override {}

    void SetTempo(uint_t /*tempo*/) override {}

    void SetFloat(int_t /*direction*/) override {}

    void SetFloatParam(uint_t /*step*/) override {}

    void SetVibrato() override {}

    void SetVibratoParams(uint_t /*step*/, uint_t /*period*/) override {}

    void SetArpeggio() override {}

    void SetArpeggioParams(uint_t /*step*/, uint_t /*period*/) override {}

    void SetSlide(int_t /*direction*/) override {}

    void SetSlideParams(uint_t /*step*/, uint_t /*period*/) override {}

    void SetDoubleNote() override {}

    void SetDoubleNoteParam(uint_t /*period*/) override {}

    void SetVolumeAttack() override {}

    void SetVolumeAttackParams(uint_t /*limit*/, uint_t /*period*/) override {}

    void SetVolumeDecay() override {}

    void SetVolumeDecayParams(uint_t /*limit*/, uint_t /*period*/) override {}

    void SetMixSample(uint_t /*idx*/) override {}

    void SetNoEffects() override {}
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

    void SetSample(uint_t /*index*/, std::size_t /*loop*/, Binary::View /*sample*/) override {}

    std::unique_ptr<ChannelBuilder> SetSampleMixin(uint_t /*index*/, uint_t /*period*/) override
    {
      return std::make_unique<NullChannelDumper>();
    }

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
}  // namespace Dump::DigitalMusicMaker

bool DumpDigitalMusicMaker(const Binary::Container& data)
{
  return Dump::DumpTracking<Dump::DigitalMusicMaker::Dumper>(data, &Formats::Chiptune::CreateDigitalMusicMakerDecoder,
                                                             &Formats::Chiptune::DigitalMusicMaker::Parse);
}
