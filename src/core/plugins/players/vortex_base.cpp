/*
Abstract:
  Vortex-based modules playback support implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "aym_base.h"
#include "vortex_base.h"
//common includes
#include <error_tools.h>
#include <tools.h>

#define FILE_TAG 023C2245

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;
  
  const uint_t LIMITER = ~uint_t(0);

  typedef boost::array<uint8_t, 256> VolumeTable;
  
  //Volume table of Pro Tracker 3.3x - 3.4x
  const VolumeTable Vol33_34 =
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
  const VolumeTable Vol35 =
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

  //helper for sliding processing
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

  //internal per-channel state type
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

  //internal common state type
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

  struct VortexState
  {
    boost::array<ChannelState, AYM::CHANNELS> ChanState;
    CommonState CommState;
  };

  typedef AYMPlayer<Vortex::Track, VortexState> VortexPlayerBase;

  //simple player type
  class VortexPlayer : public VortexPlayerBase
  {
  public:
    typedef boost::shared_ptr<VortexPlayer> Ptr;

    VortexPlayer(Holder::ConstPtr holder, const Vortex::Track::ModuleData& data,
       uint_t version, const String& freqTableName, AYM::Chip::Ptr device)
      : VortexPlayerBase(holder, data, device, freqTableName)
      , Version(version)
      , VolTable(version <= 4 ? Vol33_34 : Vol35)
    {
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
    
    virtual void RenderData(AYM::DataChunk& chunk)
    {
      const Vortex::Track::Line& line = Data.Patterns[ModState.Track.Pattern][ModState.Track.Line];
      if (0 == ModState.Track.Frame)//begin note
      {
        if (0 == ModState.Track.Line)//pattern begin
        {
          PlayerState.CommState.NoiseBase = 0;
        }
        for (uint_t chan = 0; chan != line.Channels.size(); ++chan)
        {
          const Vortex::Track::Line::Chan& src = line.Channels[chan];
          ChannelState& dst = PlayerState.ChanState[chan];
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
              PlayerState.CommState.EnvSlider.Period = PlayerState.CommState.EnvSlider.Counter = it->Param1;
              PlayerState.CommState.EnvSlider.Delta = it->Param2;
              break;
            case Vortex::ENVELOPE:
              chunk.Data[AYM::DataChunk::REG_ENV] = static_cast<uint8_t>(it->Param1);
              PlayerState.CommState.EnvBase = it->Param2;
              chunk.Mask |= (1 << AYM::DataChunk::REG_ENV);
              dst.Envelope = true;
              PlayerState.CommState.EnvSlider.Reset();
              dst.PosInOrnament = 0;
              break;
            case Vortex::NOENVELOPE:
              dst.Envelope = false;
              dst.PosInOrnament = 0;
              break;
            case Vortex::NOISEBASE:
              PlayerState.CommState.NoiseBase = it->Param1;
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
      const int_t envPeriod = envelopeAddon + PlayerState.CommState.EnvSlider.Value + int_t(PlayerState.CommState.EnvBase);
      chunk.Data[AYM::DataChunk::REG_TONEN] = static_cast<uint8_t>(PlayerState.CommState.NoiseBase + PlayerState.CommState.NoiseAddon) & 0x1f;
      chunk.Data[AYM::DataChunk::REG_TONEE_L] = static_cast<uint8_t>(envPeriod & 0xff);
      chunk.Data[AYM::DataChunk::REG_TONEE_H] = static_cast<uint8_t>(envPeriod / 256);
      chunk.Mask |= (1 << AYM::DataChunk::REG_TONEN) |
        (1 << AYM::DataChunk::REG_TONEE_L) | (1 << AYM::DataChunk::REG_TONEE_H);
      PlayerState.CommState.EnvSlider.Update();
      //count actually enabled channels
      ModState.Track.Channels = std::count_if(PlayerState.ChanState.begin(), PlayerState.ChanState.end(),
        boost::mem_fn(&ChannelState::Enabled));
    }
    
    void ApplyData(uint_t chan, AYM::DataChunk& chunk, int_t& envelopeAddon)
    {
      ChannelState& dst = PlayerState.ChanState[chan];
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
          PlayerState.CommState.NoiseAddon = curSampleLine.NoiseOrEnvelopeOffset + dst.NoiseSliding;
          if (curSampleLine.KeepNoiseOrEnvelopeOffset)
          {
            dst.NoiseSliding = PlayerState.CommState.NoiseAddon;
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
    const uint_t Version;
    const VolumeTable& VolTable;

    friend class VortexTSPlayer;
  };

  //TurboSound implementation

  //TODO: extract TS-related code also from ts_supp.cpp
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
        std::memcpy(Cursor->begin(), &data[0], std::min<uint_t>(Channels, data.size()) * sizeof(Sound::Sample));
      }
      ++Cursor;
    }

    virtual void Flush()
    {
    }

    void Reset(const Sound::RenderParameters& params)
    {
      Buffer.resize(params.SamplesPerFrame());
      Cursor = Buffer.begin();
      Receiver = 0;
    }

    void Switch(Sound::MultichannelReceiver& receiver)
    {
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

  //TODO: fix wrongly calculated total time
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
    VortexPlayer::Ptr Player2;
    Vortex::Track::ModuleData Data;
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
