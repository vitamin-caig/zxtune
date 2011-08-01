/*
Abstract:
  AY/YM chips interface implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  Based on sources of UnrealSpeccy by SMT
*/

//local includes
#include <devices/aym.h>
//common includes
#include <time_tools.h>
//std includes
#include <cassert>
#include <functional>
#include <limits>
#include <memory>
//boost includes
#include <boost/bind.hpp>
#include <boost/function.hpp>
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

  typedef boost::array<Sample, 32> VolumeTable;

  // chip-specific volume tables- ym supports 32 volume steps, ay - only 16
  const VolumeTable AYVolumeTab =
  { {
    0x0000, 0x0000, 0x0340, 0x0340, 0x04C0, 0x04C0, 0x06F2, 0x06F2,
    0x0A44, 0x0A44, 0x0F13, 0x0F13, 0x1510, 0x1510, 0x227E, 0x227E,
    0x289F, 0x289F, 0x414E, 0x414E, 0x5B21, 0x5B21, 0x7258, 0x7258,
    0x905E, 0x905E, 0xB550, 0xB550, 0xD7A0, 0xD7A0, 0xFFFF, 0xFFFF
  } };
  const VolumeTable YMVolumeTab =
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

  const uint_t MAX_DUTYCYCLE = 100;
  const uint_t NO_DUTYCYCLE = MAX_DUTYCYCLE / 2;

  //PSG-related functionality
  class BaseGenerator
  {
  public:
    BaseGenerator()
      : Counter()
      , Flipped()
      , Masked(true)
    {
    }

    void Reset()
    {
      Counter = 0;
      Flipped = false;
      Masked = true;
    }

    void Tick(uint_t period)
    {
      if (++Counter >= period)
      {
        Counter = 0;
        Flipped = !Flipped;
      }
    }

    uint_t GetCounter() const
    {
      return Counter;
    }

    void ResetCounter()
    {
      Counter = 0;
    }

    bool GetFlip() const
    {
      return Flipped;
    }

    bool IsMasked() const
    {
      return Masked;
    }

    void SetMask(bool masked)
    {
      Masked = masked;
    }
  private:
    uint_t Counter;
    bool Flipped;
    bool Masked;
  };

  class DutedGenerator : public BaseGenerator
  {
  public:
    DutedGenerator()
      : DutyCycle(NO_DUTYCYCLE)
      , FullPeriod(), FirstHalfPeriod(), SecondHalfPeriod()
    {
    }

    void Reset()
    {
      SetPeriod(0);
      BaseGenerator::Reset();
    }

    /*
    Perform square pulse width according to current state and duty parameter
    e.g. if duty == 25
      _     _
     | |   | |
    _| |___| |_

     |1| 3 |1| ...

    positive pulse width is 1/4 of period value
    */
    void Tick()
    {
      BaseGenerator::Tick(GetFlip() ? FirstHalfPeriod : SecondHalfPeriod);
    }

    void SetDutyCycle(uint_t dutyCycle)
    {
      assert(dutyCycle > 0 && dutyCycle < MAX_DUTYCYCLE);
      DutyCycle = dutyCycle;
      UpdateHalfPeriods();
    }

    void SetPeriod(uint_t period)
    {
      FullPeriod = period * 2;
      UpdateHalfPeriods();
    }
  private:
    void UpdateHalfPeriods()
    {
      FirstHalfPeriod = DutyCycle * FullPeriod / MAX_DUTYCYCLE;
      SecondHalfPeriod = FullPeriod - FirstHalfPeriod;
    }
  private:
    uint_t DutyCycle;
    uint_t FullPeriod;
    uint_t FirstHalfPeriod;
    uint_t SecondHalfPeriod;
  };

  class SimpleGenerator : public BaseGenerator
  {
  public:
    SimpleGenerator()
      : Period()
    {
    }

    void Reset()
    {
      SetPeriod(0);
      BaseGenerator::Reset();
    }

    void Tick()
    {
      BaseGenerator::Tick(Period);
    }

    void SetPeriod(uint_t period)
    {
      Period = period;
    }

    void SetDutyCycle(uint_t /*dutyCycle*/)
    {
    }
  private:
    uint_t Period;
  };
  
  template<class Generator>
  class ToneGenerator : public Generator
  {
  public:
    bool GetLevel() const
    {
      return Generator::IsMasked() || Generator::GetFlip();
    }
  };

  template<class Generator>
  class NoiseGenerator : public Generator
  {
  public:
    NoiseGenerator()
      : Seed()
    {
    }

    void Reset()
    {
      Seed = 0;
      Generator::Reset();
    }

    void Tick()
    {
      Generator::Tick();
      if (!Generator::GetCounter())
      {
        Seed = (Seed * 2 + 1) ^ (((Seed >> 16) ^ (Seed >> 13)) & 1);
      }
    }

    bool GetLevel() const
    {
      return 0 != (Seed & 0x10000);
    }
  private:
    uint_t Seed;
  };

  template<class Generator>
  class EnvelopeGenerator : public Generator
  {
  public:
    EnvelopeGenerator()
      : Type()
      , Level()
      , Decay()
    {
    }

    void Reset()
    {
      Type = 0;
      Level = 0;
      Decay = 0;
      Generator::Reset();
    }

    void Tick()
    {
      Generator::Tick();
      if (Generator::GetCounter() || !Decay)
      {
        return;
      }
      Level += Decay;
      if (0 == (Level & ~31u))
      {
        return;
      }
      const uint_t envTypeMask = 1 << Type;
      if (envTypeMask & ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) |
                         (1 << 5) | (1 << 6) | (1 << 7) | (1 << 9) | (1 << 15)))
      {
        Level = Decay = 0;
      }
      else if (envTypeMask & ((1 << 8) | (1 << 12)))
      {
        Level &= 31;
      }
      else if (envTypeMask & ((1 << 10) | (1 << 14)))
      {
        Decay = -Decay;
        Level += Decay;
      }
      else
      {
        Level = 31;
        Decay = 0; //11, 13
      }
    }

    void SetType(uint_t type)
    {
      Generator::ResetCounter();
      Type = type;
      if (Type & 4) //down-up envelopes
      {
        Level = 0;
        Decay = 1;
      }
      else //up-down envelopes
      {
        Level = 31;
        Decay = -1;
      }
    }

    uint_t GetLevel() const
    {
      return Level;
    }
  private:
    uint_t Type;
    uint_t Level;
    int_t Decay;
  };

  class Renderer
  {
  public:
    virtual ~Renderer() {}

    virtual void Reset() = 0;
    virtual void SetNewData(const DataChunk& data) = 0;
    virtual void Tick() = 0;
    virtual bool HasLevelChanges() const = 0;
    virtual void GetLevels(MultiSample& result) const = 0;
  };

  class AYMDevice
  {
  public:
    virtual ~AYMDevice() {}

    virtual void SetType(bool isYM) = 0;
    virtual void SetDutyCycle(uint_t value, uint_t mask) = 0;
    virtual void SetMixer(uint_t mixer) = 0;
    virtual void SetPeriods(uint_t toneA, uint_t toneB, uint_t toneC, uint_t toneN, uint_t toneE) = 0;
    virtual void SetEnvType(uint_t type) = 0;
    virtual void SetLevel(uint_t levelA, uint_t levelB, uint_t levelC) = 0;
    virtual void Reset() = 0;
    virtual void Tick() = 0;
    virtual bool HasLevelChanges() const = 0;
    virtual void GetLevels(MultiSample& result) const = 0;
  };

  template<class Generator>
  class AYMBaseDevice : public AYMDevice
  {
  public:
    AYMBaseDevice()
      : LevelA(), LevelB(), LevelC()
      , MaskNoiseA(true), MaskNoiseB(true), MaskNoiseC(true)
      , MaskEnvA(true), MaskEnvB(true), MaskEnvC(true)
      , VolTable(&AYVolumeTab)
    {
    }

    virtual void SetType(bool isYM)
    {
      VolTable = isYM ? &YMVolumeTab : &AYVolumeTab;
    }

    virtual void SetDutyCycle(uint_t value, uint_t mask)
    {
      GenA.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_A) ? value : NO_DUTYCYCLE);
      GenB.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_B) ? value : NO_DUTYCYCLE);
      GenC.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_C) ? value : NO_DUTYCYCLE);
      GenN.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_N) ? value : NO_DUTYCYCLE);
      GenE.SetDutyCycle(0 != (mask & DataChunk::CHANNEL_MASK_E) ? value : NO_DUTYCYCLE);
    }

    virtual void SetMixer(uint_t mixer)
    {
      GenA.SetMask(0 != (mixer & DataChunk::REG_MASK_TONEA));
      GenB.SetMask(0 != (mixer & DataChunk::REG_MASK_TONEB));
      GenC.SetMask(0 != (mixer & DataChunk::REG_MASK_TONEC));
      MaskNoiseA = 0 != (mixer & DataChunk::REG_MASK_NOISEA);
      MaskNoiseB = 0 != (mixer & DataChunk::REG_MASK_NOISEB);
      MaskNoiseC = 0 != (mixer & DataChunk::REG_MASK_NOISEC);
      GenN.SetMask(MaskNoiseA && MaskNoiseB && MaskNoiseC);
    }

    virtual void SetPeriods(uint_t toneA, uint_t toneB, uint_t toneC, uint_t toneN, uint_t toneE)
    {
      GenA.SetPeriod(toneA);
      GenB.SetPeriod(toneB);
      GenC.SetPeriod(toneC);
      GenN.SetPeriod(toneN);
      GenE.SetPeriod(toneE);
    }

    virtual void SetEnvType(uint_t type)
    {
      GenE.SetType(type);
    }

    virtual void SetLevel(uint_t levelA, uint_t levelB, uint_t levelC)
    {
      MaskEnvA = 0 == (levelA & DataChunk::REG_MASK_ENV);
      MaskEnvB = 0 == (levelB & DataChunk::REG_MASK_ENV);
      MaskEnvC = 0 == (levelC & DataChunk::REG_MASK_ENV);
      GenE.SetMask(MaskEnvA && MaskEnvB && MaskEnvC);
      LevelA = ((levelA & DataChunk::REG_MASK_VOL) << 1) + 1;
      LevelB = ((levelB & DataChunk::REG_MASK_VOL) << 1) + 1;
      LevelC = ((levelC & DataChunk::REG_MASK_VOL) << 1) + 1;
    }

    virtual void Reset()
    {
      GenA.Reset();
      GenB.Reset();
      GenC.Reset();
      GenN.Reset();
      GenE.Reset();
      LevelA = LevelB = LevelC = 0;
      MaskNoiseA = MaskNoiseB = MaskNoiseC = true;
      MaskEnvA = MaskEnvB = MaskEnvC = true;
      VolTable = &AYVolumeTab;
    }

    virtual void Tick()
    {
      GenA.Tick();
      GenB.Tick();
      GenC.Tick();
      GenN.Tick();
      GenE.Tick();
    }

    virtual bool HasLevelChanges() const
    {
      return (!GenA.IsMasked() && !GenA.GetCounter())
          || (!GenB.IsMasked() && !GenB.GetCounter())
          || (!GenC.IsMasked() && !GenC.GetCounter())
          || (!GenN.IsMasked() && !GenN.GetCounter())
          || (!GenE.IsMasked() && !GenE.GetCounter())
      ;
    }

    virtual void GetLevels(MultiSample& result) const
    {
      const bool triggeredNoise = GenN.GetLevel();
      const uint_t envelope = GenE.GetLevel();
      //references to mixered bits. updated automatically
      const bool maskedNoiseA = triggeredNoise || MaskNoiseA;
      const uint_t outA = MaskEnvA ? LevelA : envelope;
      const bool maskedNoiseB = triggeredNoise || MaskNoiseB;
      const uint_t outB = MaskEnvB ? LevelB : envelope;
      const bool maskedNoiseC = triggeredNoise || MaskNoiseC;
      const uint_t outC = MaskEnvC ? LevelC : envelope;

      const VolumeTable& table = *VolTable;
      assert(outA < 32 && outB < 32 && outC < 32);
      result[0] = maskedNoiseA && GenA.GetLevel() ? table[outA] : 0;
      result[1] = maskedNoiseB && GenB.GetLevel() ? table[outB] : 0;
      result[2] = maskedNoiseC && GenC.GetLevel() ? table[outC] : 0;
    }
  private:
    ToneGenerator<Generator> GenA;
    ToneGenerator<Generator> GenB;
    ToneGenerator<Generator> GenC;
    NoiseGenerator<Generator> GenN;
    EnvelopeGenerator<Generator> GenE;
    uint_t LevelA;
    uint_t LevelB;
    uint_t LevelC;
    bool MaskNoiseA;
    bool MaskNoiseB;
    bool MaskNoiseC;
    bool MaskEnvA;
    bool MaskEnvB;
    bool MaskEnvC;
    const VolumeTable* VolTable;
  };

  class AYMRenderer : public Renderer
  {
  public:
    AYMRenderer()
      : Mixer(State.Data[DataChunk::REG_MIXER])
      , VolA(State.Data[DataChunk::REG_VOLA]), VolB(State.Data[DataChunk::REG_VOLB]), VolC(State.Data[DataChunk::REG_VOLC])
      , IsDuted(false)
      , Device(new AYMBaseDevice<SimpleGenerator>())
    {
      Mixer = 0xff;
    }

    void SetType(bool isYM)
    {
      Device->SetType(isYM);
    }

    void SetDutyCycle(uint_t value, uint_t mask)
    {
      const bool newDuted = mask && value != NO_DUTYCYCLE;
      if (newDuted != IsDuted)
      {
        if (newDuted)
        {
          Device.reset(new AYMBaseDevice<DutedGenerator>());
        }
        else
        {
          Device.reset(new AYMBaseDevice<SimpleGenerator>());
        }
        IsDuted = newDuted;
      }
      Device->SetDutyCycle(value, mask);
    }

    virtual void Reset()
    {
      State = DataChunk();
      Mixer = 0xff;
      Device->Reset();
    }

    virtual void SetNewData(const DataChunk& data)
    {
      for (uint_t idx = 0, mask = 1; idx != data.Data.size(); ++idx, mask <<= 1)
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
        State.Data[idx] = reg;
      }
      if (data.Mask & (1 << DataChunk::REG_MIXER))
      {
        Device->SetMixer(Mixer);
      }
      if (data.Mask & ((1 << DataChunk::REG_TONEA_L) | (1 << DataChunk::REG_TONEA_H) |
                       (1 << DataChunk::REG_TONEB_L) | (1 << DataChunk::REG_TONEB_H) |
                       (1 << DataChunk::REG_TONEC_L) | (1 << DataChunk::REG_TONEC_H) |
                       (1 << DataChunk::REG_TONEN) |
                       (1 << DataChunk::REG_TONEE_L) | (1 << DataChunk::REG_TONEE_H)
                       ))
      {
        Device->SetPeriods(GetToneA(), GetToneB(), GetToneC(), GetToneN(), GetToneE());
      }
      if (data.Mask & (1 << DataChunk::REG_ENV))
      {
        Device->SetEnvType(GetEnvType());
      }
      if (data.Mask & ((1 << DataChunk::REG_VOLA) | (1 << DataChunk::REG_VOLB) | (1 << DataChunk::REG_VOLC)))
      {
        Device->SetLevel(VolA, VolB, VolC);
      }
    }

    virtual void Tick()
    {
      Device->Tick();
    }

    virtual bool HasLevelChanges() const
    {
      return Device->HasLevelChanges();
    }

    virtual void GetLevels(MultiSample& result) const
    {
      return Device->GetLevels(result);
    }

    void GetState(uint64_t ticksPerSec, ChannelsState& state) const
    {
      const uint_t MAX_LEVEL = 100;
      //one channel is noise
      ChanState& noiseChan = state[CHANNELS];
      noiseChan = ChanState('N');
      noiseChan.Band = GetBandByPeriod(ticksPerSec, GetToneN());
      //one channel is envelope    
      ChanState& envChan = state[CHANNELS + 1];
      envChan = ChanState('E');
      envChan.Band = GetBandByPeriod(ticksPerSec, 16 * GetToneE());
      const uint_t mixer = ~Mixer;
      for (uint_t chan = 0; chan != CHANNELS; ++chan) 
      {
        const uint_t volReg = State.Data[DataChunk::REG_VOLA + chan];
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
        if (hasEnv)
        {        
          envChan.Enabled = true;
          envChan.LevelInPercents += MAX_LEVEL / CHANNELS;
        }
        //calculate tone channel
        ChanState& channel = state[chan];
        channel.Name = 'A' + chan;
        if (hasTone)
        {
          channel.Enabled = true;
          channel.LevelInPercents = (volReg & DataChunk::REG_MASK_VOL) * MAX_LEVEL / 15;
          const uint_t chanTone = 256 * State.Data[DataChunk::REG_TONEA_H + chan * 2] +
            State.Data[DataChunk::REG_TONEA_L + chan * 2];
          channel.Band = GetBandByPeriod(ticksPerSec, chanTone);
        }
      } 
    }

  private:
    static uint_t GetBandByPeriod(uint64_t ticksPerSec, uint_t period)
    {
      const uint_t FREQ_MULTIPLIER = 100;
      //table in Hz * FREQ_MULTIPLIER
      static const uint_t FREQ_TABLE[] =
      {
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
      };
      const uint_t freq = static_cast<uint_t>(ticksPerSec * FREQ_MULTIPLIER / (2 * AYM_CLOCK_DIVISOR * (period ? period : 1)));
      const uint_t maxBand = static_cast<uint_t>(ArraySize(FREQ_TABLE) - 1);
      const uint_t currentBand = static_cast<uint_t>(std::lower_bound(FREQ_TABLE, ArrayEnd(FREQ_TABLE), freq) - FREQ_TABLE);
      return std::min(currentBand, maxBand);
    }
  private:
    uint_t GetToneA() const
    {
      return 256 * State.Data[DataChunk::REG_TONEA_H] + State.Data[DataChunk::REG_TONEA_L];
    }

    uint_t GetToneB() const
    {
      return 256 * State.Data[DataChunk::REG_TONEB_H] + State.Data[DataChunk::REG_TONEB_L];
    }

    uint_t GetToneC() const
    {
      return 256 * State.Data[DataChunk::REG_TONEC_H] + State.Data[DataChunk::REG_TONEC_L];
    }

    uint_t GetToneN() const
    {
      return 2 * State.Data[DataChunk::REG_TONEN];//for optimization
    }

    uint_t GetToneE() const
    {
      return 256 * State.Data[DataChunk::REG_TONEE_H] + State.Data[DataChunk::REG_TONEE_L];
    }

    uint_t GetEnvType() const
    {
      return State.Data[DataChunk::REG_ENV];
    }
  private:
    //registers state
    DataChunk State;
    //aliases for registers
    uint8_t& Mixer;
    uint8_t& VolA;
    uint8_t& VolB;
    uint8_t& VolC;
    //device
    bool IsDuted;
    boost::scoped_ptr<AYMDevice> Device;
  };

  class BeeperRenderer : public Renderer
  {
  public:
    BeeperRenderer()
      : Beeper()
    {
    }

    virtual void Reset()
    {
      Beeper = 0;
    }

    virtual void SetNewData(const DataChunk& data)
    {
      if (data.Mask & (1 << DataChunk::REG_BEEPER))
      {
        const uint_t inLevel = ((data.Data[DataChunk::REG_BEEPER] & DataChunk::REG_MASK_VOL) << 1) + 1;
        Beeper = AYVolumeTab[inLevel];
      }
    }

    virtual void Tick()
    {
    }

    virtual bool HasLevelChanges() const
    {
      return false;
    }

    virtual void GetLevels(MultiSample& result) const
    {
      const uint_t levelBeeper = Beeper;
      std::fill(result.begin(), result.end(), levelBeeper);
    }
  private:
    uint_t Beeper;
  };

  class MixedRenderer : public Renderer
  {
  public:
    MixedRenderer(Renderer& first, Renderer& second)
      : First(first)
      , Second(second)
    {
    }

    virtual void Reset()
    {
      First.Reset();
      Second.Reset();
    }

    virtual void SetNewData(const DataChunk& data)
    {
      First.SetNewData(data);
      Second.SetNewData(data);
    }

    virtual void Tick()
    {
      First.Tick();
      Second.Tick();
    }

    virtual bool HasLevelChanges() const
    {
      return First.HasLevelChanges() || Second.HasLevelChanges();
    }

    virtual void GetLevels(MultiSample& result) const
    {
      MultiSample firstResult;
      MultiSample secondResult;
      First.GetLevels(firstResult);
      Second.GetLevels(secondResult);
      std::transform(firstResult.begin(), firstResult.end(), secondResult.begin(), result.begin(), &Average);
    }
  private:
    static Sample Average(Sample first, Sample second)
    {
      return static_cast<Sample>((uint_t(first) + second) / 2);
    }
  private:
    Renderer& First;
    Renderer& Second;
  };

  class InterpolatedRenderer : public Renderer
  {
  public:
    InterpolatedRenderer()
      : Delegate()
      , Levels()
      , AccumulatedSamples()
    {
    }

    void SetSource(Renderer* delegate)
    {
      assert(delegate);
      Delegate = delegate;
    }

    virtual void Reset()
    {
      if (Delegate)
      {
        Delegate->Reset();
      }
      std::fill(Levels.begin(), Levels.end(), 0);
      AccumulatedSamples = 0;
    }

    virtual void SetNewData(const DataChunk &data)
    {
      Delegate->SetNewData(data);
      Delegate->GetLevels(Levels);
    }

    virtual void Tick()
    {
      std::transform(Accumulators.begin(), Accumulators.end(), Levels.begin(), Accumulators.begin(), std::plus<uint_t>());
      ++AccumulatedSamples;
      Delegate->Tick();
      if (Delegate->HasLevelChanges())
      {
        Delegate->GetLevels(Levels);
      }
    }

    bool HasLevelChanges() const
    {
      return true;
    }

    virtual void GetLevels(MultiSample& result) const
    {
      std::transform(Accumulators.begin(), Accumulators.end(), result.begin(), std::bind2nd(std::divides<uint_t>(), AccumulatedSamples));
      std::fill(Accumulators.begin(), Accumulators.end(), 0);
      AccumulatedSamples = 0;
    }
  private:
    Renderer* Delegate;
    MultiSample Levels;
    mutable boost::array<uint_t, CHANNELS> Accumulators;
    mutable uint_t AccumulatedSamples;
  };

  class RelayoutRenderer : public Renderer
  {
  public:
    RelayoutRenderer(Renderer& delegate, LayoutType layout)
      : Delegate(delegate)
      , Layout(LAYOUTS[layout])
    {
    }

    virtual void Reset()
    {
      return Delegate.Reset();
    }

    virtual void SetNewData(const DataChunk &data)
    {
      return Delegate.SetNewData(data);
    }

    virtual void Tick()
    {
      return Delegate.Tick();
    }

    virtual bool HasLevelChanges() const
    {
      return Delegate.HasLevelChanges();
    }

    virtual void GetLevels(MultiSample& result) const
    {
      MultiSample tmp;
      Delegate.GetLevels(tmp);
      for (uint_t idx = 0; idx < Devices::AYM::CHANNELS; ++idx)
      {
        const uint_t chipChannel = Layout[idx];
        result[idx] = tmp[chipChannel];
      }
    }
  private:
    Renderer& Delegate;
    const LayoutData Layout;
  };

  class ClockSource
  {
  public:
    ClockSource()
      : LastTick()
      , NextSoundTick()
    {
    }

    void SetFrequency(uint64_t clockFreq, uint_t soundFreq)
    {
      PsgOscillator.SetFrequency(clockFreq);
      SndOscillator.SetFrequency(soundFreq);
      UpdateSoundTick();
    }

    void Reset()
    {
      LastTick = 0;
      PsgOscillator.Reset();
      SndOscillator.Reset();
      NextSoundTick = 0;
    }

    bool HasOutputBefore(const Time::Nanoseconds& time) const
    {
      return SndOscillator.GetCurrentTime() < time;
    }

    void SetFrameEnd(const Time::Nanoseconds& time)
    {
      LastTick = PsgOscillator.GetTickAtTime(time);
    }

    bool Tick()
    {
      PsgOscillator.AdvanceTick(AYM_CLOCK_DIVISOR);
      if (PsgOscillator.GetCurrentTick() > NextSoundTick)
      {
        SndOscillator.AdvanceTick(1);
        UpdateSoundTick();
        assert(PsgOscillator.GetCurrentTick() < NextSoundTick);
        return true;
      }
      return false;
    }

    bool InFrame() const
    {
      return PsgOscillator.GetCurrentTick() < LastTick;
    }
  private:
    void UpdateSoundTick()
    {
      const Time::Nanoseconds nextSoundTime = SndOscillator.GetCurrentTime();
      NextSoundTick = PsgOscillator.GetTickAtTime(nextSoundTime); 
    }
  private:
    uint64_t LastTick;
    Time::NanosecOscillator PsgOscillator;
    Time::NanosecOscillator SndOscillator;
    uint64_t NextSoundTick;
  };

  class DataCache
  {
  public:
    void Add(const DataChunk& src)
    {
      Buffer.push_back(src);
    }

    void ForEachElement(boost::function<void(const DataChunk&)> cb)
    {
      std::for_each(Buffer.begin(), Buffer.end(), cb);
      Reset();
    };

    void Reset()
    {
      Buffer.clear();
    }

    bool HasBeeperData() const
    {
      return Buffer.end() != std::find_if(Buffer.begin(), Buffer.end(), &IsBeeperData);
    }

    bool HasPSGData() const
    {
      return Buffer.end() != std::find_if(Buffer.begin(), Buffer.end(), &IsPSGData);
    }
  private:
    static bool IsBeeperData(const DataChunk& data)
    {
      return 0 != (data.Mask & (1 << DataChunk::REG_BEEPER));
    }

    static bool IsPSGData(const DataChunk& data)
    {
      return 0 != (data.Mask & ((1 << DataChunk::REG_BEEPER) - 1));
    }
  private:
    std::vector<DataChunk> Buffer;
  };

  class RegularAYMChip : public Chip
  {
  public:
    RegularAYMChip(ChipParameters::Ptr params, Receiver::Ptr target)
      : Params(params)
      , Target(target)
      , PSGWithBeeper(PSG, Beeper)
      , Clock()
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
      Renderer& targetDevice = GetTargetDevice();
      Filter.SetSource(&targetDevice);
      Renderer& targetRender = Params->Interpolate()
        ? static_cast<Renderer&>(Filter)
        : targetDevice;
      const LayoutType layout = Params->Layout();
      if (LAYOUT_ABC == layout)
      {
        RenderChunks(targetRender);
      }
      else
      {
        RelayoutRenderer relayoutRender(targetRender, layout);
        RenderChunks(relayoutRender);
      }
    }

    virtual void GetState(ChannelsState& state) const
    {
      return PSG.GetState(Params->ClockFreq(), state);
    }

    virtual void Reset()
    {
      Filter.Reset();
      PSGWithBeeper.Reset();
      Clock.Reset();
      BufferedData.Reset();
    }
  private:
    void ApplyParameters()
    {
      PSG.SetType(Params->IsYM());
      PSG.SetDutyCycle(Params->DutyCycleValue(), Params->DutyCycleMask());
      Clock.SetFrequency(Params->ClockFreq(), Params->SoundFreq());
    }

    Renderer& GetTargetDevice()
    {
      const bool hasPSG = BufferedData.HasPSGData();
      const bool hasBeeper = BufferedData.HasBeeperData();
      return hasBeeper
      ? (hasPSG
         ? static_cast<Renderer&>(PSGWithBeeper)
         : static_cast<Renderer&>(Beeper)
         )
      : static_cast<Renderer&>(PSG);
    }

    void RenderChunks(Renderer& render)
    {
      BufferedData.ForEachElement(boost::bind(&RegularAYMChip::RenderSingleChunk, this, _1, boost::ref(render)));
    }

    void RenderSingleChunk(const DataChunk& src, Renderer& render)
    {
      render.SetNewData(src);
      if (!Clock.HasOutputBefore(src.TimeStamp))
      {
        return;
      }
      Clock.SetFrameEnd(src.TimeStamp);
      MultiSample result;
      while (Clock.InFrame())
      {
        render.Tick();
        if (Clock.Tick())
        {
          render.GetLevels(result);
          Target->ApplyData(result);
        }
      }
    }
  private:
    const ChipParameters::Ptr Params;
    const Receiver::Ptr Target;
    AYMRenderer PSG;
    BeeperRenderer Beeper;
    MixedRenderer PSGWithBeeper;
    InterpolatedRenderer Filter;
    ClockSource Clock;
    DataCache BufferedData;
  };
}

namespace Devices
{
  namespace AYM
  {
    Chip::Ptr CreateChip(ChipParameters::Ptr params, Receiver::Ptr target)
    {
      return Chip::Ptr(new RegularAYMChip(params, target));
    }
  }
}
