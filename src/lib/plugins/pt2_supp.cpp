#include "plugin_enumerator.h"

#include "tracking_supp.h"
#include "../devices/data_source.h"
#include "../devices/aym/aym.h"

#include <tools.h>

#include <player_attrs.h>

#include <boost/static_assert.hpp>

#include <cassert>
#include <valarray>

namespace
{
  using namespace ZXTune;

  const String TEXT_PT2_INFO("ProTracker v2 modules support");
  const String TEXT_PT2_VERSION("0.1");
  const String TEXT_PT2_EDITOR("ProTracker v2");

  //hints
  const std::size_t MAX_PATTERN_SIZE = 64;
  const std::size_t MAX_PATTERN_COUNT = 64;//TODO

  const uint16_t FreqTable[96] = {
    0xef8, 0xe10, 0xd60, 0xc80, 0xbd8, 0xb28, 0xa88, 0x9f0, 0x960, 0x8e0, 0x858, 0x7e0,
    0x77c, 0x708, 0x6b0, 0x640, 0x5ec, 0x594, 0x544, 0x4f8, 0x4b0, 0x470, 0x42c, 0x3fd,
    0x3be, 0x384, 0x358, 0x320, 0x2f6, 0x2ca, 0x2a2, 0x27c, 0x258, 0x238, 0x216, 0x1f8,
    0x1df, 0x1c2, 0x1ac, 0x190, 0x17b, 0x165, 0x151, 0x13e, 0x12c, 0x11c, 0x10a, 0x0fc,
    0x0ef, 0x0e1, 0x0d6, 0x0c8, 0x0bd, 0x0b2, 0x0a8, 0x09f, 0x096, 0x08e, 0x085, 0x07e,
    0x077, 0x070, 0x06b, 0x064, 0x05e, 0x059, 0x054, 0x04f, 0x04b, 0x047, 0x042, 0x03f,
    0x03b, 0x038, 0x035, 0x032, 0x02f, 0x02c, 0x02a, 0x027, 0x025, 0x023, 0x021, 0x01f,
    0x01d, 0x01c, 0x01a, 0x019, 0x017, 0x016, 0x015, 0x013, 0x012, 0x011, 0x010, 0x00f
  };
  //////////////////////////////////////////////////////////////////////////
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct PT2Header
  {
    uint8_t Tempo;
    uint8_t Length;
    uint8_t Loop;
    uint16_t SamplesOffsets[32];
    uint16_t OrnamentsOffsets[16];
    uint16_t PatternsOffset;
    char Name[30];
    uint8_t Positions[1];
  } PACK_POST;

  const uint8_t POS_END_MARKER = 0xff;

  PACK_PRE struct PT2Sample
  {
    uint8_t Size;
    uint8_t Loop;
    PACK_PRE struct Line
    {
      uint8_t NoiseOff : 1;
      uint8_t ToneOff : 1;
      uint8_t VibratoSign : 1;
      uint8_t Noise : 5;
      uint8_t VibratoHi : 4;
      uint8_t Level : 4;
      uint8_t VibratoLo;
    } PACK_POST;
    Line Data[1];

    operator bool () const
    {
      return Size >= Loop;
    }
  } PACK_POST;

  PACK_PRE struct PT2Ornament
  {
    uint8_t Size;
    uint8_t Loop;
    int8_t Data[1];

    operator bool () const
    {
      return Size >= Loop;
    }
  } PACK_POST;

  PACK_PRE struct PT2Pattern
  {
    uint16_t Offsets[3];

    operator bool () const
    {
      return Offsets[0] && Offsets[1] && Offsets[2];
    }
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(PT2Header) == 132);
  BOOST_STATIC_ASSERT(sizeof(PT2Sample) == 5);
  BOOST_STATIC_ASSERT(sizeof(PT2Ornament) == 3);

  struct Sample
  {
    Sample() : Loop(), Data()
    {
    }

    Sample(std::size_t size, std::size_t loop) : Loop(loop), Data(size)
    {
    }

    std::size_t Loop;

    struct Line
    {
      unsigned Level;//0-15
      unsigned Noise;//0-31
      bool ToneOff;
      bool NoiseOff;
      signed Vibrato;
    };
    std::vector<Line> Data;
  };

  class SampleCreator : public std::unary_function<uint16_t, Sample>
  {
  public:
    SampleCreator(const Dump& data) : Data(data)
    {
    }

