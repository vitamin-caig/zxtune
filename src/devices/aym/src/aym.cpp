/*
Abstract:
  AY/YM chips interface implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  Based on sources of UnrealSpeccy by SMT and Xpeccy sources by SamStyle
*/

//local includes
#include "device.h"
//common includes
#include <tools.h>
//library includes
#include <sound/lpfilter.h>
#include <time/oscillator.h>
//std includes
#include <cassert>
#include <functional>
#include <memory>
#include <numeric>
//boost includes
#include <boost/scoped_ptr.hpp>

namespace
{
  using namespace Devices::AYM;

  enum
  {
    // set of registers which required input data masking (4 or 5 lsb)
    REGS_4BIT_SET = (1 << DataChunk::REG_TONEA_H) | (1 << DataChunk::REG_TONEB_H) |
                    (1 << DataChunk::REG_TONEC_H) | (1 << DataChunk::REG_ENV) | (1 << DataChunk::REG_BEEPER),
    REGS_5BIT_SET = (1 << DataChunk::REG_TONEN) | (1 << DataChunk::REG_VOLA) |
                    (1 << DataChunk::REG_VOLB) | (1 << DataChunk::REG_VOLC),
  };

  BOOST_STATIC_ASSERT(DataChunk::REG_LAST < 8 * sizeof(uint_t));

  // chip-specific volume tables- ym supports 32 volume steps, ay - only 16
  typedef boost::array<Sound::Sample::Type, 32> VolTable;

  inline Sound::Sample::Type ToSample(uint_t val)
  {
    return Sound::Sample::MID + val * (Sound::Sample::MAX - Sound::Sample::MID) / (Sound::Sample::MAX - Sound::Sample::MIN);
  }

  const VolTable AYVolumeTab =
  { {
    ToSample(0x0000), ToSample(0x0000), ToSample(0x0340), ToSample(0x0340), ToSample(0x04C0), ToSample(0x04C0), ToSample(0x06F2), ToSample(0x06F2),
    ToSample(0x0A44), ToSample(0x0A44), ToSample(0x0F13), ToSample(0x0F13), ToSample(0x1510), ToSample(0x1510), ToSample(0x227E), ToSample(0x227E),
    ToSample(0x289F), ToSample(0x289F), ToSample(0x414E), ToSample(0x414E), ToSample(0x5B21), ToSample(0x5B21), ToSample(0x7258), ToSample(0x7258),
    ToSample(0x905E), ToSample(0x905E), ToSample(0xB550), ToSample(0xB550), ToSample(0xD7A0), ToSample(0xD7A0), ToSample(0xFFFF), ToSample(0xFFFF)
  } };
  const VolTable YMVolumeTab =
  { {
    ToSample(0x0000), ToSample(0x0000), ToSample(0x00EF), ToSample(0x01D0), ToSample(0x0290), ToSample(0x032A), ToSample(0x03EE), ToSample(0x04D2),
    ToSample(0x0611), ToSample(0x0782), ToSample(0x0912), ToSample(0x0A36), ToSample(0x0C31), ToSample(0x0EB6), ToSample(0x1130), ToSample(0x13A0),
    ToSample(0x1751), ToSample(0x1BF5), ToSample(0x20E2), ToSample(0x2594), ToSample(0x2CA1), ToSample(0x357F), ToSample(0x3E45), ToSample(0x475E),
    ToSample(0x5502), ToSample(0x6620), ToSample(0x7730), ToSample(0x8844), ToSample(0xA1D2), ToSample(0xC102), ToSample(0xE0A2), ToSample(0xFFFF)
  } };

  typedef boost::array<uint_t, Devices::AYM::SOUND_CHANNELS> LayoutData;

  const LayoutData LAYOUTS[] =
  {
    { {0, 1, 2} }, //ABC
    { {0, 2, 1} }, //ACB
    { {1, 0, 2} }, //BAC
    { {1, 2, 0} }, //BCA
    { {2, 1, 0} }, //CBA
    { {2, 0, 1} }, //CAB
  };

  const uint_t AYM_CLOCK_DIVISOR = 8;

  class AYMRenderer
  {
  public:
    AYMRenderer()
      : Registers()
      , Beeper()
    {
      Registers[DataChunk::REG_MIXER] = 0xff;
    }

    void SetDutyCycle(uint_t value, uint_t mask)
    {
      Device.SetDutyCycle(value, mask);
    }

