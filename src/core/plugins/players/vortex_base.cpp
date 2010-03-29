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
  
  const uint_t LIMITER(~uint_t(0));

  typedef boost::array<uint8_t, 256> VolumeTable;
  
  //Volume table of Pro Tracker 3.3x - 3.4x
  static const VolumeTable Vol33_34 =
  { {
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
  static const VolumeTable Vol35 =
  { {
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
      uint_t Period;
      int_t Value;
      uint_t Counter;
      int_t Delta;

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
      uint_t Note;
      uint_t SampleNum;
      uint_t PosInSample;
      uint_t OrnamentNum;
      uint_t PosInOrnament;
      uint_t Volume;
      int_t VolSlide;
      Slider ToneSlider;
      uint_t SlidingTargetNote;
      int_t ToneAccumulator;
      int_t EnvSliding;
      int_t NoiseSliding;
      uint_t VibrateCounter;
      uint_t VibrateOn;
      uint_t VibrateOff;
    };
    struct CommonState
    {
      CommonState()
        : EnvBase()
        , NoiseBase()
        , NoiseAddon()
      {
      }
      
      uint_t EnvBase;
      Slider EnvSlider;
      uint_t NoiseBase;
      int_t NoiseAddon;
    };
  public:
    typedef boost::shared_ptr<VortexPlayer> Ptr;

    VortexPlayer(Holder::ConstPtr holder, const Vortex::Track::ModuleData& data,
       uint_t version, const String& freqTableName, AYM::Chip::Ptr device)
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
      while (Vortex::Track::UpdateState(Data, ModState, Sound::LOOP_NONE));
      Reset();
#endif
    }
    
    virtual const Holder& GetModule() const
    {
      return *Module;
    }

    virtual Error GetPlaybackState(uint_t& timeState,
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
      if (Vortex::Track::UpdateState(Data, ModState, params.Looping))
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
      Vortex::Track::InitState(Data, ModState);
      std::fill(ChanState.begin(), ChanState.end(), ChannelState());
      CommState = CommonState();
      CurrentState = MODULE_STOPPED;
      return Error();
    }

    virtual Error SetPosition(uint_t frame)
    {
      if (frame < ModState.Frame)
      {
        //reset to beginning in case of moving back
        const uint64_t keepTicks = ModState.Tick;
        Vortex::Track::InitState(Data, ModState);
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
        if (!Vortex::Track::UpdateState(Data, ModState, Sound::LOOP_NONE))
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
      const Vortex::Track::Line& line = Data.Patterns[ModState.Track.Pattern][ModState.Track.Line];
      if (0 == ModState.Track.Frame)//begin note
      {
        if (0 == ModState.Track.Line)//pattern begin
        {
          CommState.NoiseBase = 0;
        }
        for (uint_t chan = 0; chan != line.Channels.size(); ++chan)
        {
          const Vortex::Track::Line::Chan& src = line.Channels[chan];
          ChannelState& dst = ChanState[chan];
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
          for (Vortex::Track::CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
          {
            switch (it->Type)
            {
            case Vortex::GLISS:
              dst.ToneSlider.Period = dst.ToneSlider.Counter = it->Param1;
              dst.ToneSlider.Delta = it->Param2;
              dst.SlidingTargetNote = LIMITER;
              dst.VibrateCounter = 0;
              if (0 == dst.ToneSlider.Counter && Version >= 7)
              {
                ++dst.ToneSlider.Counter;
              }
              break;
            case Vortex::GLISS_NOTE:
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
            case Vortex::SAMPLEOFFSET:
              dst.PosInSample = it->Param1;
              break;
            case Vortex::ORNAMENTOFFSET:
              dst.PosInOrnament = it->Param1;
              break;
            case Vortex::VIBRATE:
              dst.VibrateCounter = dst.VibrateOn = it->Param1;
              dst.VibrateOff = it->Param2;
              dst.ToneSlider.Value = 0;
              dst.ToneSlider.Counter = 0;
              break;
            case Vortex::SLIDEENV:
              CommState.EnvSlider.Period = CommState.EnvSlider.Counter = it->Param1;
              CommState.EnvSlider.Delta = it->Param2;
              break;
            case Vortex::ENVELOPE:
              chunk.Data[AYM::DataChunk::REG_ENV] = static_cast<uint8_t>(it->Param1);
              CommState.EnvBase = it->Param2;
              chunk.Mask |= (1 << AYM::DataChunk::REG_ENV);
              dst.Envelope = true;
              CommState.EnvSlider.Reset();
              dst.PosInOrnament = 0;
              break;
            case Vortex::NOENVELOPE:
              dst.Envelope = false;
              dst.PosInOrnament = 0;
              break;
            case Vortex::NOISEBASE:
              CommState.NoiseBase = it->Param1;
              break;
            case Vortex::TEMPO:
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
      int_t envelopeAddon = 0;
      for (uint_t chan = 0; chan < AYM::CHANNELS; ++chan)
      {
        ApplyData(chan, chunk, envelopeAddon);
      }
      const int_t envPeriod = envelopeAddon + CommState.EnvSlider.Value + int_t(CommState.EnvBase);
      chunk.Data[AYM::DataChunk::REG_TONEN] = static_cast<uint8_t>(CommState.NoiseBase + CommState.NoiseAddon) & 0x1f;
      chunk.Data[AYM::DataChunk::REG_TONEE_L] = static_cast<uint8_t>(envPeriod & 0xff);
      chunk.Data[AYM::DataChunk::REG_TONEE_H] = static_cast<uint8_t>(envPeriod / 256);
      chunk.Mask |= (1 << AYM::DataChunk::REG_TONEN) |
        (1 << AYM::DataChunk::REG_TONEE_L) | (1 << AYM::DataChunk::REG_TONEE_H);
      CommState.EnvSlider.Update();
      //count actually enabled channels
      ModState.Track.Channels = std::count_if(ChanState.begin(), ChanState.end(),
        boost::mem_fn(&ChannelState::Enabled));
    }
    
    void ApplyData(uint_t chan, AYM::DataChunk& chunk, int_t& envelopeAddon)
    {
      ChannelState& dst = ChanState[chan];
      const uint_t toneReg = AYM::DataChunk::REG_TONEA_L + 2 * chan;
      const uint_t volReg = AYM::DataChunk::REG_VOLA + chan;
      const uint_t toneMsk = AYM::DataChunk::REG_MASK_TONEA << chan;
      const uint_t noiseMsk = AYM::DataChunk::REG_MASK_NOISEA << chan;

      const FrequencyTable& freqTable = AYMHelper->GetFreqTable();
      if (dst.Enabled)
      {
        const Vortex::Track::Sample& curSample = Data.Samples[dst.SampleNum];
        const Vortex::Track::Sample::Line& curSampleLine = curSample.Data[dst.PosInSample];
        const Vortex::Track::Ornament& curOrnament = Data.Ornaments[dst.OrnamentNum];

        assert(!curOrnament.Data.empty());
        //calculate tone
        const int_t toneAddon = curSampleLine.ToneOffset + dst.ToneAccumulator;
        if (curSampleLine.KeepToneOffset)
        {
          dst.ToneAccumulator = toneAddon;
        }
        const uint_t halfTone = static_cast<uint_t>(clamp<int_t>(int_t(dst.Note) + curOrnament.Data[dst.PosInOrnament], 0, 95));
        const uint_t tone = static_cast<uint_t>(freqTable[halfTone] + dst.ToneSlider.Value + toneAddon) & 0xfff;
        if (dst.ToneSlider.Update() &&
            LIMITER != dst.SlidingTargetNote)
        {
          const uint_t targetTone = freqTable[dst.SlidingTargetNote];
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
        chunk.Data[toneReg] = static_cast<uint8_t>(tone & 0xff);
        chunk.Data[toneReg + 1] = static_cast<uint8_t>(tone >> 8);
        chunk.Mask |= 3 << toneReg;
        dst.VolSlide = clamp<int_t>(dst.VolSlide + curSampleLine.VolumeSlideAddon, -15, 15);
        //calculate level
        chunk.Data[volReg] = static_cast<uint8_t>(GetVolume(dst.Volume, clamp<int_t>(dst.VolSlide + curSampleLine.Level, 0, 15))
          | uint8_t(dst.Envelope && !curSampleLine.EnvMask ? AYM::DataChunk::REG_MASK_ENV : 0));
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
    
    uint_t GetVolume(uint_t volume, uint_t level)
    {
      return VolTable[volume * 16 + level];
    }
  private:
    const Holder::ConstPtr Module;
    const Vortex::Track::ModuleData& Data;
    const uint_t Version;
    const VolumeTable& VolTable;

    AYM::ParametersHelper::Ptr AYMHelper;
    AYM::Chip::Ptr Device;
    
    PlaybackState CurrentState;
    Vortex::Track::ModuleState ModState;
    boost::array<ChannelState, AYM::CHANNELS> ChanState;
    CommonState CommState;

    friend class VortexTSPlayer;
  };

  //TurboSound implementation

  //TODO: extract TS-related code
  template<class T>
  inline T avg(T val1, T val2)
  {
    return (val1 + val2) / 2;
  }

  template<uint_t Channels>
  class TSMixer : public Sound::MultichannelReceiver
  {
  public:
    TSMixer() : Buffer(), Cursor(Buffer.end()), SampleBuf(Channels), Receiver(0)
    {
    }

    virtual void ApplySample(const std::vector<Sound::Sample>& data)
    {
      assert(data.size() == Channels);

      if (Receiver) //mix and out
      {
        std::transform(data.begin(), data.end(), Cursor->begin(), SampleBuf.begin(), avg<Sound::Sample>);
        Receiver->ApplySample(SampleBuf);
      }
      else //store
      {
        std::memcpy(Cursor->begin(), &data[0], std::min(Channels, data.size()) * sizeof(Sound::Sample));
      }
      ++Cursor;
    }

    virtual void Flush()
    {
    }

    void Reset(const Sound::RenderParameters& params)
    {
      //assert(Cursor == Buffer.end());
      Buffer.resize(params.SamplesPerFrame());
      Cursor = Buffer.begin();
      Receiver = 0;
    }

    void Switch(Sound::MultichannelReceiver& receiver)
    {
      //assert(Cursor == Buffer.end());
      Receiver = &receiver;
      Cursor = Buffer.begin();
    }
  private:
    typedef boost::array<Sound::Sample, Channels> InternalSample;
    std::vector<InternalSample> Buffer;
    typename std::vector<InternalSample>::iterator Cursor;
    std::vector<Sound::Sample> SampleBuf;
    Sound::MultichannelReceiver* Receiver;
  };

  class VortexTSPlayer : public Player
  {
  public:
    VortexTSPlayer(Holder::ConstPtr holder, const Vortex::Track::ModuleData& data,
         uint_t version, const String& freqTableName, uint_t patternBase, AYM::Chip::Ptr device1, AYM::Chip::Ptr device2)
      : Module(holder)
      , Player2(new VortexPlayer(holder, data, version, freqTableName, device2))
      //copy and patch
      , Data(data)
    {
      std::transform(Data.Positions.begin(), Data.Positions.end(), Data.Positions.begin(),
        std::bind1st(std::minus<uint_t>(), patternBase - 1));
      Player1.reset(new VortexPlayer(holder, Data, version, freqTableName, device1));
    }

    virtual const Holder& GetModule() const
    {
      return *Module;
    }

    virtual Error GetPlaybackState(uint_t& timeState,
                                   Tracking& trackState,
                                   Analyze::ChannelsState& analyzeState) const
    {
      Analyze::ChannelsState firstAnalyze;
      if (const Error& err = Player1->GetPlaybackState(timeState, trackState, firstAnalyze))
      {
        return err;
      }
      Analyze::ChannelsState secondAnalyze;
      uint_t dummyTime = 0;
      Tracking dummyTracking;
      if (const Error& err = Player2->GetPlaybackState(dummyTime, dummyTracking, secondAnalyze))
      {
        return err;
      }
      assert(timeState == dummyTime);
      //merge
      analyzeState.resize(firstAnalyze.size() + secondAnalyze.size());
      std::copy(secondAnalyze.begin(), secondAnalyze.end(),
        std::copy(firstAnalyze.begin(), firstAnalyze.end(), analyzeState.begin()));
      trackState.Channels += dummyTracking.Channels;
      return Error();
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params,
                              PlaybackState& state,
                              Sound::MultichannelReceiver& receiver)
    {
      const uint_t tempo1 = Player1->ModState.Track.Tempo;
      PlaybackState state1;
      Mixer.Reset(params);
      if (const Error& e = Player1->RenderFrame(params, state1, Mixer))
      {
        return e;
      }
      const uint_t tempo2 = Player2->ModState.Track.Tempo;
      PlaybackState state2;
      Mixer.Switch(receiver);
      if (const Error& e = Player2->RenderFrame(params, state2, Mixer))
      {
        return e;
      }
      state = state1 == MODULE_STOPPED || state2 == MODULE_STOPPED ? MODULE_STOPPED : MODULE_PLAYING;
      //synchronize tempo
      if (tempo1 != Player1->ModState.Track.Tempo)
      {
        const uint_t pattern = Player2->ModState.Track.Pattern;
        Player2->ModState = Player1->ModState;
        Player2->ModState.Track.Pattern = pattern;
      }
      else if (tempo2 != Player2->ModState.Track.Tempo)
      {
        const uint_t pattern = Player1->ModState.Track.Pattern;
        Player1->ModState = Player2->ModState;
        Player1->ModState.Track.Pattern = pattern;
      }
      return Error();
    }

    virtual Error Reset()
    {
      if (const Error& e = Player1->Reset())
      {
        return e;
      }
      if (const Error& e = Player2->Reset())
      {
        return e;
      }
      const uint_t pattern = Player2->ModState.Track.Pattern;
      Player2->ModState = Player1->ModState;
      Player2->ModState.Track.Pattern = pattern;
      return Error();
    }

    virtual Error SetPosition(uint_t frame)
    {
      if (const Error& e = Player1->SetPosition(frame))
      {
        return e;
      }
      return Player2->SetPosition(frame);
    }

    virtual Error SetParameters(const Parameters::Map& params)
    {
      if (const Error& e = Player1->SetParameters(params))
      {
        return e;
      }
      return Player2->SetParameters(params);
    }
  private:
    const Holder::ConstPtr Module;
    //first player
    VortexPlayer::Ptr Player1;
    //second player and data
    Vortex::Track::ModuleData Data;
    VortexPlayer::Ptr Player2;
    //mixer
    TSMixer<AYM::CHANNELS> Mixer;
  };
}

namespace ZXTune
{
  namespace Module
  {
    namespace Vortex
    {
      Player::Ptr CreatePlayer(Holder::ConstPtr holder, const Track::ModuleData& data,
         uint_t version, const String& freqTableName, AYM::Chip::Ptr device)
      {
        return Player::Ptr(new VortexPlayer(holder, data, version, freqTableName, device));
      }

      Player::Ptr CreateTSPlayer(Holder::ConstPtr holder, const Track::ModuleData& data,
         uint_t version, const String& freqTableName, uint_t patternBase, AYM::Chip::Ptr device1, AYM::Chip::Ptr device2)
      {
        return Player::Ptr(new VortexTSPlayer(holder, data, version, freqTableName, patternBase, device1, device2));
      }
    }
  }
}
