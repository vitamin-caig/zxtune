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
#include <time/oscillator.h>
//std includes
#include <cassert>
#include <cmath>
#include <functional>
#include <limits>
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
  const VolTable AYVolumeTab =
  { {
    0x0000, 0x0000, 0x0340, 0x0340, 0x04C0, 0x04C0, 0x06F2, 0x06F2,
    0x0A44, 0x0A44, 0x0F13, 0x0F13, 0x1510, 0x1510, 0x227E, 0x227E,
    0x289F, 0x289F, 0x414E, 0x414E, 0x5B21, 0x5B21, 0x7258, 0x7258,
    0x905E, 0x905E, 0xB550, 0xB550, 0xD7A0, 0xD7A0, 0xFFFF, 0xFFFF
  } };
  const VolTable YMVolumeTab =
  { {
    0x0000, 0x0000, 0x00EF, 0x01D0, 0x0290, 0x032A, 0x03EE, 0x04D2,
    0x0611, 0x0782, 0x0912, 0x0A36, 0x0C31, 0x0EB6, 0x1130, 0x13A0,
    0x1751, 0x1BF5, 0x20E2, 0x2594, 0x2CA1, 0x357F, 0x3E45, 0x475E,
    0x5502, 0x6620, 0x7730, 0x8844, 0xA1D2, 0xC102, 0xE0A2, 0xFFFF
  } };

  typedef boost::array<uint_t, Devices::AYM::CHANNELS> LayoutData;

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
      Registers[DataChunk::REG_MIXER ] = 0xff;
    }

    void SetDutyCycle(uint_t value, uint_t mask)
    {
      Device.SetDutyCycle(value, mask);
    }

    void Reset()
    {
      std::fill(Registers.begin(), Registers.end(), 0);
      Registers[DataChunk::REG_MIXER ] = 0xff;
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
      const uint_t MAX_LEVEL = 100;
      //one channel is noise
      ChanState& noiseChan = state[CHANNELS];
      noiseChan = ChanState('N');
      noiseChan.Band = GetToneN();
      //one channel is envelope    
      ChanState& envChan = state[CHANNELS + 1];
      envChan = ChanState('E');
      envChan.Band = 16 * GetToneE();
      //taking into account only periodic envelope
      const bool periodicEnv = 0 != ((1 << GetEnvType()) & ((1 << 8) | (1 << 10) | (1 << 12) | (1 << 14)));
      const uint_t mixer = ~GetMixer();
      for (uint_t chan = 0; chan != CHANNELS; ++chan) 
      {
        const uint_t volReg = Registers[DataChunk::REG_VOLA + chan];
        const bool hasNoise = 0 != (mixer & (uint_t(DataChunk::REG_MASK_NOISEA) << chan));
        const bool hasTone = 0 != (mixer & (uint_t(DataChunk::REG_MASK_TONEA) << chan));
        const bool hasEnv = 0 != (volReg & DataChunk::REG_MASK_ENV);
        //accumulate level in noise channel
        if (hasNoise)
        {
          noiseChan.Enabled = true;
          noiseChan.LevelInPercents += MAX_LEVEL / CHANNELS;
        }
        //accumulate level in envelope channel      
        if (periodicEnv && hasEnv)
        {        
          envChan.Enabled = true;
          envChan.LevelInPercents += MAX_LEVEL / CHANNELS;
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

  static Sample Average(Sample first, Sample second)
  {
    return static_cast<Sample>((uint_t(first) + second) / 2);
  }

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
      : Table()
    {
      Reset();
    }

    void Reset()
    {
      Set(GetAY38910VolTable());
    }

    void Set(const VolTable& table)
    {
      if (Table != &table)
      {
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
        Table = &table;
      }
    }

    MultiSample Get(uint_t in) const
    {
      return Lookup[in];
    }
  private:
    const VolTable* Table;
    boost::array<MultiSample, 1 << 3 * BITS_PER_LEVEL> Lookup;
  };

  class Renderer
  {
  public:
    virtual ~Renderer() {}

    virtual void Render(const Stamp& tillTime, Receiver& target) = 0;
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

    virtual void Render(const Stamp& tillTime, Receiver& target)
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
    void RenderNextSample(Receiver& target)
    {
      const uint_t psgLevel = PSG.GetLevels();
      const MultiSample& sndLevel = Table.Get(psgLevel);
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

    virtual void Render(const Stamp& tillTime, Receiver& target)
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
    void RenderNextSample(Receiver& target)
    {
      const uint_t psgLevel = PSG.GetLevels();
      const MultiSample& curLevel = Table.Get(psgLevel);
      const MultiSample& sndLevel = Interpolate(curLevel);
      target.ApplyData(sndLevel);
      Clock.NextSample();
    }

    MultiSample Interpolate(const MultiSample& newLevel)
    {
      const MultiSample out =
      {{
        Average(PrevLevel[0], newLevel[0]),
        Average(PrevLevel[1], newLevel[1]),
        Average(PrevLevel[2], newLevel[2]),
      }};
      PrevLevel = newLevel;
      return out;
    }
  private:
    ClockSource& Clock;
    AYMRenderer& PSG;
    const MultiVolumeTable& Table;
    MultiSample PrevLevel;
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
        CalculateFilter(clockFreq, soundFreq);
      }
    }

    virtual void Render(const Stamp& tillTime, Receiver& target)
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
    typedef int_t CoeffType;
    typedef boost::array<CoeffType, CHANNELS> MultiCoeff;

    static const uint_t PRECISION = 16384;

    static CoeffType MakeFixed(float f, CoeffType multiply)
    {
      return static_cast<CoeffType>(f * multiply);
    }

    void CalculateFilter(uint64_t clockFreq, uint_t soundFreq)
    {
      /*
        A0 * Y[i] = B0 * X[i] + B1 * X[i-1] + B2 * X[i-2] - A1 * Y[i-1] - A2 * Y[i-2]

        For LPF filter:
          d = cutoffFrq / sampleRate
          w0 = 2 * PI * d
          alpha = sin(w0) / 2;

          A0 = 1 + alpha
          A1 = -2 * cos(w0) (always < 0)
          A2 = (1 - alpha)

          B0 = B2 = (1 - cos(w0))/2
          B1 = 2B0

          Y[i] = A(X[i] + 2X[i-1] + X[i-2]) + BY[i-1] - CY[i-2]

          SIN = sin(w0)
          COS = cos(w0)

          A = B0/A0
          B = A1/A0
          C = A2/A0

          http://we.easyelectronics.ru/Theory/chestno-prostoy-cifrovoy-filtr.html
          http://howtodoit.com.ua/tsifrovoy-bih-filtr-iir-filter/
          http://www.ece.uah.edu/~jovanov/CPE621/notes/msp430_filter.pdf
      */
      const uint_t sampleFreq = clockFreq;
      const uint_t cutOffFreq = soundFreq / 4;
      const float w0 = 3.14159265358f * 2.0f * cutOffFreq / sampleFreq;
      const float q = 1.0f;
      //specify gain to avoid overload
      const float gain = 0.98f;

      const float sinus = sin(w0);
      const float cosine = cos(w0);
      const float alpha = sinus / (2.0f * q);

      const float a0 = (1.0f + alpha) / gain;
      const float a1 = -2.0f * cosine;
      const float a2 = 1.0f - alpha;
      const float b0 = (1.0f - cosine) / 2.0f;

      const float a = b0 / a0;
      const float b = -a1 / a0;
      const float c = a2 / a0;

      //1 bit- floor rounding
      //2 bits- scale for A (4)
      DCShift = Math::Log2(static_cast<uint_t>(1.0f / a)) - 3;

      A = MakeFixed(a, PRECISION << DCShift);
      B = MakeFixed(b, PRECISION);
      C = MakeFixed(c, PRECISION);

      In_1 = In_2 = MultiSample();
      Out_1 = Out_2 = MultiCoeff();

      ClockFreq = clockFreq;
      SoundFreq = soundFreq;
    }

    void RenderTicks(uint_t ticksPassed)
    {
      while (ticksPassed--)
      {
        const uint_t psgLevel = PSG.GetLevels();
        const MultiSample& curLevel = Table.Get(psgLevel);
        FeedFilter(curLevel);
        PSG.Tick(1);
      }
    }

    void FeedFilter(const MultiSample& in)
    {
      MultiSample out;
      for (std::size_t chan = 0; chan != CHANNELS; ++chan)
      {
        const uint_t inSum = uint_t(in[chan]) + 2 * In_1[chan] + In_2[chan];
        CoeffType sum = A * inSum >> DCShift;
        sum += B * Out_1[chan];
        sum -= C * Out_2[chan];
        out[chan] = sum / PRECISION;
      }
      Out_2 = Out_1;
      Out_1 = out;
      In_2 = In_1;
      In_1 = in;
    }

    void RenderNextSample(Receiver& target)
    {
      target.ApplyData(Out_1);
      Clock.NextSample();
    }
  private:
    ClockSource& Clock;
    AYMRenderer& PSG;
    const MultiVolumeTable& Table;
    uint64_t ClockFreq;
    uint_t SoundFreq;
    MultiSample In_1, In_2;
    MultiSample Out_1, Out_2;
    uint_t DCShift;
    CoeffType A, B, C;
  };

  class RegularAYMChip : public Chip
  {
  public:
    RegularAYMChip(ChipParameters::Ptr params, Receiver::Ptr target)
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
      if (LAYOUT_ABC == layout)
      {
        RenderChunks(source, *Target);
      }
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
      VolTable.Set(Params->VolumeTable());
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

    void RenderChunks(Renderer& source, Receiver& target)
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
    const Receiver::Ptr Target;
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
    const VolTable& GetAY38910VolTable()
    {
      return AYVolumeTab;
    }

    const VolTable& GetYM2149FVolTable()
    {
      return YMVolumeTab;
    }

    Chip::Ptr CreateChip(ChipParameters::Ptr params, Receiver::Ptr target)
    {
      return Chip::Ptr(new RegularAYMChip(params, target));
    }
  }
}