    void Reset()
    {
      std::fill(Registers.begin(), Registers.end(), 0);
      Registers[DataChunk::REG_MIXER] = 0xff;
      Device.Reset();
      Beeper = 0;
    }

    void SetNewData(const DataChunk& data)
    {
      for (uint_t idx = 0, mask = 1; idx != Registers.size(); ++idx, mask <<= 1)
      {
        if (0 == (data.Mask & mask))
        {
          //no new data
          continue;
        }
        //copy registers
        uint8_t reg = data.Data[idx];
        //limit values
        if (mask & REGS_4BIT_SET)
        {
          reg &= 0x0f;
        }
        else if (mask & REGS_5BIT_SET)
        {
          reg &= 0x1f;
        }
        Registers[idx] = reg;
      }
      if (data.Mask & (1 << DataChunk::REG_MIXER))
      {
        Device.SetMixer(GetMixer());
      }
      if (data.Mask & ((1 << DataChunk::REG_TONEA_L) | (1 << DataChunk::REG_TONEA_H)))
      {
        Device.SetToneA(GetToneA());
      }
      if (data.Mask & ((1 << DataChunk::REG_TONEB_L) | (1 << DataChunk::REG_TONEB_H)))
      {
        Device.SetToneB(GetToneB());
      }
      if (data.Mask & ((1 << DataChunk::REG_TONEC_L) | (1 << DataChunk::REG_TONEC_H)))
      {
        Device.SetToneC(GetToneC());
      }
      if (data.Mask & (1 << DataChunk::REG_TONEN))
      {
        Device.SetToneN(GetToneN());
      }
      if (data.Mask & ((1 << DataChunk::REG_TONEE_L) | (1 << DataChunk::REG_TONEE_H)))
      {
        Device.SetToneE(GetToneE());
      }
      if (data.Mask & (1 << DataChunk::REG_ENV))
      {
        Device.SetEnvType(GetEnvType());
      }
      if (data.Mask & ((1 << DataChunk::REG_VOLA) | (1 << DataChunk::REG_VOLB) | (1 << DataChunk::REG_VOLC)))
      {
        Device.SetLevel(Registers[DataChunk::REG_VOLA], Registers[DataChunk::REG_VOLB], Registers[DataChunk::REG_VOLC]);
      }
      if (data.Mask & (1 << DataChunk::REG_BEEPER))
      {
        const uint_t inLevel = ((data.Data[DataChunk::REG_BEEPER] & DataChunk::REG_MASK_VOL) << 1) + 1;
        Beeper = inLevel | (inLevel << BITS_PER_LEVEL) | (inLevel << 2 * BITS_PER_LEVEL);
      }
    }

    void Tick(uint_t ticks)
    {
      Device.Tick(ticks);
    }

    uint_t GetLevels() const
    {
      if (Beeper)
      {
        return Beeper;
      }
      else
      {
        return Device.GetLevels();
      }
    }

    void GetState(ChannelsState& state) const
    {
      const uint_t TONE_VOICES = 3;
      const uint_t MAX_LEVEL = 100;
      //one channel is noise
      ChanState& noiseChan = state[TONE_VOICES];
      noiseChan = ChanState('N');
      noiseChan.Band = GetToneN();
      //one channel is envelope    
      ChanState& envChan = state[TONE_VOICES + 1];
      envChan = ChanState('E');
      envChan.Band = 16 * GetToneE();
      //taking into account only periodic envelope
      const bool periodicEnv = 0 != ((1 << GetEnvType()) & ((1 << 8) | (1 << 10) | (1 << 12) | (1 << 14)));
      const uint_t mixer = ~GetMixer();
      for (uint_t chan = 0; chan != TONE_VOICES; ++chan) 
      {
        const uint_t volReg = Registers[DataChunk::REG_VOLA + chan];
        const bool hasNoise = 0 != (mixer & (uint_t(DataChunk::REG_MASK_NOISEA) << chan));
        const bool hasTone = 0 != (mixer & (uint_t(DataChunk::REG_MASK_TONEA) << chan));
        const bool hasEnv = 0 != (volReg & DataChunk::REG_MASK_ENV);
        //accumulate level in noise channel
        if (hasNoise)
        {
          noiseChan.Enabled = true;
          noiseChan.LevelInPercents += MAX_LEVEL / TONE_VOICES;
        }
        //accumulate level in envelope channel      
        if (periodicEnv && hasEnv)
        {        
          envChan.Enabled = true;
          envChan.LevelInPercents += MAX_LEVEL / TONE_VOICES;
        }
        //calculate tone channel
        ChanState& channel = state[chan];
        channel.Name = static_cast<Char>('A' + chan);
        if (hasTone)
        {
          channel.Enabled = true;
          channel.LevelInPercents = (volReg & DataChunk::REG_MASK_VOL) * MAX_LEVEL / 15;
          //Use full period
          channel.Band = 2 * (256 * Registers[DataChunk::REG_TONEA_H + chan * 2] +
            Registers[DataChunk::REG_TONEA_L + chan * 2]);
        }
      } 
    }
  private:
    uint_t GetMixer() const
    {
      return Registers[DataChunk::REG_MIXER];
    }

