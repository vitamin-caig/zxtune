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
#include <devices/details/analysis_map.h>
#include <devices/details/chunks_cache.h>
#include <devices/details/clock_source.h>
#include <devices/details/parameters_helper.h>
#include <sound/chunk_builder.h>
#include <sound/lpfilter.h>
//std includes
#include <cassert>
#include <functional>
#include <memory>
#include <numeric>
//boost includes
#include <boost/scoped_ptr.hpp>

namespace Devices
{
namespace AYM
{

  enum
  {
    // set of registers which required input data masking (4 or 5 lsb)
    REGS_4BIT_SET = (1 << Registers::TONEA_H) | (1 << Registers::TONEB_H) |
                    (1 << Registers::TONEC_H) | (1 << Registers::ENV),
    REGS_5BIT_SET = (1 << Registers::TONEN) | (1 << Registers::VOLA) |
                    (1 << Registers::VOLB) | (1 << Registers::VOLC),
  };

  BOOST_STATIC_ASSERT(Registers::TOTAL == 14);
  BOOST_STATIC_ASSERT(sizeof(Registers) == 16);

  inline Sound::Sample::Type ToSample(uint_t val)
  {
    return Sound::Sample::MID + val * (Sound::Sample::MAX - Sound::Sample::MID) / (Sound::Sample::MAX - Sound::Sample::MIN);
  }

  const Sound::Sample::Type AYVolumeTab[32] =
  {
    ToSample(0x0000), ToSample(0x0000), ToSample(0x0340), ToSample(0x0340), ToSample(0x04C0), ToSample(0x04C0), ToSample(0x06F2), ToSample(0x06F2),
    ToSample(0x0A44), ToSample(0x0A44), ToSample(0x0F13), ToSample(0x0F13), ToSample(0x1510), ToSample(0x1510), ToSample(0x227E), ToSample(0x227E),
    ToSample(0x289F), ToSample(0x289F), ToSample(0x414E), ToSample(0x414E), ToSample(0x5B21), ToSample(0x5B21), ToSample(0x7258), ToSample(0x7258),
    ToSample(0x905E), ToSample(0x905E), ToSample(0xB550), ToSample(0xB550), ToSample(0xD7A0), ToSample(0xD7A0), ToSample(0xFFFF), ToSample(0xFFFF)
  };
  const Sound::Sample::Type YMVolumeTab[32] =
  {
    ToSample(0x0000), ToSample(0x0000), ToSample(0x00EF), ToSample(0x01D0), ToSample(0x0290), ToSample(0x032A), ToSample(0x03EE), ToSample(0x04D2),
    ToSample(0x0611), ToSample(0x0782), ToSample(0x0912), ToSample(0x0A36), ToSample(0x0C31), ToSample(0x0EB6), ToSample(0x1130), ToSample(0x13A0),
    ToSample(0x1751), ToSample(0x1BF5), ToSample(0x20E2), ToSample(0x2594), ToSample(0x2CA1), ToSample(0x357F), ToSample(0x3E45), ToSample(0x475E),
    ToSample(0x5502), ToSample(0x6620), ToSample(0x7730), ToSample(0x8844), ToSample(0xA1D2), ToSample(0xC102), ToSample(0xE0A2), ToSample(0xFFFF)
  };

  const uint_t SOUND_CHANNELS = 3;
  typedef Sound::ThreeChannelsMixer MixerType;

  typedef boost::array<uint_t, SOUND_CHANNELS> LayoutData;

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
      : Regs()
      , Beeper()
    {
      Regs[Registers::MIXER] = 0xff;
    }

    void SetDutyCycle(uint_t value, uint_t mask)
    {
      Device.SetDutyCycle(value, mask);
    }

    void Reset()
    {
      std::fill(Regs.begin(), Regs.end(), 0);
      Regs[Registers::MIXER] = 0xff;
      Device.Reset();
      Beeper = 0;
    }