    SampleCreator(const SampleCreator& rh) : Data(rh.Data)
    {
    }

    result_type operator () (const argument_type arg) const
    {
      if (0 == arg)//dummy
      {
        return result_type();
      }
      const PT2Sample* const sample(safe_ptr_cast<const PT2Sample*>(&Data[fromLE(arg)]));
      result_type tmp(sample->Size, sample->Loop);
      for (std::size_t idx = 0; idx != sample->Size; ++idx)
      {
        const PT2Sample::Line& src(sample->Data[idx]);
        Sample::Line& dst(tmp.Data[idx]);
        dst.NoiseOff = src.NoiseOff;
        dst.ToneOff = src.ToneOff;
        dst.Noise = src.Noise;
        dst.Level = src.Level;
        dst.Vibrato = src.VibratoLo | (signed(src.VibratoHi) << 8);
        if (!src.VibratoSign)
        {
          dst.Vibrato = -dst.Vibrato;
        }
      }
      return tmp;
    }
  private:
    const Dump& Data;
  };

  enum CmdType
  {
    EMPTY,
    ENVELOPE,     //2p
    NOENVELOPE,   //0p
    GLISS,        //1p
    GLISS_NOTE,   //2p
    NOGLISS,      //0p
    NOISE_ADD,    //1p
  };

  uint8_t GetVolume(std::size_t volume, std::size_t level)
  {
    return uint8_t((volume * 17 + (volume > 7 ? 1 : 0)) * level >> 8);
  }

  class PlayerImpl : public Tracking::TrackPlayer<3, Sample>
  {
    typedef Tracking::TrackPlayer<3, Sample> Parent;

    class OrnamentCreator : public std::unary_function<uint16_t, Parent::Ornament>
    {
    public:
      OrnamentCreator(const Dump& data) : Data(data)
      {
      }

      OrnamentCreator(const OrnamentCreator& rh) : Data(rh.Data)
      {
      }

      result_type operator () (const argument_type arg) const
      {
        if (0 == arg)
        {
          return result_type();
        }
        const PT2Ornament* const ornament(safe_ptr_cast<const PT2Ornament*>(&Data[fromLE(arg)]));
        result_type tmp;
        tmp.Loop = ornament->Loop;
        tmp.Data.assign(ornament->Data, ornament->Data + ornament->Size);
        return tmp;
      }
    private:
      const Dump& Data;
    };

    static void ParsePattern(const Dump& data, std::vector<std::size_t>& offsets, Parent::Line& line,
      std::valarray<std::size_t>& periods,
      std::valarray<std::size_t>& counters)
    {
      for (std::size_t chan = 0; chan != line.Channels.size(); ++chan)
      {
        if (counters[chan]--)
        {
          continue;//has to skip
        }
        for (;;)
        {
          const uint8_t cmd(data[offsets[chan]++]);
          Line::Chan& channel(line.Channels[chan]);
          if (cmd >= 0xe1) //sample
          {
            assert(!channel.SampleNum);
            channel.SampleNum = cmd - 0xe0;
          }
          else if (cmd == 0xe0) //sample 0 - shut up
          {
            channel.Enabled = false;
            break;
          }
          else if (cmd >= 0x80 && cmd <= 0xdf)//note
          {
            assert(!channel.Note);
            channel.Enabled = true;
            const std::size_t note(cmd - 0x80);
            //for note gliss calculate limit manually
            CommandsArray::iterator noteGlissCmd(std::find(channel.Commands.begin(), channel.Commands.end(), GLISS_NOTE));
            if (channel.Commands.end() != noteGlissCmd)
            {
              noteGlissCmd->Param2 = int(note);
            }
            else
            {
              channel.Note = note;
            }
            break;
          }
          else if (cmd == 0x7f) //env off
          {
            channel.Commands.push_back(NOENVELOPE);
          }
          else if (cmd >= 0x71 && cmd <= 0x7e) //envelope
          {
            channel.Commands.push_back(
              Command(ENVELOPE, cmd - 0x70, data[offsets[chan]] | (unsigned(data[offsets[chan] + 1]) << 8)));
            offsets[chan] += 2;
          }
          else if (cmd == 0x70)//quit
          {
            break;
          }
          else if (cmd >= 0x60 && cmd <= 0x6f)//ornament
          {
            assert(!channel.OrnamentNum);
            channel.OrnamentNum = cmd - 0x60;
          }
          else if (cmd >= 0x20 && cmd <= 0x5f)//skip
          {
            periods[chan] = cmd - 0x20;
          }
          else if (cmd >= 0x10 && cmd <= 0x1f)//volume
          {
            assert(!channel.Volume);
            channel.Volume = cmd - 0x10;
          }
          else if (cmd == 0x0f)//new delay
          {
            assert(!line.Tempo);
            line.Tempo = data[offsets[chan]++];
          }
          else if (cmd == 0x0e)//gliss
          {
            channel.Commands.push_back(Command(GLISS, static_cast<int8_t>(data[offsets[chan]++])));
          }
          else if (cmd == 0x0d)//note gliss
          {
            //too late when note is filled
            assert(!channel.Note);
            channel.Commands.push_back(Command(GLISS_NOTE, static_cast<int8_t>(data[offsets[chan]])));
            //ignore delta due to error
            offsets[chan] += 3;
          }
          else if (cmd == 0x0c) //gliss off
          {
            channel.Commands.push_back(NOGLISS);
          }
          else //noise add
          {
            channel.Commands.push_back(Command(NOISE_ADD, static_cast<int8_t>(data[offsets[chan]++])));
          }
        }
        counters[chan] = periods[chan];
      }
    }