    uint_t GetToneA() const
    {
      return 256 * Registers[DataChunk::REG_TONEA_H] + Registers[DataChunk::REG_TONEA_L];
    }

    uint_t GetToneB() const
    {
      return 256 * Registers[DataChunk::REG_TONEB_H] + Registers[DataChunk::REG_TONEB_L];
    }

    uint_t GetToneC() const
    {
      return 256 * Registers[DataChunk::REG_TONEC_H] + Registers[DataChunk::REG_TONEC_L];
    }

    uint_t GetToneN() const
    {
      return 2 * Registers[DataChunk::REG_TONEN];//for optimization
    }

    uint_t GetToneE() const
    {
      return 256 * Registers[DataChunk::REG_TONEE_H] + Registers[DataChunk::REG_TONEE_L];
    }

    uint_t GetEnvType() const
    {
      return Registers[DataChunk::REG_ENV];
    }
  private:
    //registers state
    boost::array<uint_t, DataChunk::REG_LAST_AY> Registers;
    //device
    AYMDevice Device;
    uint_t Beeper;
  };

  /*
  class RelayoutTarget : public Receiver
  {
  public:
    RelayoutTarget(Receiver& delegate, LayoutType layout)
      : Delegate(delegate)
      , Layout(LAYOUTS[layout])
    {
    }

    virtual void ApplyData(const MultiSample& in)
    {
      const MultiSample out =
      {{
        in[Layout[0]],
        in[Layout[1]],
        in[Layout[2]]
      }};
      return Delegate.ApplyData(out);
    }

    virtual void Flush()
    {
      return Delegate.Flush();
    }
  private:
    Receiver& Delegate;
    const LayoutData Layout;
  };

  class MonoTarget : public Receiver
  {
  public:
    MonoTarget(Receiver& delegate)
      : Delegate(delegate)
    {
    }

    virtual void ApplyData(const MultiSample& in)
    {
      const Sample average = static_cast<Sample>(std::accumulate(in.begin(), in.end(), uint_t(0)) / in.size());
      const MultiSample out =
      {{
        average,
        average,
        average
      }};
      Delegate.ApplyData(out);
    }

    virtual void Flush()
    {
      return Delegate.Flush();
    }
  private:
    Receiver& Delegate;
  };
  */

  class ClockSource
  {
  public:
    void SetFrequency(uint64_t clockFreq, uint_t soundFreq)
    {
      SndOscillator.SetFrequency(soundFreq);
      PsgOscillator.SetFrequency(clockFreq);
    }

    void Reset()
    {
      PsgOscillator.Reset();
      SndOscillator.Reset();
    }

    Stamp GetCurrentTime() const
    {
      return PsgOscillator.GetCurrentTime();
    }

    Stamp GetNextSampleTime() const
    {
      return SndOscillator.GetCurrentTime();
    }

    void NextSample()
    {
      SndOscillator.AdvanceTick();
    }

    uint_t NextTime(const Stamp& stamp)
    {
      const Stamp prevStamp = PsgOscillator.GetCurrentTime();
      const uint64_t prevTick = PsgOscillator.GetCurrentTick();
      PsgOscillator.AdvanceTime(stamp.Get() - prevStamp.Get());
      return static_cast<uint_t>(PsgOscillator.GetCurrentTick() - prevTick);
    }
  private:
    Time::Oscillator<Stamp> SndOscillator;
    Time::TimedOscillator<Stamp> PsgOscillator;
  };

  class DataCache
  {
  public:
    void Add(const DataChunk& src)
    {
      Buffer.push_back(src);
    }

    const DataChunk* GetBegin() const
    {
      return &Buffer.front();
    }
    
