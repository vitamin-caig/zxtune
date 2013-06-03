/*
Abstract:
  SAA chips interface implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  Based on sources of PerfectZX emulator
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

namespace
{
  using namespace Devices::SAA;

  BOOST_STATIC_ASSERT(DataChunk::REG_LAST <= 8 * sizeof(uint_t));

  class SAARenderer
  {
  public:
    void Reset()
    {
      Device.Reset();
    }

    void SetNewData(const DataChunk& data)
    {
      for (uint_t idx = 0, mask = 1; idx != data.Data.size(); ++idx, mask <<= 1)
      {
        if (0 == (data.Mask & mask))
        {
          //no new data
          continue;
        }
        const uint_t val = data.Data[idx];
        switch (idx)
        {
        case DataChunk::REG_LEVEL0:
        case DataChunk::REG_LEVEL1:
        case DataChunk::REG_LEVEL2:
        case DataChunk::REG_LEVEL3:
        case DataChunk::REG_LEVEL4:
        case DataChunk::REG_LEVEL5:
          Device.SetLevel(idx - DataChunk::REG_LEVEL0, LoNibble(val), HiNibble(val));
          break;
        case DataChunk::REG_TONENUMBER0:
        case DataChunk::REG_TONENUMBER1:
        case DataChunk::REG_TONENUMBER2:
        case DataChunk::REG_TONENUMBER3:
        case DataChunk::REG_TONENUMBER4:
        case DataChunk::REG_TONENUMBER5:
          Device.SetToneNumber(idx - DataChunk::REG_TONENUMBER0, val);
          break;
        case DataChunk::REG_TONEOCTAVE01:
          Device.SetToneOctave(0, LoNibble(val));
          Device.SetToneOctave(1, HiNibble(val));
          break;
        case DataChunk::REG_TONEOCTAVE23:
          Device.SetToneOctave(2, LoNibble(val));
          Device.SetToneOctave(3, HiNibble(val));
          break;
        case DataChunk::REG_TONEOCTAVE45:
          Device.SetToneOctave(4, LoNibble(val));
          Device.SetToneOctave(5, HiNibble(val));
          break;
        case DataChunk::REG_TONEMIXER:
          Device.SetToneMixer(val);
          break;
        case DataChunk::REG_NOISEMIXER:
          Device.SetNoiseMixer(val);
          break;
        case DataChunk::REG_NOISECLOCK:
          Device.SetNoiseControl(val);
          break;
        case DataChunk::REG_ENVELOPE0:
        case DataChunk::REG_ENVELOPE1:
          Device.SetEnvelope(idx - DataChunk::REG_ENVELOPE0, val);
          break;
        }
      }
    }

    void Tick(uint_t ticks)
    {
      Device.Tick(ticks);
    }

    Sound::Sample GetLevels() const
    {
      return Device.GetLevels();
    }

    void GetState(ChannelsState& state) const
    {
      for (uint_t chan = 0; chan != state.size(); ++chan)
      {
        state[chan] = ChanState(static_cast<Char>('A' + chan));
      }
      Device.GetState(state);
    }
  private:
    SAADevice Device;
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
    LQRenderer(ClockSource& clock, SAARenderer& psg)
      : Clock(clock)
      , PSG(psg)
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
      const Sound::Sample& sndLevel = PSG.GetLevels();
      target.ApplyData(sndLevel);
      Clock.NextSample();
    }
  private:
    ClockSource& Clock;
    SAARenderer& PSG;
  };

  /*
    Simple decimation with post simple FIR filter (0.5, 0.5)
  */
  class MQRenderer : public Renderer
  {
  public:
    MQRenderer(ClockSource& clock, SAARenderer& psg)
      : Clock(clock)
      , PSG(psg)
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
      const Sound::Sample curLevel = PSG.GetLevels();
      const Sound::Sample& sndLevel = Interpolate(curLevel);
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
    SAARenderer& PSG;
    Sound::Sample PrevLevel;
  };

  class RegularSAAChip : public Chip
  {
  public:
    RegularSAAChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
      : Params(params)
      , Target(target)
      , Clock()
      , LQ(Clock, PSG)
      , MQ(Clock, PSG)
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
      RenderChunks(source);
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
    }
  private:
    void ApplyParameters()
    {
      const uint64_t clock = Params->ClockFreq();
      const uint_t sndFreq = Params->SoundFreq();
      Clock.SetFrequency(clock, sndFreq);
      Analyser.SetClockRate(clock);
    }

    Renderer& GetRenderer()
    {
      switch (Params->Interpolation())
      {
      case INTERPOLATION_LQ:
        return MQ;
      default:
        return LQ;
      }
    }

    void RenderChunks(Renderer& source)
    {
      Sound::Receiver& target = *Target;
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
    SAARenderer PSG;
    ClockSource Clock;
    DataCache BufferedData;
    AnalysisMap Analyser;
    LQRenderer LQ;
    MQRenderer MQ;
  };
}

namespace Devices
{
  namespace SAA
  {
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::Receiver::Ptr target)
    {
      return Chip::Ptr(new RegularSAAChip(params, target));
    }
  }
}