  private:
    struct ChannelState
    {
      ChannelState()
        : Enabled(false), Envelope(false)
        , Note(), SampleNum(0), PosInSample(0)
        , OrnamentNum(0), PosInOrnament(0)
        , Volume(15), NoiseAdd(0)
        , Sliding(0), SlidingTargetNote(~std::size_t(0)), Glissade(0)
      {
      }
      bool Enabled;
      bool Envelope;
      std::size_t Note;
      std::size_t SampleNum;
      std::size_t PosInSample;
      std::size_t OrnamentNum;
      std::size_t PosInOrnament;
      std::size_t Volume;
      signed NoiseAdd;
      signed Sliding;
      std::size_t SlidingTargetNote;
      signed Glissade;
    };
  public:
    PlayerImpl(const String& filename, const Dump& data)
      : Device(AYM::CreateChip())//TODO: put out
    {
      //assume all data is correct
      const PT2Header* const header(safe_ptr_cast<const PT2Header*>(&data[0]));
      Information.Statistic.Tempo = header->Tempo;
      Information.Statistic.Position = header->Length;
      Information.Loop = header->Loop;
      Information.Properties.insert(StringMap::value_type(Module::ATTR_FILENAME, filename));
      Information.Properties.insert(StringMap::value_type(Module::ATTR_TITLE, String(header->Name, ArrayEnd(header->Name))));
      Information.Properties.insert(StringMap::value_type(Module::ATTR_PROGRAM, TEXT_PT2_EDITOR));

      //fill samples
      std::transform(header->SamplesOffsets, ArrayEnd(header->SamplesOffsets),
        std::back_inserter(Data.Samples), SampleCreator(data));
      //fill ornaments
      std::transform(header->OrnamentsOffsets, ArrayEnd(header->OrnamentsOffsets),
        std::back_inserter(Data.Ornaments), OrnamentCreator(data));
      //fill order
      Data.Positions.assign(header->Positions,
        std::find(header->Positions, header->Positions + header->Length, POS_END_MARKER));
      assert(header->Length == Data.Positions.size());

      //fill patterns
      Data.Patterns.reserve(MAX_PATTERN_COUNT);
      for (const PT2Pattern* patPos(safe_ptr_cast<const PT2Pattern*>(&data[fromLE(header->PatternsOffset)]));
        *patPos;
        ++patPos)
      {
        Data.Patterns.push_back(Pattern());
        Pattern& pat(Data.Patterns.back());
        std::vector<std::size_t> offsets(ArraySize(patPos->Offsets));
        std::valarray<std::size_t> periods(std::size_t(0), ArraySize(patPos->Offsets));
        std::valarray<std::size_t> counters(std::size_t(0), ArraySize(patPos->Offsets));
        std::transform(patPos->Offsets, ArrayEnd(patPos->Offsets), offsets.begin(), &fromLE<uint16_t>);
        pat.reserve(MAX_PATTERN_SIZE);
        do
        {
          pat.push_back(Line());
          Line& line(pat.back());
          ParsePattern(data, offsets, line, periods, counters);
          //skip lines
          if (const std::size_t linesToSkip = counters.min())
          {
            counters -= linesToSkip;
            pat.resize(pat.size() + linesToSkip);//add dummies
          }
        }
        while (data[offsets[0]] || counters[0]);
        assert(0 == counters.max());
        assert(pat.size() <= MAX_PATTERN_SIZE);
      }
      Information.Statistic.Pattern = Data.Patterns.size();
      Information.Statistic.Channels = 3;

      InitTime();
    }