    const DataChunk* GetEnd() const
    {
      return &Buffer.back() + 1;
    }

    void Reset()
    {
      Buffer.clear();
    }
  private:
    std::vector<DataChunk> Buffer;
  };
  
  class AnalysisMap
  {
  public:
    AnalysisMap()
      : ClockRate()
    {
    }

    void SetClockRate(uint64_t clock)
    {
      //table in Hz * FREQ_MULTIPLIER
      static const NoteTable FREQUENCIES =
      { {
        //octave1
        3270,   3465,   3671,   3889,   4120,   4365,   4625,   4900,   5191,   5500,   5827,   6173,
        //octave2
        6541,   6929,   7342,   7778,   8241,   8730,   9250,   9800,  10382,  11000,  11654,  12346,
        //octave3
        13082,  13858,  14684,  15556,  16482,  17460,  18500,  19600,  20764,  22000,  23308,  24692,
        //octave4
        26164,  27716,  29368,  31112,  32964,  34920,  37000,  39200,  41528,  44000,  46616,  49384,
        //octave5
        52328,  55432,  58736,  62224,  65928,  69840,  74000,  78400,  83056,  88000,  93232,  98768,
        //octave6
        104650, 110860, 117470, 124450, 131860, 139680, 148000, 156800, 166110, 176000, 186460, 197540,
        //octave7
        209310, 221720, 234940, 248890, 263710, 279360, 296000, 313600, 332220, 352000, 372930, 395070,
        //octave8
        418620, 443460, 469890, 497790, 527420, 558720, 592000, 627200, 664450, 704000, 745860, 790140,
        //octave9
        837200, 886980, 939730, 995610,1054800,1117500,1184000,1254400,1329000,1408000,1491700,1580400
      } };
      if (ClockRate == clock)
      {
        return;
      }
      ClockRate = clock;
      std::transform(FREQUENCIES.begin(), FREQUENCIES.end(), Lookup.rbegin(), std::bind1st(std::ptr_fun(&GetPeriod), clock));
    }
    
    uint_t GetBandByPeriod(uint_t period) const
    {
      const uint_t maxBand = static_cast<uint_t>(Lookup.size() - 1);
      const uint_t currentBand = static_cast<uint_t>(Lookup.end() - std::lower_bound(Lookup.begin(), Lookup.end(), period));
      return std::min(currentBand, maxBand);
    }
  private:
    static const uint_t FREQ_MULTIPLIER = 100;

    static uint_t GetPeriod(uint64_t clock, uint_t freq)
    {
      return static_cast<uint_t>(clock * FREQ_MULTIPLIER / freq);
    }
  private:
    uint64_t ClockRate;
    typedef boost::array<uint_t, 12 * 9> NoteTable;
    NoteTable Lookup;
  };

  class MultiVolumeTable
  {
  public:
    MultiVolumeTable()
      : Type(static_cast<ChipType>(-1))
      , Mixer()
    {
      Reset();
    }

    void Reset()
    {
      SetType(TYPE_AY38910);
    }

    void SetType(ChipType type)
    {
      if (Type != type)
      {
        Type = type;
        FillLookupTable(GetVolumeTable(type));
      }
    }

    void SetMixer(const MixerType& mixer)
    {
      Mixer = &mixer;
    }

    Sound::Sample Get(uint_t in) const
    {
      return Mixer->ApplyData(Lookup[in]);
    }
  private:
    static const VolTable& GetVolumeTable(ChipType type)
    {
      switch (type)
      {
      case TYPE_YM2149F:
        return YMVolumeTab;
      default:
        return AYVolumeTab;
      }
    }

    typedef MixerType::InDataType MultiSample;

    void FillLookupTable(const VolTable& table)
    {
      BOOST_STATIC_ASSERT(0 == sizeof(AlignedSample) % sizeof(uint_t));
      for (uint_t idx = 0; idx != Lookup.size(); ++idx)
      {
        const MultiSample res =
        {{
          table[idx & HIGH_LEVEL_A],
          table[(idx >> BITS_PER_LEVEL) & HIGH_LEVEL_A],
          table[idx >> 2 * BITS_PER_LEVEL]
        }};
        Lookup[idx] = res;
      }
    }
  private:
    ChipType Type;
    const MixerType* Mixer;

    struct AlignedSample : MultiSample
    {
      Sound::Sample::Type Alignment;
      
