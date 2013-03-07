/*
Abstract:
  SoundTracker support implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "soundtracker.h"
#include "ay_base.h"
#include "core/plugins/utils.h"
//library includes
#include <formats/chiptune/soundtracker.h>
//boost includes
#include <boost/make_shared.hpp>

namespace SoundTracker
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class DataBuilder : public Formats::Chiptune::SoundTracker::Builder
  {
  public:
    DataBuilder(SoundTracker::ModuleData::RWPtr data, ModuleProperties::RWPtr props)
      : Data(data)
      , Properties(props)
      , Context(*Data)
    {
    }

    virtual void SetProgram(const String& program)
    {
      Properties->SetProgram(OptimizeString(program));
      Properties->SetFreqtable(TABLE_SOUNDTRACKER);
    }

    virtual void SetTitle(const String& title)
    {
      Properties->SetTitle(OptimizeString(title));
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      Data->InitialTempo = tempo;
    }

    virtual void SetSample(uint_t index, const Formats::Chiptune::SoundTracker::Sample& sample)
    {
      Data->Samples.resize(index + 1, sample);
    }

    virtual void SetOrnament(uint_t index, const Formats::Chiptune::SoundTracker::Ornament& ornament)
    {
      Data->Ornaments.resize(index + 1);
      Data->Ornaments[index] = Track::Ornament(0, ornament.begin(), ornament.end());
    }

    virtual void SetPositions(const std::vector<Formats::Chiptune::SoundTracker::PositionEntry>& positions)
    {
      using namespace Formats::Chiptune::SoundTracker;
      std::vector<uint_t> indices(positions.size());
      std::transform(positions.begin(), positions.end(), indices.begin(), boost::mem_fn(&PositionEntry::PatternIndex));
      Data->Order = boost::make_shared<SimpleOrderList>(indices.begin(), indices.end(), 0);
      Data->Transpositions.resize(positions.size());
      std::transform(positions.begin(), positions.end(), Data->Transpositions.begin(), boost::mem_fn(&PositionEntry::Transposition));
    }

    virtual void StartPattern(uint_t index)
    {
      Context.SetPattern(index);
    }

    virtual void FinishPattern(uint_t size)
    {
      Context.FinishPattern(size);
    }

    virtual void StartLine(uint_t index)
    {
      Context.SetLine(index);
    }

    virtual void StartChannel(uint_t index)
    {
      Context.SetChannel(index);
    }

    virtual void SetRest()
    {
      Context.CurChannel->SetEnabled(false);
    }

    virtual void SetNote(uint_t note)
    {
      Context.CurChannel->SetEnabled(true);
      Context.CurChannel->SetNote(note);
    }

    virtual void SetSample(uint_t sample)
    {
      Context.CurChannel->SetSample(sample);
    }

    virtual void SetOrnament(uint_t ornament)
    {
      Context.CurChannel->SetOrnament(ornament);
    }

    virtual void SetEnvelope(uint_t type, uint_t value)
    {
      Context.CurChannel->AddCommand(SoundTracker::ENVELOPE, type, value);
    }

    virtual void SetNoEnvelope()
    {
      Context.CurChannel->AddCommand(SoundTracker::NOENVELOPE);
    }
  private:
    const SoundTracker::ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;

    Track::BuildContext Context;
  };

  class ChannelBuilder
  {
  public:
    ChannelBuilder(uint_t transposition, AYM::TrackBuilder& track, uint_t chanNum)
      : Transposition(transposition)
      , Track(track)
      , Channel(Track.GetChannel(chanNum))
    {
    }

    void SetLevel(uint_t level)
    {
      Channel.SetLevel(level);
    }

    void EnableEnvelope()
    {
      Channel.EnableEnvelope();
    }

    void SetTone(int_t halftones, int_t offset)
    {
      Channel.SetTone(halftones + Transposition, offset);
    }

    void DisableTone()
    {
      Channel.DisableTone();
    }

    void SetNoise(int_t level)
    {
      Track.SetNoise(level);
    }

    void DisableNoise()
    {
      Channel.DisableNoise();
    }
  private:
    const uint_t Transposition;
    AYM::TrackBuilder& Track;
    AYM::ChannelBuilder Channel;
  };

  struct EnvelopeState
  {
    explicit EnvelopeState(uint_t& type, uint_t& tone)
      : Type(type)
      , Tone(tone)
      , Enabled(0)
    {
    }

    void Reset()
    {
      Enabled = 0;
    }

    void SetNewState(const Cell& src)
    {
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        ApplyCommand(*it);
      }
    }

    void Iterate()
    {
      if (2 == Enabled)
      {
        Type = 0;
      }
      else if (1 == Enabled)
      {
        Enabled = 2;
        Type = 0;
      }
    }

    void Synthesize(ChannelBuilder& channel) const
    {
      if (Enabled)
      {
        channel.EnableEnvelope();
      }
    } 
  private:
    void ApplyCommand(const Command& command)
    {
      if (command == ENVELOPE)
      {
        Type = command.Param1;
        Tone = command.Param2;
        Enabled = 1;
      }
      else if (command == NOENVELOPE)
      {
        Enabled = 0;
      }
    }
  private:
    uint_t& Type;
    uint_t& Tone;
    uint_t Enabled;
  };

  struct StateCursor
  {
    int_t CountDown;
    uint_t Position;

    StateCursor()
      : CountDown(-1)
      , Position(0)
    {
    }

    void Next(const Track::Sample& sample)
    {
      if (!IsValid())
      {
        return;
      }
      --CountDown;
      Position = (Position + 1) & 0x1f;
      if (0 == CountDown)
      {
        if (const uint_t loop = sample.GetLoop())
        {
          Position = loop & 0x1f;
          CountDown = sample.GetLoopLimit() - loop;
        }
        else
        {
          CountDown = -1;
        }
      }
    }

    bool IsValid() const
    {
      return CountDown >= 0;
    }
  };

  struct ChannelState
  {
    explicit ChannelState(ModuleData::Ptr data, uint_t& envType, uint_t& envTone)
      : Data(data)
      , Note()
      , CurSample(GetStubSample())
      , CurOrnament(GetStubOrnament())
      , EnvState(envType, envTone)
    {
    }

    void Reset()
    {
      Note = 0;
      Cursor = StateCursor();
      CurSample = GetStubSample();
      CurOrnament = GetStubOrnament();
      EnvState.Reset();
    }

    void SetNewState(const Cell& src)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        Cursor.CountDown = *enabled ? 32 : -1;
      }
      if (const uint_t* note = src.GetNote())
      {
        Note = *note;
        Cursor.Position = 0;
      }
      if (const uint_t* sample = src.GetSample())
      {
        CurSample = *sample < Data->Samples.size() ? &Data->Samples[*sample] : GetStubSample();
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        CurOrnament = *ornament < Data->Ornaments.size() ? &Data->Ornaments[*ornament] : GetStubOrnament();
      }
      EnvState.SetNewState(src);
    }

    void Synthesize(ChannelBuilder& channel) const
    {
      StateCursor nextState(Cursor);
      nextState.Next(*CurSample);
      if (!nextState.IsValid())
      {
        channel.SetLevel(0);
        return;
      }

      const uint_t nextPosition = (nextState.Position - 1) & 0x1f;
      const Track::Sample::Line& curSampleLine = CurSample->GetLine(nextPosition);
      //apply level
      channel.SetLevel(curSampleLine.Level);
      //apply tone
      const int_t halftones = int_t(Note) + CurOrnament->GetLine(nextPosition);
      channel.SetTone(halftones, curSampleLine.Effect);
      if (curSampleLine.EnvelopeMask)
      {
        channel.DisableTone();
      }
      //apply noise
      if (!curSampleLine.NoiseMask)
      {
        channel.SetNoise(curSampleLine.Noise);
      }
      else
      {
        channel.DisableNoise();
      }
      EnvState.Synthesize(channel);
    }

    void Iterate()
    {
      Cursor.Next(*CurSample);
      if (Cursor.IsValid())
      {
        EnvState.Iterate();
      }
    }
  private:
    static const Track::Sample* GetStubSample()
    {
      static const Track::Sample stubSample;
      return &stubSample;
    }

    static const Track::Ornament* GetStubOrnament()
    {
      static const Track::Ornament stubOrnament;
      return &stubOrnament;
    }
  private:
    const ModuleData::Ptr Data;
    uint_t Note;
    StateCursor Cursor;
    const Track::Sample* CurSample;
    const Track::Ornament* CurOrnament;
    EnvelopeState EnvState;
  };

  class DataIterator : public AYM::DataIterator
  {
  public:
    DataIterator(AYM::TrackParameters::Ptr trackParams, TrackStateIterator::Ptr delegate, ModuleData::Ptr data)
      : TrackParams(trackParams)
      , Delegate(delegate)
      , State(Delegate->GetStateObserver())
      , Data(data)
      , StateA(Data, EnvType, EnvTone)
      , StateB(Data, EnvType, EnvTone)
      , StateC(Data, EnvType, EnvTone)
      , EnvType(0)
      , EnvTone(0)
    {
      SwitchToNewLine();
    }

    virtual void Reset()
    {
      Delegate->Reset();
      StateA.Reset();
      StateB.Reset();
      StateC.Reset();
      EnvType = EnvTone = 0;
      SwitchToNewLine();
    }

    virtual bool IsValid() const
    {
      return Delegate->IsValid();
    }

    virtual void NextFrame(bool looped)
    {
      if (Delegate->IsValid())
      {
        Delegate->NextFrame(looped);
        StateA.Iterate();
        StateB.Iterate();
        StateC.Iterate();
        if (Delegate->IsValid() && 0 == State->Quirk())
        {
          SwitchToNewLine();
        }
      }
    }

    virtual TrackState::Ptr GetStateObserver() const
    {
      return State;
    }

    virtual void GetData(Devices::AYM::DataChunk& chunk) const
    {
      if (Delegate->IsValid())
      {
        AYM::TrackBuilder track(TrackParams->FreqTable());

        SynthesizeChannelsData(track);
        track.GetResult(chunk);
      }
      else
      {
        assert(!"SoundTracker: invalid iterator access");
        chunk = Devices::AYM::DataChunk();
      }
    }
  private:
    void SwitchToNewLine()
    {
      assert(0 == State->Quirk());
      if (const Line::Ptr line = State->LineObject())
      {
        if (const Cell::Ptr chan = line->GetChannel(0))
        {
          StateA.SetNewState(*chan);
        }
        if (const Cell::Ptr chan = line->GetChannel(1))
        {
          StateB.SetNewState(*chan);
        }
        if (const Cell::Ptr chan = line->GetChannel(2))
        {
          StateC.SetNewState(*chan);
        }
      }
    }

    void SynthesizeChannelsData(AYM::TrackBuilder& track) const
    {
      const uint_t transposition = Data->Transpositions[State->Position()];
      {
        ChannelBuilder channel(transposition, track, 0);
        StateA.Synthesize(channel);
      }
      {
        ChannelBuilder channel(transposition, track, 1);
        StateB.Synthesize(channel);
      }
      {
        ChannelBuilder channel(transposition, track, 2);
        StateC.Synthesize(channel);
      }
      if (EnvType)
      {
        track.SetEnvelopeType(EnvType);
        track.SetEnvelopeTone(EnvTone);
      }
    }
  private:
    const AYM::TrackParameters::Ptr TrackParams;
    const TrackStateIterator::Ptr Delegate;
    const TrackModelState::Ptr State;
    const ModuleData::Ptr Data;
    ChannelState StateA;
    ChannelState StateB;
    ChannelState StateC;
    uint_t EnvType, EnvTone;
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
      const TrackStateIterator::Ptr iter = CreateTrackStateIterator(Info, Data);
      return boost::make_shared<DataIterator>(trackParams, iter, Data);
    }
  private:
    const ModuleData::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };
}
 
namespace SoundTracker
{
  std::auto_ptr<Formats::Chiptune::SoundTracker::Builder> CreateDataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props)
  {
    return std::auto_ptr<Formats::Chiptune::SoundTracker::Builder>(new DataBuilder(data, props));
  }

  ZXTune::Module::AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ZXTune::Module::ModuleProperties::Ptr properties)
  {
    return boost::make_shared<SoundTracker::Chiptune>(data, properties);
  }
}
