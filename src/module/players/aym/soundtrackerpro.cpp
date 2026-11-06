/**
 *
 * @file
 *
 * @brief  SoundTrackerPro-based chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/aym/soundtrackerpro.h"

#include "module/players/aym/aym_base_track.h"
#include "module/players/aym/aym_chiptune.h"
#include "module/players/aym/aym_properties_helper.h"
#include "module/players/generic_presentation.h"
#include "module/players/properties_meta.h"
#include "module/players/simple_orderlist.h"
#include "module/players/track_builder.h"

#include "binary/container.h"
#include "parameters/container.h"

#include "make_ptr.h"

#include <array>

namespace Module::SoundTrackerPro
{
  // Supported commands and their parameters
  enum CmdType
  {
    // type, tone
    ENVELOPE,
    // ornament number
    ORNAMENT,
    // glissade
    GLISS
  };

  using Formats::Chiptune::SoundTrackerPro::Sample;
  using Formats::Chiptune::SoundTrackerPro::Ornament;

  using OrderListWithTransposition =
      SimpleOrderListWithTransposition<Formats::Chiptune::SoundTrackerPro::PositionEntry>;

  using ModuleData = AYM::ModuleData<OrderListWithTransposition, Sample, Ornament>;

  class DataChannelBuilder : public TrackChannelBuilder<Formats::Chiptune::SoundTrackerPro::ChannelBuilder>
  {
  public:
    void SetOrnament(uint_t ornament) override
    {
      auto& chan = GetCell();
      // Keep the cell field for the generic representation, but also emit an
      // ordered command so playback can reproduce the native state machine.
      chan.SetOrnament(ornament);
      chan.GetCommands().AddOrReplace(ORNAMENT, ornament);
    }
    void SetEnvelope(uint_t type, uint_t value) override
    {
      GetCommands().AddOrReplace(ENVELOPE, type, value);
    }
    void SetGliss(int_t step) override
    {
      GetCommands().AddOrReplace(GLISS, 0, step);
    }
  };

  using LineBuilder = TrackLineBuilder<Formats::Chiptune::SoundTrackerPro::ChannelBuilder, DataChannelBuilder>;
  using PatternsBuilder = TrackPatternsBuilder<Formats::Chiptune::SoundTrackerPro::ChannelBuilder, DataChannelBuilder>;

  class DataBuilder : public TrackDataBuilder<Formats::Chiptune::SoundTrackerPro::Builder, ModuleData, PatternsBuilder>
  {
  public:
    explicit DataBuilder(AYM::PropertiesHelper& props)
      : TrackDataBuilder(props)
      , Properties(props)
    {
      Properties.SetFrequencyTable(AYM::TABLE_SOUNDTRACKER_PRO);
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSample(uint_t index, Formats::Chiptune::SoundTrackerPro::Sample sample) override
    {
      Data->Samples.Add(index, std::move(sample));
    }

    void SetOrnament(uint_t index, Formats::Chiptune::SoundTrackerPro::Ornament ornament) override
    {
      Data->Ornaments.Add(index, std::move(ornament));
    }

    void SetPositions(Formats::Chiptune::SoundTrackerPro::Positions positions) override
    {
      Data->Order = MakePtr<OrderListWithTransposition>(positions.Loop, std::move(positions.Lines));
    }

  private:
    AYM::PropertiesHelper& Properties;
  };

  struct ChannelState
  {
    ChannelState()
      : SampleNum(Formats::Chiptune::SoundTrackerPro::DEFAULT_SAMPLE)
      , OrnamentNum(Formats::Chiptune::SoundTrackerPro::DEFAULT_ORNAMENT)
    {}
    bool Enabled = false;
    bool Envelope = false;
    uint_t Volume = 0;
    uint_t Note = 0;
    uint_t SampleNum;
    uint_t PosInSample = 0;
    uint_t OrnamentNum;
    uint_t PosInOrnament = 0;
    int_t TonSlide = 0;
    int_t Glissade = 0;
  };

  class DataRenderer
  {
  public:
    explicit DataRenderer(const ModuleData& data)
      : Data(data)
    {}

    void Reset()
    {
      std::fill(PlayerState.begin(), PlayerState.end(), ChannelState());
    }

    void SynthesizeData(const TrackState& state, AYM::TrackBuilder& track)
    {
      if (0 == state.Quirk)
      {
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(state, track);
    }

  private:
    void GetNewLineState(const TrackState& state, AYM::TrackBuilder& track)
    {
      if (const auto* const line = Data.GetLine(state))
      {
        line->ForEachChannel([&](auto chan, const auto& src) { GetNewChannelState(src, PlayerState[chan], track); });
      }
    }

    static void GetNewChannelState(const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        dst.Enabled = *enabled;
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.Note = *note;
        dst.PosInSample = 0;
        dst.PosInOrnament = 0;
        dst.TonSlide = 0;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleNum = *sample;
      }
      if (const uint_t* volume = src.GetVolume())
      {
        dst.Volume = *volume;
      }
      for (const auto& cmd : src.GetCommands())
      {
        switch (cmd.Type)
        {
        case ENVELOPE:
          if (cmd.Param1)
          {
            track.SetEnvelopeType(cmd.Param1);
            track.SetEnvelopeTone(cmd.Param2);
          }
          dst.Envelope = true;
          dst.Glissade = 0;
          dst.PosInOrnament = 0;
          break;
        case ORNAMENT:
          dst.Envelope = false;
          dst.Glissade = 0;
          dst.OrnamentNum = cmd.Param1;
          break;
        case GLISS:
          dst.Glissade = cmd.Param2;
          break;
        default:
          break;
        }
      }
    }

    void SynthesizeChannelsData(const TrackState& state, AYM::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        AYM::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(state, PlayerState[chan], channel, track);
      }
    }

    void SynthesizeChannel(const TrackState& state, ChannelState& dst, AYM::ChannelBuilder& channel,
                           AYM::TrackBuilder& track)
    {
      if (!dst.Enabled)
      {
        channel.SetLevel(0);
        channel.DisableTone();
        channel.DisableNoise();
        return;
      }

      const Sample& curSample = Data.Samples.Get(dst.SampleNum);
      const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Ornament& curOrnament = Data.Ornaments.Get(dst.OrnamentNum);

      // calculate tone
      dst.TonSlide += dst.Glissade;

      // apply level
      channel.SetLevel(int_t(curSampleLine.Level) - dst.Volume);
      // apply envelope
      if (curSampleLine.EnvelopeMask && dst.Envelope)
      {
        channel.EnableEnvelope();
      }
      // apply tone
      const int_t halftones = int_t(dst.Note) + Data.Order->GetTransposition(state.Position)
                              + (dst.Envelope ? 0 : curOrnament.GetLine(dst.PosInOrnament));
      channel.SetTone(halftones, dst.TonSlide + curSampleLine.Vibrato);
      if (curSampleLine.ToneMask)
      {
        channel.DisableTone();
      }
      // apply noise
      if (!curSampleLine.NoiseMask)
      {
        track.SetNoise(curSampleLine.Noise);
      }
      else
      {
        channel.DisableNoise();
      }

      // The native player freezes the ornament position while the envelope is
      // enabled (it sets the ornament length to 1), so it stays wherever the
      // envelope left it.
      if (!dst.Envelope && ++dst.PosInOrnament >= curOrnament.GetSize())
      {
        dst.PosInOrnament = curOrnament.GetLoop();
      }

      if (++dst.PosInSample >= curSample.GetSize())
      {
        const auto loop = curSample.GetLoop();
        if (loop < dst.PosInSample)
        {
          dst.PosInSample = loop;
        }
        else
        {
          dst.Enabled = false;
        }
      }
    }

  private:
    const ModuleData& Data;
    std::array<ChannelState, AYM::TRACK_CHANNELS> PlayerState;
  };

  namespace Presentation
  {
    using Delimiter = Module::Presentation::Delimiter;
    using Layout = Module::Presentation::Layout<'0'>;

    struct Channel
    {
      Delimiter Separator;
      Module::Presentation::NoteAndOctave Note;
      char Space = ' ';
      Layout::Hex<1> Sample;
      Layout::Hex<1> Mode;
      Layout::Hex<1> Param1;
      Layout::Hex<1> Param2;
    };

    struct Row
    {
      Module::Presentation::Dec<2> Index;
      Channel Channels[AYM::TRACK_CHANNELS];
    };

    void FillCell(const Cell& cell, Channel& txt)
    {
      txt.Note.Set(&cell);
      if (const auto* sample = cell.GetSample())
      {
        // Instruments are numbered from 1 in the editor.
        txt.Sample = *sample + 1;
      }
      if (const auto* vol = cell.GetVolume())
      {
        // Volume is stored as attenuation; the editor shows the remaining level.
        txt.Mode = 0xf;
        txt.Param1 = 0xf - *vol;
      }
      if (const auto* orn = cell.GetOrnament())
      {
        txt.Mode = 0xf;
        txt.Param2 = *orn;
      }
      for (const auto& cmd : cell.GetCommands())
      {
        switch (cmd.Type)
        {
        case ENVELOPE:
          txt.Mode = cmd.Param1;
          txt.Param1 = static_cast<uint_t>(cmd.Param2) >> 4;
          txt.Param2 = static_cast<uint_t>(cmd.Param2) & 0x0f;
          break;
        case GLISS:
          if (cmd.Param2)
          {
            // Native presentation shows the raw two's-complement slide byte with mode 1,
            // even for negative slides. Mode 1 denotes a frequency decrease, but the byte
            // is an AY tone period offset, so its sign is inverse to the frequency change.
            const uint_t raw = static_cast<uint8_t>(cmd.Param2);
            txt.Mode = 1;
            txt.Param1 = raw >> 4;
            txt.Param2 = raw & 0x0f;
          }
          break;
        default:
          break;
        }
      }
    }

    void FillLine(const Line& line, Row& row)
    {
      for (uint_t channel = 0; channel != AYM::TRACK_CHANNELS; ++channel)
      {
        if (const auto* cell = line.GetChannel(channel))
        {
          FillCell(*cell, row.Channels[channel]);
        }
      }
    }

    String ForPattern(const Pattern& pattern, uint_t /*channels*/)
    {
      String result;
      const auto lines = pattern.GetSize();
      result.reserve(lines * (sizeof(Row) + 1));
      for (uint_t idx = 0; idx != lines; ++idx)
      {
        auto& dst = Module::Presentation::AppendLine<Row>(result);
        dst.Index = idx;
        if (const auto* line = pattern.GetLine(idx))
        {
          FillLine(*line, dst);
        }
      }
      return result;
    }
  }  // namespace Presentation
}  // namespace Module::SoundTrackerPro

namespace Module::AYM
{
  Chiptune::Ptr CreateSoundTrackerProChiptune(const Binary::Container& rawData, PropertiesHelper& props)
  {
    SoundTrackerPro::DataBuilder dataBuilder(props);
    if (const auto container = Formats::Chiptune::SoundTrackerPro::ParseCompiled(rawData, dataBuilder))
    {
      props.SetSource(*container);
      return CreateTrackingChiptune<SoundTrackerPro::DataRenderer>(dataBuilder.CaptureResult(),
                                                                   &SoundTrackerPro::Presentation::ForPattern);
    }
    return {};
  }
}  // namespace Module::AYM