      void operator = (const MultiSample& rh)
      {
        static_cast<MultiSample&>(*this) = rh;
      }
    };
    boost::array<AlignedSample, 1 << SOUND_CHANNELS * BITS_PER_LEVEL> Lookup;
  };

  class Renderer
  {
  public:
    virtual ~Renderer() {}

    virtual void Render(const Stamp& tillTime, Sound::Receiver& target) = 0;
  };

  /*
    Simple decimation algorithm without any filtering
  */
  class LQRenderer : public Renderer
  {
  public:
    LQRenderer(ClockSource& clock, AYMRenderer& psg, const MultiVolumeTable& tab)
      : Clock(clock)
      , PSG(psg)
      , Table(tab)
    {
    }

    virtual void Render(const Stamp& tillTime, Sound::Receiver& target)
    {
      for (;;)
      {
        const Stamp& nextSampleTime = Clock.GetNextSampleTime();
        if (!(nextSampleTime < tillTime))
        {
          break;
        }
        else if (const uint_t ticksPassed = Clock.NextTime(nextSampleTime))
        {
          PSG.Tick(ticksPassed);
        }
        RenderNextSample(target);
      }
      if (const uint_t ticksPassed = Clock.NextTime(tillTime))
      {
        PSG.Tick(ticksPassed);
      }
    }
  private:
    void RenderNextSample(Sound::Receiver& target)
    {
      const uint_t psgLevel = PSG.GetLevels();
      const Sound::Sample sndLevel = Table.Get(psgLevel);
      target.ApplyData(sndLevel);
      Clock.NextSample();
    }
  private:
    ClockSource& Clock;
    AYMRenderer& PSG;
    const MultiVolumeTable& Table;
  };

  /*
    Simple decimation with post simple FIR filter (0.5, 0.5)
  */
  class MQRenderer : public Renderer
  {
  public:
    MQRenderer(ClockSource& clock, AYMRenderer& psg, const MultiVolumeTable& tab)
      : Clock(clock)
      , PSG(psg)
      , Table(tab)
    {
    }

    virtual void Render(const Stamp& tillTime, Sound::Receiver& target)
    {
      for (;;)
      {
        const Stamp& nextSampleTime = Clock.GetNextSampleTime();
        if (!(nextSampleTime < tillTime))
        {
          break;
        }
        else if (const uint_t ticksPassed = Clock.NextTime(nextSampleTime))
        {
          PSG.Tick(ticksPassed);
        }
        RenderNextSample(target);
      }
      if (const uint_t ticksPassed = Clock.NextTime(tillTime))
      {
        PSG.Tick(ticksPassed);
      }
    }
  private:
    void RenderNextSample(Sound::Receiver& target)
    {
      const uint_t psgLevel = PSG.GetLevels();
      const Sound::Sample curLevel = Table.Get(psgLevel);
      const Sound::Sample sndLevel = Interpolate(curLevel);
      target.ApplyData(sndLevel);
      Clock.NextSample();
    }

    Sound::Sample Interpolate(const Sound::Sample& newLevel)
    {
      const Sound::Sample out(Average(PrevLevel.Left(), newLevel.Left()), Average(PrevLevel.Right(), newLevel.Right()));
      PrevLevel = newLevel;
      return out;
    }

    static Sound::Sample::Type Average(Sound::Sample::Type first, Sound::Sample::Type second)
    {
      return static_cast<Sound::Sample::Type>((int_t(first) + second) / 2);
    }
  private:
    ClockSource& Clock;
    AYMRenderer& PSG;
    const MultiVolumeTable& Table;
    Sound::Sample PrevLevel;
  };

  /*
    Decimation is performed after 2-order IIR LPF
    Cutoff freq of LPF should be less than Nyquist frequency of target signal
  */
  class HQRenderer : public Renderer
  {
  public:
    HQRenderer(ClockSource& clock, AYMRenderer& psg, const MultiVolumeTable& tab)
      : Clock(clock)
      , PSG(psg)
      , Table(tab)
      , ClockFreq()
      , SoundFreq()
    {
    }

    void SetFrequency(uint64_t clockFreq, uint_t soundFreq)
    {
      if (ClockFreq != clockFreq || SoundFreq != soundFreq)
      {
        Filter.SetParameters(clockFreq, soundFreq / 4);
        ClockFreq = clockFreq;
        SoundFreq = soundFreq;
      }
    }

