/*
Abstract:
  SoundTrackerPro support implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "soundtrackerpro.h"
#include "ay_base.h"
//library includes
#include <formats/chiptune/aym/soundtrackerpro.h>
//boost includes
#include <boost/make_shared.hpp>

namespace SoundTrackerPro
{
  class DataBuilder : public Formats::Chiptune::SoundTrackerPro::Builder
  {
  public:
    DataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props)
      : Data(data)
      , Properties(props)
      , Builder(PatternsBuilder::Create<Devices::AYM::CHANNELS>())
    {
      Data->Patterns = Builder.GetPatterns();
      Properties->SetFreqtable(TABLE_SOUNDTRACKER_PRO);
    }

    virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
    {
      return *Properties;
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      Data->InitialTempo = tempo;
    }

    virtual void SetSample(uint_t index, const Formats::Chiptune::SoundTrackerPro::Sample& sample)
    {
      Data->Samples.Add(index, sample);
    }

    virtual void SetOrnament(uint_t index, const Formats::Chiptune::SoundTrackerPro::Ornament& ornament)
    {
      Data->Ornaments.Add(index, Ornament(ornament.Loop, ornament.Lines.begin(), ornament.Lines.end()));
    }

    virtual void SetPositions(const std::vector<Formats::Chiptune::SoundTrackerPro::PositionEntry>& positions, uint_t loop)
    {
      Data->Order = boost::make_shared<OrderListWithTransposition>(loop, positions.begin(), positions.end());
    }

    virtual Formats::Chiptune::PatternBuilder& StartPattern(uint_t index)
    {
      Builder.SetPattern(index);
      return Builder;
    }

    virtual void StartChannel(uint_t index)
    {
      Builder.SetChannel(index);
    }

    virtual void SetRest()
    {
      Builder.GetChannel().SetEnabled(false);
    }

    virtual void SetNote(uint_t note)
    {
      Builder.GetChannel().SetEnabled(true);
      Builder.GetChannel().SetNote(note);
    }

    virtual void SetSample(uint_t sample)
    {
      Builder.GetChannel().SetSample(sample);
    }

    virtual void SetOrnament(uint_t ornament)
    {
      Builder.GetChannel().SetOrnament(ornament);
    }

    virtual void SetEnvelope(uint_t type, uint_t value)
    {
      Builder.GetChannel().AddCommand(ENVELOPE, type, value);
    }

    virtual void SetNoEnvelope()
    {
      Builder.GetChannel().AddCommand(NOENVELOPE);
    }

    virtual void SetGliss(uint_t target)
    {
      Builder.GetChannel().AddCommand(GLISS, target);
    }
    
    virtual void SetVolume(uint_t vol)
    {
      Builder.GetChannel().SetVolume(vol);
    }
  private:
    const ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;
    PatternsBuilder Builder;
  };

  struct ChannelState
  {
    ChannelState()
      : Enabled(false), Envelope(false), Volume(0)
      , Note(0), SampleNum(0), PosInSample(0)
      , OrnamentNum(0), PosInOrnament(0)
      , TonSlide(0), Glissade(0)
    {
    }
    bool Enabled;
    bool Envelope;
    uint_t Volume;
    uint_t Note;
    uint_t SampleNum;
    uint_t PosInSample;
    uint_t OrnamentNum;
    uint_t PosInOrnament;
    int_t TonSlide;
    int_t Glissade;
  };

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
      : Data(data)
    {
    }

    virtual void Reset()
    {
      std::fill(PlayerState.begin(), PlayerState.end(), ChannelState());
    }

    virtual void SynthesizeData(const TrackModelState& state, AYM::TrackBuilder& track)
    {
      if (0 == state.Quirk())
      {
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(state, track);
    }

  private:
    void GetNewLineState(const TrackModelState& state, AYM::TrackBuilder& track)
    {
      if (const Line::Ptr line = state.LineObject())
      {
        for (uint_t chan = 0; chan != Devices::AYM::CHANNELS; ++chan)
        {
          if (const Cell::Ptr src = line->GetChannel(chan))
          {
            GetNewChannelState(*src, PlayerState[chan], track);
          }
        }
      }
    }

    void GetNewChannelState(const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        dst.Enabled = *enabled;
        dst.PosInSample = 0;
        dst.PosInOrnament = 0;
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
        dst.PosInSample = 0;
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.OrnamentNum = *ornament;
        dst.PosInOrnament = 0;
      }
      if (const uint_t* volume = src.GetVolume())
      {
        dst.Volume = *volume;
      }
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case ENVELOPE:
          if (it->Param1)
          {
            track.SetEnvelopeType(it->Param1);
            track.SetEnvelopeTone(it->Param2);
          }
          dst.Envelope = true;
          break;
        case NOENVELOPE:
          dst.Envelope = false;
          break;
        case GLISS:
          dst.Glissade = it->Param1;
          break;
        default:
          assert(!"Invalid command");
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

    void SynthesizeChannel(const TrackState& state, ChannelState& dst, AYM::ChannelBuilder& channel, AYM::TrackBuilder& track)
    {
      if (!dst.Enabled)
      {
        channel.SetLevel(0);
        channel.DisableTone();
        channel.DisableNoise();
        return;
      }

      const Sample& curSample = Data->Samples.Get(dst.SampleNum);
      const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Ornament& curOrnament = Data->Ornaments.Get(dst.OrnamentNum);

      //calculate tone
      dst.TonSlide += dst.Glissade;

      //apply level
      channel.SetLevel(int_t(curSampleLine.Level) - dst.Volume);
      //apply envelope
      if (curSampleLine.EnvelopeMask && dst.Envelope)
      {
        channel.EnableEnvelope();
      }
      //apply tone
      const int_t halftones = int_t(dst.Note) +
                              Data->Order->GetTransposition(state.Position()) +
                              (dst.Envelope ? 0 : curOrnament.GetLine(dst.PosInOrnament));
      channel.SetTone(halftones, dst.TonSlide + curSampleLine.Vibrato);
      if (curSampleLine.ToneMask)
      {
        channel.DisableTone();
      }
      //apply noise
      if (!curSampleLine.NoiseMask)
      {
        track.SetNoise(curSampleLine.Noise);
      }
      else
      {
        channel.DisableNoise();
      }

      if (++dst.PosInOrnament >= curOrnament.GetSize())
      {
        dst.PosInOrnament = curOrnament.GetLoop();
      }

      if (++dst.PosInSample >= curSample.GetSize())
      {
        if (curSample.GetLoop() >= 0)
        {
          dst.PosInSample = curSample.GetLoop();
        }
        else
        {
          dst.Enabled = false;
        }
      }
    }
  private:
    const ModuleData::Ptr Data;
    boost::array<ChannelState, Devices::AYM::CHANNELS> PlayerState;
  };

  class Chiptune : public AYM::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateTrackInfo(Data, Devices::AYM::CHANNELS))
    {
    }

    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual ModuleProperties::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual AYM::DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams) const
    {
      const TrackStateIterator::Ptr iterator = CreateTrackStateIterator(Data);
      const AYM::DataRenderer::Ptr renderer = boost::make_shared<DataRenderer>(Data);
      return AYM::CreateDataIterator(trackParams, iterator, renderer);
    }
  private:
    const ModuleData::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };

}
 
namespace SoundTrackerPro
{
  std::auto_ptr<Formats::Chiptune::SoundTrackerPro::Builder> CreateDataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props)
  {
    return std::auto_ptr<Formats::Chiptune::SoundTrackerPro::Builder>(new DataBuilder(data, props));
  }

  AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties)
  {
    return boost::make_shared<Chiptune>(data, properties);
  }
}
