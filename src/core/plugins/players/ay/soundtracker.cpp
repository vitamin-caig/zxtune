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
#include "ay_conversion.h"
//boost includes
#include <boost/make_shared.hpp>

namespace SoundTracker
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

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

  struct ChannelState
  {
    explicit ChannelState(ModuleData::Ptr data)
      : Data(data)
      , Enabled(false), Envelope(false)
      , Note()
      , CurSample(GetStubSample()), PosInSample(0), LoopedInSample(false)
      , CurOrnament(GetStubOrnament())
    {
    }

    void Reset()
    {
      Enabled = false;
      Envelope = false;
      Note = 0;
      CurSample = GetStubSample();
      PosInSample = 0;
      LoopedInSample = false;
      CurOrnament = GetStubOrnament();
    }

    void SetNewState(const Track::Line::Chan& src)
    {
      if (src.Enabled)
      {
        SetEnabled(*src.Enabled);
      }
      if (src.Note)
      {
        SetNote(*src.Note);
      }
      if (src.SampleNum)
      {
        SetSample(*src.SampleNum);
      }
      if (src.OrnamentNum)
      {
        SetOrnament(*src.OrnamentNum);
      }
      if (!src.Commands.empty())
      {
        ApplyCommands(src.Commands);
      }
    }

    void Synthesize(ChannelBuilder& channel) const
    {
      if (!Enabled)
      {
        channel.SetLevel(0);
        return;
      }

      const Track::Sample::Line& curSampleLine = CurSample->GetLine(PosInSample);

      //apply level
      channel.SetLevel(curSampleLine.Level);
      //apply envelope
      if (Envelope)
      {
        channel.EnableEnvelope();
      }
      //apply tone
      const int_t halftones = int_t(Note) + CurOrnament->GetLine(PosInSample);
      if (!curSampleLine.EnvelopeMask)
      {
        channel.SetTone(halftones, curSampleLine.Effect);
      }
      else
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
    }

    void Iterate()
    {
      if (++PosInSample >= (LoopedInSample ? CurSample->GetLoopLimit() : CurSample->GetSize()))
      {
        if (CurSample->GetLoop() && CurSample->GetLoop() < CurSample->GetSize())
        {
          PosInSample = CurSample->GetLoop();
          LoopedInSample = true;
        }
        else
        {
          Enabled = false;
        }
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

    void SetEnabled(bool enabled)
    {
      Enabled = enabled;
      if (!Enabled)
      {
        PosInSample = 0;
      }
    }

    void SetNote(uint_t note)
    {
      Note = note;
      PosInSample = 0;
      LoopedInSample = false;
    }

    void SetSample(uint_t sampleNum)
    {
      CurSample = sampleNum < Data->Samples.size() ? &Data->Samples[sampleNum] : GetStubSample();
      PosInSample = 0;
    }

    void SetOrnament(uint_t ornamentNum)
    {
      CurOrnament = ornamentNum < Data->Ornaments.size() ? &Data->Ornaments[ornamentNum] : GetStubOrnament();
      PosInSample = 0;
    }

    void ApplyCommands(const Track::CommandsArray& commands)
    {
      std::for_each(commands.begin(), commands.end(), boost::bind(&ChannelState::ApplyCommand, this, _1));
    }

    void ApplyCommand(const Track::Command& command)
    {
      if (command == ENVELOPE)
      {
        Envelope = true;
      }
      else if (command == NOENVELOPE)
      {
        Envelope = false;
      }
    }
  private:
    const ModuleData::Ptr Data;
    bool Enabled;
    bool Envelope;
    uint_t Note;
    const Track::Sample* CurSample;
    uint_t PosInSample;
    bool LoopedInSample;
    const Track::Ornament* CurOrnament;
  };

  void ProcessEnvelopeCommands(const Track::CommandsArray& commands, AYM::TrackBuilder& track)
  {
    const Track::CommandsArray::const_iterator it = std::find(commands.begin(), commands.end(), ENVELOPE);
    if (it != commands.end())
    {
      track.SetEnvelopeType(it->Param1);
      track.SetEnvelopeTone(it->Param2);
    }
  }

  class DataIterator : public AYM::DataIterator
  {
  public:
    DataIterator(AYM::TrackParameters::Ptr trackParams, StateIterator::Ptr delegate, ModuleData::Ptr data)
      : TrackParams(trackParams)
      , Delegate(delegate)
      , State(Delegate->GetStateObserver())
      , Data(data)
      , StateA(Data)
      , StateB(Data)
      , StateC(Data)
    {
      SwitchToNewLine();
    }

    virtual void Reset()
    {
      Delegate->Reset();
      StateA.Reset();
      StateB.Reset();
      StateC.Reset();
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

        if (0 == State->Quirk())
        {
          GetNewLineState(track);
        }
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
      if (const Track::Line* line = Data->Patterns[State->Pattern()].GetLine(State->Line()))
      {
        if (const Track::Line::Chan& src = line->Channels[0])
        {
          StateA.SetNewState(src);
        }
        if (const Track::Line::Chan& src = line->Channels[1])
        {
          StateB.SetNewState(src);
        }
        if (const Track::Line::Chan& src = line->Channels[2])
        {
          StateC.SetNewState(src);
        }
      }
    }

    void GetNewLineState(AYM::TrackBuilder& track) const
    {
      if (const Track::Line* line = Data->Patterns[State->Pattern()].GetLine(State->Line()))
      {
        if (const Track::Line::Chan& src = line->Channels[0])
        {
          ProcessEnvelopeCommands(src.Commands, track);
        }
        if (const Track::Line::Chan& src = line->Channels[1])
        {
          ProcessEnvelopeCommands(src.Commands, track);
        }
        if (const Track::Line::Chan& src = line->Channels[2])
        {
          ProcessEnvelopeCommands(src.Commands, track);
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
    }
  private:
    const AYM::TrackParameters::Ptr TrackParams;
    const StateIterator::Ptr Delegate;
    const TrackState::Ptr State;
    const ModuleData::Ptr Data;
    ChannelState StateA;
    ChannelState StateB;
    ChannelState StateC;
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
      const StateIterator::Ptr iter = CreateTrackStateIterator(Info, Data);
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
  ZXTune::Module::AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ZXTune::Module::ModuleProperties::Ptr properties)
  {
    return boost::make_shared<SoundTracker::Chiptune>(data, properties);
  }
}