    virtual void Render(const Stamp& tillTime, Sound::Receiver& target)
    {
      for (;;)
      {
        const Stamp& nextSampleTime = Clock.GetNextSampleTime();
        if (!(nextSampleTime < tillTime))
        {
          break;
        }
        else if (const uint_t ticksPassed = Clock.NextTime(nextSampleTime))
        {
          RenderTicks(ticksPassed);
        }
        RenderNextSample(target);
      }
      if (const uint_t ticksPassed = Clock.NextTime(tillTime))
      {
        RenderTicks(ticksPassed);
      }
    }
  private:
    void RenderTicks(uint_t ticksPassed)
    {
      while (ticksPassed--)
      {
        const uint_t psgLevel = PSG.GetLevels();
        const Sound::Sample curLevel = Table.Get(psgLevel);
        Filter.Feed(curLevel);
        PSG.Tick(1);
      }
    }

    void RenderNextSample(Sound::Receiver& target)
    {
      target.ApplyData(Filter.Get());
      Clock.NextSample();
    }
  private:
    ClockSource& Clock;
    AYMRenderer& PSG;
    const MultiVolumeTable& Table;
    uint64_t ClockFreq;
    uint_t SoundFreq;
    Sound::LPFilter Filter;
  };

  class RegularAYMChip : public Chip
  {
  public:
    RegularAYMChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
      : Params(params)
      , Target(target)
      , Clock()
      , LQ(Clock, PSG, VolTable)
      , MQ(Clock, PSG, VolTable)
      , HQ(Clock, PSG, VolTable)
    {
      Reset();
    }

    virtual void RenderData(const DataChunk& src)
    {
      BufferedData.Add(src);
    }

    virtual void Flush()
    {
      ApplyParameters();
      Renderer& source = GetRenderer();
      const LayoutType layout = Params->Layout();
      //if (LAYOUT_ABC == layout)
      {
        RenderChunks(source, *Target);
      }
      /*
      else if (LAYOUT_MONO == layout)
      {
        MonoTarget monoTarget(*Target);
        RenderChunks(source, monoTarget);
      }
      else
      {
        RelayoutTarget relayoutTarget(*Target, layout);
        RenderChunks(source, relayoutTarget);
      }
      */
      Target->Flush();
    }

    virtual void GetState(ChannelsState& state) const
    {
      PSG.GetState(state);
      for (ChannelsState::iterator it = state.begin(), lim = state.end(); it != lim; ++it)
      {
        it->Band = it->Enabled ? Analyser.GetBandByPeriod(it->Band) : 0;
      }
    }

    virtual void Reset()
    {
      PSG.Reset();
      Clock.Reset();
      BufferedData.Reset();
      VolTable.Reset();
    }
  private:
    void ApplyParameters()
    {
      PSG.SetDutyCycle(Params->DutyCycleValue(), Params->DutyCycleMask());
      const uint64_t clock = Params->ClockFreq() / AYM_CLOCK_DIVISOR;
      const uint_t sndFreq = Params->SoundFreq();
      Clock.SetFrequency(clock, sndFreq);
      Analyser.SetClockRate(clock);
      VolTable.SetType(Params->Type());
      VolTable.SetMixer(Params->Mixer());
      HQ.SetFrequency(clock, sndFreq);
    }

    Renderer& GetRenderer()
    {
      switch (Params->Interpolation())
      {
      case INTERPOLATION_LQ:
        return MQ;
      case INTERPOLATION_HQ:
        return HQ;
      default:
        return LQ;
      }
    }

    void RenderChunks(Renderer& source, Sound::Receiver& target)
    {
      for (const DataChunk* it = BufferedData.GetBegin(), *lim = BufferedData.GetEnd(); it != lim; ++it)
      {
        const DataChunk& chunk = *it;
        if (Clock.GetCurrentTime() < chunk.TimeStamp)
        {
          source.Render(chunk.TimeStamp, target);
        }
        PSG.SetNewData(chunk);
      }
      BufferedData.Reset();
    }
  private:
    const ChipParameters::Ptr Params;
    const Sound::Receiver::Ptr Target;
    AYMRenderer PSG;
    ClockSource Clock;
    DataCache BufferedData;
    AnalysisMap Analyser;
    MultiVolumeTable VolTable;
    LQRenderer LQ;
    MQRenderer MQ;
    HQRenderer HQ;
  };
}

namespace Devices
{
  namespace AYM
  {
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
    {
      return Chip::Ptr(new RegularAYMChip(params, target));
    }
  }
}
