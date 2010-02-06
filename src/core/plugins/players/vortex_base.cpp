/*
Abstract:
  Vortex-based modules playback support implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "vortex_base.h"

#include <error_tools.h>
#include <tools.h>
#include <core/error_codes.h>
#include <core/devices/aym_parameters_helper.h>

#include <text/core.h>

#define FILE_TAG 023C2245

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;
  
  const unsigned LIMITER(~0u);

  typedef boost::array<uint8_t, 256> VolumeTable;
  
  //Volume table of Pro Tracker 3.3x - 3.4x
  static const VolumeTable Vol33_34 = { {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09,
    0x00, 0x00, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0a,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x09, 0x0a, 0x0b,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
  } };

  //Volume table of Pro Tracker 3.5x
  static const VolumeTable Vol35 = { {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07,
    0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08,
    0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x08, 0x09,
    0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x09, 0x0a,
    0x00, 0x01, 0x01, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b,
    0x00, 0x01, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b, 0x0c,
    0x00, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b, 0x0c, 0x0d,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
  } };

  // perform module 'playback' right after creating (debug purposes)
  #ifndef NDEBUG
  #define SELF_TEST
  #endif

  class VortexPlayer : public Player
  {
    struct Slider
    {
      Slider() : Period(), Value(), Counter(), Delta()
      {
      }
      unsigned Period;
      signed Value;
      unsigned Counter;
      signed Delta;

      bool Update()
      {
        if (Counter && !--Counter)
        {
          Value += Delta;
          Counter = Period;
          return true;
        }
        return false;
      }

      void Reset()
      {
        Counter = 0;
        Value = 0;
      }
    };
    struct ChannelState
    {
      ChannelState()
        : Enabled(false), Envelope(false)
        , Note()
        , SampleNum(0), PosInSample(0)
        , OrnamentNum(0), PosInOrnament(0)
        , Volume(15), VolSlide(0)
        , ToneSlider(), SlidingTargetNote(LIMITER), ToneAccumulator(0)
        , EnvSliding(), NoiseSliding()
        , VibrateCounter(0), VibrateOn(), VibrateOff()
      {
      }
      
      bool Enabled;
      bool Envelope;
      unsigned Note;
      unsigned SampleNum;
      unsigned PosInSample;
      unsigned OrnamentNum;
      unsigned PosInOrnament;
      unsigned Volume;
      signed VolSlide;
      Slider ToneSlider;
      unsigned SlidingTargetNote;
      signed ToneAccumulator;
      signed EnvSliding;
      signed NoiseSliding;
      unsigned VibrateCounter;
      unsigned VibrateOn;
      unsigned VibrateOff;
    };
    struct CommonState
    {
      CommonState()
        : EnvBase()
        , NoiseBase()
        , NoiseAddon()
      {
      }
      
      unsigned EnvBase;
      Slider EnvSlider;
      unsigned NoiseBase;
      signed NoiseAddon;
    };
  public:
    VortexPlayer(Holder::ConstPtr holder, const VortexTrack::ModuleData& data, 
       unsigned version, const String& freqTableName, AYM::Chip::Ptr device)
      : Module(holder)       
      , Data(data)
      , Version(version)
      , VolTable(version <= 4 ? Vol33_34 : Vol35)
      , AYMHelper(AYM::ParametersHelper::Create(freqTableName))
      , Device(device)
      , CurrentState(MODULE_STOPPED)
    {
      Reset();
#ifdef SELF_TEST
//perform self-test
      AYM::DataChunk chunk;
      do
      {
        assert(Data.Positions.size() > ModState.Track.Position);
        RenderData(chunk);
      }
      while (VortexTrack::UpdateState(Data, ModState, Sound::LOOP_NONE));
      Reset();
#endif
    }
    
    virtual const Holder& GetModule() const
    {
      return *Module;
    }

    virtual Error GetPlaybackState(unsigned& timeState,
                                   Tracking& trackState,
                                   Analyze::ChannelsState& analyzeState) const
    {
      timeState = ModState.Frame;
      trackState = ModState.Track;
      Device->GetState(analyzeState);
      return Error();
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params,
                              PlaybackState& state,
                              Sound::MultichannelReceiver& receiver)
    {
      if (ModState.Frame >= Data.Info.Statistic.Frame)
      {
        if (MODULE_STOPPED == CurrentState)
        {
          return Error(THIS_LINE, ERROR_MODULE_END, TEXT_MODULE_ERROR_MODULE_END);
        }
        receiver.Flush();
        state = CurrentState = MODULE_STOPPED;
        return Error();
      }

      AYM::DataChunk chunk;
      AYMHelper->GetDataChunk(chunk);
      ModState.Tick += params.ClocksPerFrame();
      chunk.Tick = ModState.Tick;
      RenderData(chunk);

      Device->RenderData(params, chunk, receiver);
      if (VortexTrack::UpdateState(Data, ModState, params.Looping))
      {
        CurrentState = MODULE_PLAYING;
      }
      else
      {
        receiver.Flush();
        CurrentState = MODULE_STOPPED;
      }
      state = CurrentState;
      return Error();
    }

    virtual Error Reset()
    {
      Device->Reset();
      VortexTrack::InitState(Data, ModState);
      std::fill(ChanState.begin(), ChanState.end(), ChannelState());
      CommState = CommonState();
      CurrentState = MODULE_STOPPED;
      return Error();
    }

    virtual Error SetPosition(unsigned frame)
    {
      if (frame < ModState.Frame)
      {
        //reset to beginning in case of moving back
        const uint64_t keepTicks = ModState.Tick;
        VortexTrack::InitState(Data, ModState);
        std::fill(ChanState.begin(), ChanState.end(), ChannelState());
        CommState = CommonState();
        ModState.Tick = keepTicks;
      }
      //fast forward
      AYM::DataChunk chunk;
      while (ModState.Frame < frame)
      {
        //do not update tick for proper rendering
        assert(Data.Positions.size() > ModState.Track.Position);
        RenderData(chunk);
        if (!VortexTrack::UpdateState(Data, ModState, Sound::LOOP_NONE))
        {
          break;
        }
      }
      return Error();
    }

    virtual Error SetParameters(const Parameters::Map& params)
    {
      try
      {
        AYMHelper->SetParameters(params);
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, ERROR_INVALID_PARAMETERS, TEXT_MODULE_ERROR_SET_PLAYER_PARAMETERS).AddSuberror(e);
      }
    }
  private:
    void RenderData(AYM::DataChunk& chunk)
    {
      const VortexTrack::Line& line(Data.Patterns[ModState.Track.Pattern][ModState.Track.Line]);
      if (0 == ModState.Track.Frame)//begin note
      {
        if (0 == ModState.Track.Line)//pattern begin
        {
          CommState.NoiseBase = 0;
        }
        for (unsigned chan = 0; chan != line.Channels.size(); ++chan)
        {
          const VortexTrack::Line::Chan& src(line.Channels[chan]);
          ChannelState& dst(ChanState[chan]);
          if (src.Enabled)
          {
            dst.PosInSample = dst.PosInOrnament = 0;
            dst.VolSlide = dst.EnvSliding = dst.NoiseSliding = 0;
            dst.ToneSlider.Reset();
            dst.ToneAccumulator = 0;
            dst.VibrateCounter = 0;
            dst.Enabled = *src.Enabled;
          }
          if (src.Note)
          {
            dst.Note = *src.Note;
          }
          if (src.SampleNum)
          {
            dst.SampleNum = *src.SampleNum;
          }
          if (src.OrnamentNum)
          {
            dst.OrnamentNum = *src.OrnamentNum;
            dst.PosInOrnament = 0;
          }
          if (src.Volume)
          {
            dst.Volume = *src.Volume;
          }
          for (VortexTrack::CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
          {
            switch (it->Type)
            {
            case GLISS:
              dst.ToneSlider.Period = dst.ToneSlider.Counter = it->Param1;
              dst.ToneSlider.Delta = it->Param2;
              dst.SlidingTargetNote = LIMITER;
              dst.VibrateCounter = 0;
              if (0 == dst.ToneSlider.Counter && Version >= 7)
              {
                ++dst.ToneSlider.Counter;
              }
              break;
            case GLISS_NOTE:
              dst.ToneSlider.Period = dst.ToneSlider.Counter = it->Param1;
              dst.ToneSlider.Delta = it->Param2;
              dst.SlidingTargetNote = it->Param3;
              dst.VibrateCounter = 0;
              //tone up                                     freq up
              if (bool(dst.Note < dst.SlidingTargetNote) != bool(dst.ToneSlider.Delta < 0))
              {
                dst.ToneSlider.Delta = -dst.ToneSlider.Delta;
              }
              break;
            case SAMPLEOFFSET:
              dst.PosInSample = it->Param1;
              break;
            case ORNAMENTOFFSET:
              dst.PosInOrnament = it->Param1;
              break;
            case VIBRATE:
              dst.VibrateCounter = dst.VibrateOn = it->Param1;
              dst.VibrateOff = it->Param2;
              dst.ToneSlider.Value = 0;
              dst.ToneSlider.Counter = 0;
              break;
            case SLIDEENV:
              CommState.EnvSlider.Period = CommState.EnvSlider.Counter = it->Param1;
              CommState.EnvSlider.Delta = it->Param2;
              break;
            case ENVELOPE:
              chunk.Data[AYM::DataChunk::REG_ENV] = uint8_t(it->Param1);
              CommState.EnvBase = it->Param2;
              chunk.Mask |= (1 << AYM::DataChunk::REG_ENV);
              dst.Envelope = true;
              CommState.EnvSlider.Reset();
              dst.PosInOrnament = 0;
              break;
            case NOENVELOPE:
              dst.Envelope = false;
              dst.PosInOrnament = 0;
              break;
            case NOISEBASE:
              CommState.NoiseBase = it->Param1;
              break;
            case TEMPO:
              //ignore
              break;
            default:
              assert(!"Invalid command");
            }
          }
        }
      }
      //permanent registers
      chunk.Data[AYM::DataChunk::REG_MIXER] = 0;
      chunk.Mask |= (1 << AYM::DataChunk::REG_MIXER) |
        (1 << AYM::DataChunk::REG_VOLA) | (1 << AYM::DataChunk::REG_VOLB) | (1 << AYM::DataChunk::REG_VOLC);
      signed envelopeAddon(0);
      for (unsigned chan = 0; chan < AYM::CHANNELS; ++chan)
      {
        ApplyData(chan, chunk, envelopeAddon);
      }
      const signed envPeriod(envelopeAddon + CommState.EnvSlider.Value + signed(CommState.EnvBase));
      chunk.Data[AYM::DataChunk::REG_TONEN] = uint8_t(CommState.NoiseBase + CommState.NoiseAddon) & 0x1f;
      chunk.Data[AYM::DataChunk::REG_TONEE_L] = uint8_t(envPeriod & 0xff);
      chunk.Data[AYM::DataChunk::REG_TONEE_H] = uint8_t(envPeriod >> 8);
      chunk.Mask |= (1 << AYM::DataChunk::REG_TONEN) |
        (1 << AYM::DataChunk::REG_TONEE_L) | (1 << AYM::DataChunk::REG_TONEE_H);
      CommState.EnvSlider.Update();
      //count actually enabled channels
      ModState.Track.Channels = static_cast<unsigned>(std::count_if(ChanState.begin(), ChanState.end(),
        boost::mem_fn(&ChannelState::Enabled)));
    }
    
    void ApplyData(unsigned chan, AYM::DataChunk& chunk, signed& envelopeAddon)
    {
      ChannelState& dst(ChanState[chan]);
      const unsigned toneReg(AYM::DataChunk::REG_TONEA_L + 2 * chan);
      const unsigned volReg = AYM::DataChunk::REG_VOLA + chan;
      const unsigned toneMsk = AYM::DataChunk::REG_MASK_TONEA << chan;
      const unsigned noiseMsk = AYM::DataChunk::REG_MASK_NOISEA << chan;

      const FrequencyTable& freqTable(AYMHelper->GetFreqTable());
      if (dst.Enabled)
      {
        const VortexTrack::Sample& curSample(Data.Samples[dst.SampleNum]);
        const VortexTrack::Sample::Line& curSampleLine(curSample.Data[dst.PosInSample]);
        const VortexTrack::Ornament& curOrnament(Data.Ornaments[dst.OrnamentNum]);

        assert(!curOrnament.Data.empty());
        //calculate tone
        const signed toneAddon(curSampleLine.ToneOffset + dst.ToneAccumulator);
        if (curSampleLine.KeepToneOffset)
        {
          dst.ToneAccumulator = toneAddon;
        }
        const unsigned halfTone(static_cast<unsigned>(clamp(int(dst.Note) + curOrnament.Data[dst.PosInOrnament], 0, 95)));
        const uint16_t tone((freqTable[halfTone] + dst.ToneSlider.Value + toneAddon) & 0xfff);
        if (dst.ToneSlider.Update() && 
            LIMITER != dst.SlidingTargetNote)
        {
          const uint16_t targetTone(freqTable[dst.SlidingTargetNote]);
          if ((dst.ToneSlider.Delta > 0 && tone + dst.ToneSlider.Delta > targetTone) ||
            (dst.ToneSlider.Delta < 0 && tone + dst.ToneSlider.Delta < targetTone))
          {
            //slided to target note
            dst.Note = dst.SlidingTargetNote;
            dst.SlidingTargetNote = LIMITER;
            dst.ToneSlider.Value = 0;
            dst.ToneSlider.Counter = 0;
          }
        }
        chunk.Data[toneReg] = uint8_t(tone & 0xff);
        chunk.Data[toneReg + 1] = uint8_t(tone >> 8);
        chunk.Mask |= 3 << toneReg;
        dst.VolSlide = clamp(dst.VolSlide + curSampleLine.VolumeSlideAddon, -15, 15);
        //calculate level
        chunk.Data[volReg] = GetVolume(dst.Volume, clamp<signed>(dst.VolSlide + curSampleLine.Level, 0, 15))
          | uint8_t(dst.Envelope && !curSampleLine.EnvMask ? AYM::DataChunk::REG_MASK_ENV : 0);
        //mixer
        if (curSampleLine.ToneMask)
        {
          chunk.Data[AYM::DataChunk::REG_MIXER] |= toneMsk;
        }
        if (curSampleLine.NoiseMask)
        {
          chunk.Data[AYM::DataChunk::REG_MIXER] |= noiseMsk;
          if (curSampleLine.NoiseOrEnvelopeOffset)
          {
            dst.EnvSliding = curSampleLine.NoiseOrEnvelopeOffset;
          }
          envelopeAddon += curSampleLine.NoiseOrEnvelopeOffset;
        }
        else
        {
          CommState.NoiseAddon = curSampleLine.NoiseOrEnvelopeOffset + dst.NoiseSliding;
          if (curSampleLine.KeepNoiseOrEnvelopeOffset)
          {
            dst.NoiseSliding = CommState.NoiseAddon;
          }
        }

        if (++dst.PosInSample >= curSample.Data.size())
        {
          dst.PosInSample = curSample.Loop;
        }
        if (++dst.PosInOrnament >= curOrnament.Data.size())
        {
          dst.PosInOrnament = curOrnament.Loop;
        }
      }
      else
      {
        chunk.Data[volReg] = 0;
        //????
        chunk.Data[AYM::DataChunk::REG_MIXER] |= toneMsk | noiseMsk;
      }
      if (dst.VibrateCounter > 0 && !--dst.VibrateCounter)
      {
        dst.Enabled = !dst.Enabled;
        dst.VibrateCounter = dst.Enabled ? dst.VibrateOn : dst.VibrateOff;
      }
    }
    
    unsigned GetVolume(unsigned volume, unsigned level)
    {
      assert(volume <= 15);
      assert(level <= 15);
      return VolTable[volume * 16 + level];
    }
  private:
    const Holder::ConstPtr Module;
    const VortexTrack::ModuleData& Data;
    const unsigned Version;
    const VolumeTable& VolTable;

    AYM::ParametersHelper::Ptr AYMHelper;
    AYM::Chip::Ptr Device;
    
    PlaybackState CurrentState;
    VortexTrack::ModuleState ModState;
    boost::array<ChannelState, AYM::CHANNELS> ChanState;
    CommonState CommState;
  };
}

namespace ZXTune
{
  namespace Module
  {
    Player::Ptr CreateVortexPlayer(Holder::ConstPtr holder, const VortexTrack::ModuleData& data, 
       unsigned version, const String& freqTableName, AYM::Chip::Ptr device)
    {
      return Player::Ptr(new VortexPlayer(holder, data, version, freqTableName, device));
    }
    
    /*
    void VortexPlayer::Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      if (const VortexTextParam* const p = parameter_cast<VortexTextParam>(&param))
      {
        VortexDescr descr;
        const String::value_type DELIMITER[] = {'\n', 0};
        StringMap::const_iterator it(Information.Properties.find(Module::ATTR_TITLE));
        if (Information.Properties.end() != it)
        {
          descr.Title = it->second;
        }
        it = Information.Properties.find(Module::ATTR_AUTHOR);
        if (Information.Properties.end() != it)
        {
          descr.Author = it->second;
        }
        descr.Version = 30 + Version;
        descr.Notetable = Notetable;
        descr.Tempo = Information.Statistic.Tempo;
        descr.Loop = Information.Loop;
        descr.Order = Data.Positions;
        StringArray asArray;
        std::back_insert_iterator<StringArray> iter(asArray);
        *iter = DescriptionHeaderToString();
        iter = DescriptionToStrings(descr, ++iter);
        *iter = String();//free
        for (std::size_t idx = 1; idx != Data.Ornaments.size(); ++idx)
        {
          if (Data.Ornaments[idx].Data.size())
          {
            *++iter = OrnamentHeaderToString(idx);
            *++iter = OrnamentToString(Data.Ornaments[idx]);
            *++iter = String();//free
          }
        }
        for (std::size_t idx = 1; idx != Data.Samples.size(); ++idx)
        {
          if (Data.Samples[idx].Data.size())
          {
            *++iter = SampleHeaderToString(idx);
            iter = SampleToStrings(Data.Samples[idx], ++iter);
            *++iter = String();
          }
        }
        for (std::size_t idx = 0; idx != Data.Patterns.size(); ++idx)
        {
          if (Data.Patterns[idx].size())
          {
            *++iter = PatternHeaderToString(idx);
            iter = PatternToStrings(Data.Patterns[idx], ++iter);
            *++iter = String();
          }
        }
        const String& result(boost::algorithm::join(asArray, DELIMITER));
        dst.resize(result.size() * sizeof(String::value_type));
        std::memcpy(&dst[0], &result[0], dst.size());
      }
      else
      {
        Parent::Convert(param, dst);
      }
    }
    */
  }
}