    void SetNewData(const Registers& data)
    {
      uint_t used = 0;
      for (Registers::IndicesIterator it(data); it; ++it)
      {
        const Registers::Index reg = *it;
        if (!data.Has(reg))
        {
          //no new data
          continue;
        }
        //copy registers
        uint8_t val = data[reg];
        const uint_t mask = 1 << reg;
        //limit values
        if (mask & REGS_4BIT_SET)
        {
          val &= 0x0f;
        }
        else if (mask & REGS_5BIT_SET)
        {
          val &= 0x1f;
        }
        Regs[reg] = val;
        used |= mask;
      }
      if (used & (1 << Registers::MIXER))
      {
        Device.SetMixer(GetMixer());
      }
      if (used & ((1 << Registers::TONEA_L) | (1 << Registers::TONEA_H)))
      {
        Device.SetToneA(GetToneA());
      }
      if (used & ((1 << Registers::TONEB_L) | (1 << Registers::TONEB_H)))
      {
        Device.SetToneB(GetToneB());
      }
      if (used & ((1 << Registers::TONEC_L) | (1 << Registers::TONEC_H)))
      {
        Device.SetToneC(GetToneC());
      }
      if (used & (1 << Registers::TONEN))
      {
        Device.SetToneN(GetToneN());
      }
      if (used & ((1 << Registers::TONEE_L) | (1 << Registers::TONEE_H)))
      {
        Device.SetToneE(GetToneE());
      }
      if (used & (1 << Registers::ENV))
      {
        Device.SetEnvType(GetEnvType());
      }
      if (used & ((1 << Registers::VOLA) | (1 << Registers::VOLB) | (1 << Registers::VOLC)))
      {
        Device.SetLevel(Regs[Registers::VOLA], Regs[Registers::VOLB], Regs[Registers::VOLC]);
      }
      if (data.HasBeeper())
      {
        Beeper = data.GetBeeper() ? HIGH_LEVEL : LOW_LEVEL;
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

    void GetState(MultiChannelState& state) const
    {
      const uint_t TONE_VOICES = 3;
      const LevelType COMMON_LEVEL_DELTA(1, TONE_VOICES);
      const LevelType EMPTY_LEVEL;
      LevelType noiseLevel, envLevel;
      //taking into account only periodic envelope
      const bool periodicEnv = 0 != ((1 << GetEnvType()) & ((1 << 8) | (1 << 10) | (1 << 12) | (1 << 14)));
      const uint_t mixer = ~GetMixer();
      for (uint_t chan = 0; chan != TONE_VOICES; ++chan) 
      {
        const uint_t volReg = Regs[Registers::VOLA + chan];
        const bool hasNoise = 0 != (mixer & (uint_t(Registers::MASK_NOISEA) << chan));
        const bool hasTone = 0 != (mixer & (uint_t(Registers::MASK_TONEA) << chan));
        const bool hasEnv = 0 != (volReg & Registers::MASK_ENV);
        //accumulate level in noise channel
        if (hasNoise)
        {
          noiseLevel += COMMON_LEVEL_DELTA;
        }
        //accumulate level in envelope channel      
        if (periodicEnv && hasEnv)
        {        
          envLevel += COMMON_LEVEL_DELTA;
        }
        //calculate tone channel
        ChannelState& channel = state[chan];
        channel = ChannelState();
        if (hasTone)
        {
          const uint_t MAX_VOL = Registers::MASK_VOL;
          const LevelType level(volReg & Registers::MASK_VOL, MAX_VOL);
          const uint_t band = 2 * (256 * Regs[Registers::TONEA_H + chan * 2] +
            Regs[Registers::TONEA_L + chan * 2]);
          state.push_back(ChannelState(band, level));
        }
      }
      if (noiseLevel != EMPTY_LEVEL)
      {
        state.push_back(ChannelState(GetToneN(), noiseLevel));
      }
      if (envLevel != EMPTY_LEVEL)
      {
        state.push_back(ChannelState(16 * GetToneE(), envLevel));
      }  
    }
  private:
    uint_t GetMixer() const
    {
      return Regs[Registers::MIXER];
    }

    uint_t GetToneA() const
    {
      return 256 * Regs[Registers::TONEA_H] + Regs[Registers::TONEA_L];
    }

    uint_t GetToneB() const
    {
      return 256 * Regs[Registers::TONEB_H] + Regs[Registers::TONEB_L];
    }

    uint_t GetToneC() const
    {
      return 256 * Regs[Registers::TONEC_H] + Regs[Registers::TONEC_L];
    }

    uint_t GetToneN() const
    {
      return 2 * Regs[Registers::TONEN];//for optimization
    }

    uint_t GetToneE() const
    {
      return 256 * Regs[Registers::TONEE_H] + Regs[Registers::TONEE_L];
    }

    uint_t GetEnvType() const
    {
      return Regs[Registers::ENV];
    }
  private:
    //registers state
    boost::array<uint_t, Registers::TOTAL> Regs;
    //device
    AYMDevice Device;
    uint_t Beeper;
  };

  typedef Details::ClockSource<Stamp> ClockSource;

  class MultiVolumeTable
  {
  public:
    MultiVolumeTable()
      : Table(0)
      , Layout(0)
    {
    }

    void SetParameters(ChipType type, LayoutType layout, const MixerType& mixer)
    {
      assert(AYVolumeTab[HIGH_LEVEL_A] == Sound::Sample::MAX);
      assert(YMVolumeTab[HIGH_LEVEL_A] == Sound::Sample::MAX);
      static const MultiSample IN_A = {{Sound::Sample::MAX, Sound::Sample::MID, Sound::Sample::MID}};
      static const MultiSample IN_B = {{Sound::Sample::MID, Sound::Sample::MAX, Sound::Sample::MID}};
      static const MultiSample IN_C = {{Sound::Sample::MID, Sound::Sample::MID, Sound::Sample::MAX}};

      const Sound::Sample::Type* const newTable = GetVolumeTable(type);
      const LayoutData* const newLayout = GetLayout(layout);
      if (Table != newTable
       || Layout != newLayout
       //simple mixer fingerprint
       || !(Lookup[HIGH_LEVEL_A] == Mix(IN_A, mixer))
       || !(Lookup[HIGH_LEVEL_B] == Mix(IN_B, mixer))
       || !(Lookup[HIGH_LEVEL_C] == Mix(IN_C, mixer)))
      {
        Table = newTable;
        Layout = newLayout;
        FillLookupTable(mixer);
      }
    }

    Sound::Sample Get(uint_t in) const
    {
      return Lookup[in];
    }
  private:
    static const Sound::Sample::Type* GetVolumeTable(ChipType type)
    {
      switch (type)
      {
      case TYPE_YM2149F:
        return YMVolumeTab;
      default:
        return AYVolumeTab;
      }
    }

    static const LayoutData* GetLayout(LayoutType type)
    {
      return type == LAYOUT_MONO
        ? 0
        : LAYOUTS + type;
    }

    typedef MixerType::InDataType MultiSample;

    void FillLookupTable(const MixerType& mixer)
    {
      for (uint_t idx = 0; idx != Lookup.size(); ++idx)
      {
        const MultiSample res =
        {{
          Table[idx & HIGH_LEVEL_A],
          Table[(idx >> BITS_PER_LEVEL) & HIGH_LEVEL_A],
          Table[idx >> 2 * BITS_PER_LEVEL]
        }};
        Lookup[idx] = Mix(res, mixer);
      }
    }

    Sound::Sample Mix(const MultiSample& in, const MixerType& mixer) const
    {
      if (Layout)
      {
        const MultiSample out =
        {{
          in[Layout->at(0)],
          in[Layout->at(1)],
          in[Layout->at(2)]
        }};
        return mixer.ApplyData(out);
      }
      else//mono
      {
        const Sound::Sample::Type avg = (int_t(in[0]) + in[1] + in[2]) / SOUND_CHANNELS;
        const MultiSample out = {{avg, avg, avg}};
        return mixer.ApplyData(out);
      }
    }
  private:
    const Sound::Sample::Type* Table;
    const LayoutData* Layout;
    boost::array<Sound::Sample, 1 << SOUND_CHANNELS * BITS_PER_LEVEL> Lookup;
  };

  class Renderer
  {
  public:
    virtual ~Renderer() {}

    virtual void Render(const Stamp& tillTime, Sound::ChunkBuilder& target) = 0;
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

    virtual void Render(const Stamp& tillTime, Sound::ChunkBuilder& target)
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
    void RenderNextSample(Sound::ChunkBuilder& target)
    {
      const uint_t psgLevel = PSG.GetLevels();
      const Sound::Sample sndLevel = Table.Get(psgLevel);
      target.Add(sndLevel);
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

    virtual void Render(const Stamp& tillTime, Sound::ChunkBuilder& target)
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
    void RenderNextSample(Sound::ChunkBuilder& target)
    {
      const uint_t psgLevel = PSG.GetLevels();
      const Sound::Sample curLevel = Table.Get(psgLevel);
      const Sound::Sample sndLevel = Interpolate(curLevel);
      target.Add(sndLevel);
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
    {
    }

    void SetFrequency(uint64_t clockFreq, uint_t soundFreq)
    {
      Filter.SetParameters(clockFreq, soundFreq / 4);
    }

    virtual void Render(const Stamp& tillTime, Sound::ChunkBuilder& target)
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

    void RenderNextSample(Sound::ChunkBuilder& target)
    {
      target.Add(Filter.Get());
      Clock.NextSample();
    }
  private:
    ClockSource& Clock;
    AYMRenderer& PSG;
    const MultiVolumeTable& Table;
    Sound::LPFilter Filter;
  };

  class RenderersSet
  {
  public:
    RenderersSet(ClockSource& clock, AYMRenderer& psg, const MultiVolumeTable& tab)
      : ClockFreq()
      , SoundFreq()
      , Clock(clock)
      , LQ(clock, psg, tab)
      , MQ(clock, psg, tab)
      , HQ(clock, psg, tab)
      , Current()
    {
    }

    void Reset()
    {
      ClockFreq = 0;
      SoundFreq = 0;
      Current = 0;
      Clock.Reset();
    }

    void SetFrequency(uint64_t clockFreq, uint_t soundFreq)
    {
      if (ClockFreq != clockFreq || SoundFreq != soundFreq)
      {
        Clock.SetFrequency(clockFreq, soundFreq);
        HQ.SetFrequency(clockFreq, soundFreq);
        ClockFreq = clockFreq;
        SoundFreq = soundFreq;
      }
    }

    void SetInterpolation(InterpolationType type)
    {
      switch (type)
      {
      case INTERPOLATION_LQ:
        Current = &MQ;
        break;
      case INTERPOLATION_HQ:
        Current = &HQ;
        break;
      default:
        Current = &LQ;
        break;
      }
    }

    Renderer& Get() const
    {
      return *Current;
    }
  private:
    uint64_t ClockFreq;
    uint_t SoundFreq;
    ClockSource& Clock;
    LQRenderer LQ;
    MQRenderer MQ;
    HQRenderer HQ;
    Renderer* Current;
  };

  class RegularAYMChip : public Chip
  {
  public:
    RegularAYMChip(ChipParameters::Ptr params, Sound::ThreeChannelsMixer::Ptr mixer, Sound::Receiver::Ptr target)
      : Params(params)
      , Mixer(mixer)
      , Target(target)
      , Clock()
      , Renderers(Clock, PSG, VolTable)
    {
      RegularAYMChip::Reset();
    }

    virtual void RenderData(const DataChunk& src)
    {
      BufferedData.Add(src);
    }

    virtual void Flush()
    {
      const Stamp till = BufferedData.GetTillTime();
      if (!(till == Stamp(0)))
      {
        SynchronizeParameters();
        Sound::ChunkBuilder builder;
        builder.Reserve(Clock.SamplesTill(till));
        RenderChunks(builder);
        Target->ApplyData(builder.GetResult());
      }
      Target->Flush();
    }

    virtual void Reset()
    {
      Params.Reset();
      PSG.Reset();
      BufferedData.Reset();
      Renderers.Reset();
    }

    virtual void GetState(MultiChannelState& state) const
    {
      MultiChannelState res;
      res.reserve(VOICES);
      PSG.GetState(res);
      for (MultiChannelState::iterator it = res.begin(), lim = res.end(); it != lim; ++it)
      {
        it->Band = Analyser.GetBandByPeriod(it->Band);
      }
      state.swap(res);
    }
  private:
    void SynchronizeParameters()
    {
      if (Params.IsChanged())
      {
        PSG.SetDutyCycle(Params->DutyCycleValue(), Params->DutyCycleMask());
        const uint64_t clock = Params->ClockFreq() / AYM_CLOCK_DIVISOR;
        const uint_t sndFreq = Params->SoundFreq();
        Renderers.SetFrequency(clock, sndFreq);
        Renderers.SetInterpolation(Params->Interpolation());
        Analyser.SetClockRate(clock);
        VolTable.SetParameters(Params->Type(), Params->Layout(), *Mixer);
      }
    }

    void RenderChunks(Sound::ChunkBuilder& builder)
    {
      Renderer& source = Renderers.Get();
      for (const DataChunk* it = BufferedData.GetBegin(), *lim = BufferedData.GetEnd(); it != lim; ++it)
      {
        const DataChunk& chunk = *it;
        if (Clock.GetCurrentTime() < chunk.TimeStamp)
        {
          source.Render(chunk.TimeStamp, builder);
        }
        PSG.SetNewData(chunk.Data);
      }
      BufferedData.Reset();
    }
  private:
    Details::ParametersHelper<ChipParameters> Params;
    const Sound::ThreeChannelsMixer::Ptr Mixer;
    const Sound::Receiver::Ptr Target;
    AYMRenderer PSG;
    ClockSource Clock;
    Details::ChunksCache<DataChunk, Stamp> BufferedData;
    Details::AnalysisMap Analyser;
    MultiVolumeTable VolTable;
    RenderersSet Renderers;
  };

  Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::ThreeChannelsMixer::Ptr mixer, Sound::Receiver::Ptr target)
  {
    return Chip::Ptr(new RegularAYMChip(params, mixer, target));
  }
}
}