    virtual void GetInfo(Info& info) const
    {
      info.Capabilities = CAP_AYM;
      info.Properties.clear();
    }

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::Volume& /*volState*/, Sound::Analyze::Spectrum& /*spectrumState*/) const
    {
      return PlaybackState;
    }

    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver)
    {
      AYM::DataChunk chunk;
      chunk.Tick = (CurrentState.Tick += params.ClocksPerFrame());
      RenderData(chunk);

      Device->RenderData(params, chunk, receiver);

      return Parent::RenderFrame(params, receiver);
    }

    virtual State Reset()
    {
      Device->Reset();
      return Parent::Reset();
    }

    virtual State SetPosition(const uint32_t& /*frame*/)
    {
      return PlaybackState;
    }

  private:
    void RenderData(AYM::DataChunk& chunk)
    {
      const Line& line(Data.Patterns[CurrentState.Position.Pattern][CurrentState.Position.Note]);
      if (0 == CurrentState.Position.Frame)//begin note
      {
        for (std::size_t chan = 0; chan != line.Channels.size(); ++chan)
        {
          const Line::Chan& src(line.Channels[chan]);
          ChannelState& dst(Channels[chan]);
          if (src.Enabled)
          {
            if (!(dst.Enabled = *src.Enabled))
            {
              dst.Sliding = dst.Glissade = 0;
              dst.SlidingTargetNote = ~std::size_t(0);
              dst.PosInSample = dst.PosInOrnament = 0;
            }
          }
          if (src.Note)
          {
            dst.Note = *src.Note;
            dst.PosInSample = dst.PosInOrnament = 0;
            dst.Sliding = dst.Glissade = 0;
            dst.SlidingTargetNote = ~std::size_t(0);
          }
          if (src.SampleNum)
          {
            dst.SampleNum = *src.SampleNum;
            dst.PosInSample = 0;
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
          for (CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
          {
            switch (it->Type)
            {
            case ENVELOPE:
              chunk.Data[AYM::DataChunk::REG_ENV] = uint8_t(it->Param1);
              chunk.Data[AYM::DataChunk::REG_TONEE_L] = uint8_t(it->Param2 & 0xff);
              chunk.Data[AYM::DataChunk::REG_TONEE_H] = uint8_t(it->Param2 >> 8);
              chunk.Mask |= (1 << AYM::DataChunk::REG_ENV) |
                (1 << AYM::DataChunk::REG_TONEE_L) | (1 << AYM::DataChunk::REG_TONEE_H);
              dst.Envelope = true;
              break;
            case NOENVELOPE:
              dst.Envelope = false;
              break;
            case NOISE_ADD:
              dst.NoiseAdd = it->Param1;
              break;
            case GLISS_NOTE:
              dst.Glissade = it->Param1;
              dst.SlidingTargetNote = it->Param2;
              break;
            case GLISS:
              dst.Glissade = it->Param1;
              break;
            case NOGLISS:
              dst.Glissade = 0;
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
      std::size_t toneReg = AYM::DataChunk::REG_TONEA_L;
      std::size_t volReg = AYM::DataChunk::REG_VOLA;
      uint8_t toneMsk = AYM::DataChunk::MASK_TONEA;
      uint8_t noiseMsk = AYM::DataChunk::MASK_NOISEA;
      for (ChannelState* dst = Channels; dst != ArrayEnd(Channels);
        ++dst, toneReg += 2, ++volReg, toneMsk <<= 1, noiseMsk <<= 1)
      {
        if (dst->Enabled)
        {
          const Sample& curSample(Data.Samples[dst->SampleNum]);
          const Sample::Line& curSampleLine(curSample.Data[dst->PosInSample]);
          const Ornament& curOrnament(Data.Ornaments[dst->OrnamentNum]);

          //calculate tone
          const std::size_t halfTone(clamp<std::size_t>(dst->Note + curOrnament.Data[dst->PosInOrnament], 0, 95));
          const uint16_t tone(uint16_t(clamp(FreqTable[halfTone] + dst->Sliding + curSampleLine.Vibrato, 0, 0xffff)));
          if (dst->SlidingTargetNote != ~std::size_t(0))
          {
            const unsigned nextTone(FreqTable[dst->Note] + dst->Sliding + dst->Glissade);
            const unsigned slidingTarget(FreqTable[dst->SlidingTargetNote]);
            if ((dst->Glissade > 0 && nextTone >= slidingTarget) ||
                (dst->Glissade < 0 && nextTone <= slidingTarget))
            {
              dst->Note = dst->SlidingTargetNote;
              dst->SlidingTargetNote = ~std::size_t(0);
              dst->Sliding = dst->Glissade = 0;
            }
          }
          dst->Sliding += dst->Glissade;
          chunk.Data[toneReg] = uint8_t(tone & 0xff);
          chunk.Data[toneReg + 1] = uint8_t(tone >> 8);
          chunk.Mask |= 3 << toneReg;
          //calculate level
          chunk.Data[volReg] = GetVolume(dst->Volume, curSampleLine.Level)
            | uint8_t(dst->Envelope ? AYM::DataChunk::MASK_ENV : 0);
          //mixer
          if (curSampleLine.ToneOff)
          {
            chunk.Data[AYM::DataChunk::REG_MIXER] |= toneMsk;
          }
          if (curSampleLine.NoiseOff)
          {
            chunk.Data[AYM::DataChunk::REG_MIXER] |= noiseMsk;
          }
          else
          {
            chunk.Data[AYM::DataChunk::REG_TONEN] = uint8_t(clamp<signed>(curSampleLine.Noise + dst->NoiseAdd, 0, 31));
            chunk.Mask |= 1 << AYM::DataChunk::REG_TONEN;
          }

          if (++dst->PosInSample >= curSample.Data.size())
          {
            dst->PosInSample = curSample.Loop;
          }
          if (++dst->PosInOrnament >= curOrnament.Data.size())
          {
            dst->PosInOrnament = curOrnament.Loop;
          }
        }
        else
        {
          chunk.Data[volReg] = 0;
          //????
          chunk.Data[AYM::DataChunk::REG_MIXER] |= toneMsk | noiseMsk;
        }
      }
      CurrentState.Position.Channels = std::count_if(Channels, ArrayEnd(Channels), 
        boost::mem_fn(&ChannelState::Enabled));
    }
  private:
    AYM::Chip::Ptr Device;
    ChannelState Channels[3];
  };

  //////////////////////////////////////////////////////////////////////////
  void Information(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_AYM;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_PT2_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_PT2_VERSION));
  }

  bool Checking(const String& /*filename*/, const Dump& data)
  {
    //check for header
    const std::size_t size(data.size());
    if (sizeof(PT2Header) > size)
    {
      return false;
    }
    const PT2Header* const header(safe_ptr_cast<const PT2Header*>(&data[0]));
    //check offsets
    for (const uint16_t* sampOff = header->SamplesOffsets; sampOff != ArrayEnd(header->SamplesOffsets); ++sampOff)
    {
      const std::size_t offset(fromLE(*sampOff));
      if (offset >= size || (offset && !*safe_ptr_cast<const PT2Sample*>(&data[offset])))
      {
        return false;
      }
    }
    for (const uint16_t* ornOff = header->OrnamentsOffsets; ornOff != ArrayEnd(header->OrnamentsOffsets); ++ornOff)
    {
      const std::size_t offset(fromLE(*ornOff));
      if (offset >= size || (offset && !*safe_ptr_cast<const PT2Ornament*>(&data[offset])))
      {
        return false;
      }
    }
    if (fromLE(header->PatternsOffset) >= size)
    {
      return false;
    }
    //check patterns
    for (const PT2Pattern* patPos(safe_ptr_cast<const PT2Pattern*>(&data[fromLE(header->PatternsOffset)]));
      *patPos;
      ++patPos)
    {
      if (! *patPos)
      {
        return false;
      }
    }
    return true;
  }

  ModulePlayer::Ptr Creating(const String& filename, const Dump& data)
  {
    assert(Checking(filename, data) || !"Attempt to create pt2 player on invalid data");
    return ModulePlayer::Ptr(new PlayerImpl(filename, data));
  }

  PluginAutoRegistrator pt2Reg(Checking, Creating, Information);
}
