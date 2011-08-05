/*
Abstract:
  STP modules playback support

Last changed:
  $Id$

Author:
(C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "ay_conversion.h"
#include "core/plugins/utils.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <range_checker.h>
#include <tools.h>
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>
#include <core/text/warnings.h>

#define FILE_TAG BD5A4CD5

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //hints
  const std::size_t MAX_MODULE_SIZE = 0x2800;
  const uint_t MAX_PATTERNS_COUNT = 256;
  const uint_t MAX_SAMPLES_COUNT = 15;
  const uint_t MAX_ORNAMENTS_COUNT = 16;
  const uint_t MAX_PATTERN_SIZE = 64;
  const uint_t MAX_ORNAMENT_SIZE = 32;
  const uint_t MAX_SAMPLE_SIZE = 32;

  //////////////////////////////////////////////////////////////////////////
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct STPHeader
  {
    uint8_t Tempo;
    uint16_t PositionsOffset;
    uint16_t PatternsOffset;
    uint16_t OrnamentsOffset;
    uint16_t SamplesOffset;
    uint8_t FixesCount;
  } PACK_POST;

  const uint8_t STP_ID[] =
  {
    'K', 'S', 'A', ' ', 'S', 'O', 'F', 'T', 'W', 'A', 'R', 'E', ' ',
    'C', 'O', 'M', 'P', 'I', 'L', 'A', 'T', 'I', 'O', 'N', ' ', 'O', 'F', ' '
  };

  PACK_PRE struct STPId
  {
    uint8_t Id[sizeof(STP_ID)];
    char Title[25];

    bool Check() const
    {
      return 0 == std::memcmp(Id, STP_ID, sizeof(Id));
    }
  } PACK_POST;

  PACK_PRE struct STPPositions
  {
    uint8_t Lenght;
    uint8_t Loop;
    PACK_PRE struct STPPosEntry
    {
      uint8_t PatternOffset;//*6
      int8_t PatternHeight;
    } PACK_POST;
    STPPosEntry Data[1];
  } PACK_POST;

  PACK_PRE struct STPPattern
  {
    boost::array<uint16_t, Devices::AYM::CHANNELS> Offsets;
  } PACK_POST;

  PACK_PRE struct STPOrnament
  {
    uint8_t Loop;
    uint8_t Size;
    int8_t Data[1];

    static uint_t GetMinimalSize()
    {
      return sizeof(STPOrnament) - sizeof(int8_t);
    }

    uint_t GetSize() const
    {
      return sizeof(*this) + (Size - 1) * sizeof(Data[0]);
    }
  } PACK_POST;

  PACK_PRE struct STPOrnaments
  {
    boost::array<uint16_t, MAX_ORNAMENTS_COUNT> Offsets;
  } PACK_POST;

  PACK_PRE struct STPSample
  {
    int8_t Loop;
    uint8_t Size;

    static uint_t GetMinimalSize()
    {
      return sizeof(STPSample) - sizeof(STPSample::Line);
    }

    uint_t GetSize() const
    {
      return sizeof(*this) - sizeof(Data[0]);
    }

    PACK_PRE struct Line
    {
      // NxxTaaaa
      // xxnnnnnE
      // vvvvvvvv
      // vvvvvvvv

      // a - level
      // T - tone mask
      // N - noise mask
      // E - env mask
      // n - noise
      // v - signed vibrato
      uint8_t LevelAndFlags;
      uint8_t NoiseAndFlag;
      int16_t Vibrato;

      uint_t GetLevel() const
      {
        return LevelAndFlags & 15;
      }

      bool GetToneMask() const
      {
        return 0 != (LevelAndFlags & 16);
      }

      bool GetNoiseMask() const
      {
        return 0 != (LevelAndFlags & 128);
      }

      bool GetEnvelopeMask() const
      {
        return 0 != (NoiseAndFlag & 1);
      }

      uint_t GetNoise() const
      {
        return (NoiseAndFlag & 62) >> 1;
      }

      int_t GetVibrato() const
      {
        return fromLE(Vibrato);
      }
    } PACK_POST;
    Line Data[1];
  } PACK_POST;

  PACK_PRE struct STPSamples
  {
    boost::array<uint16_t, MAX_SAMPLES_COUNT> Offsets;
  } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(STPHeader) == 10);
  BOOST_STATIC_ASSERT(sizeof(STPPositions) == 4);
  BOOST_STATIC_ASSERT(sizeof(STPPattern) == 6);
  BOOST_STATIC_ASSERT(sizeof(STPOrnament) == 3);
  BOOST_STATIC_ASSERT(sizeof(STPOrnaments) == 32);
  BOOST_STATIC_ASSERT(sizeof(STPSample) == 6);
  BOOST_STATIC_ASSERT(sizeof(STPSamples) == 30);

  struct Sample
  {
    Sample() : Loop(), Lines()
    {
    }

    template<class T>
    Sample(int_t loop, T from, T to)
      : Loop(loop), Lines(from, to)
    {
    }

    struct Line
    {
      Line()
        : Level(), Noise(), ToneMask(), NoiseMask(), EnvelopeMask(), Vibrato()
      {
      }

      /*explicit*/Line(const STPSample::Line& line)
        : Level(line.GetLevel()), Noise(line.GetNoise())
        , ToneMask(line.GetToneMask()), NoiseMask(line.GetNoiseMask()), EnvelopeMask(line.GetEnvelopeMask())
        , Vibrato(line.GetVibrato())
      {
      }
      uint_t Level;//0-15
      uint_t Noise;//0-31
      bool ToneMask;
      bool NoiseMask;
      bool EnvelopeMask;
      int_t Vibrato;
    };

    int_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetSize() const
    {
      return static_cast<uint_t>(Lines.size());
    }

    const Line& GetLine(uint_t idx) const
    {
      static const Line STUB;
      return Lines.size() > idx ? Lines[idx] : STUB;
    }
  private:
    int_t Loop;
    std::vector<Line> Lines;
  };

  enum CmdType
  {
    EMPTY,
    ENVELOPE,     //2p
    NOENVELOPE,   //0p
    GLISS,        //1p
  };

  typedef TrackingSupport<Devices::AYM::CHANNELS, CmdType, Sample> STPTrack;
  typedef std::vector<int_t> STPTransposition;

  class STPAreas
  {
  public:
    enum AreaTypes
    {
      HEADER,
      IDENTIFIER,
      POSITIONS,
      PATTERNS,
      ORNAMENTS,
      SAMPLES,
      END,
    };

    explicit STPAreas(const IO::FastDump& data)
      : Data(data)
    {
      const STPHeader* const header = safe_ptr_cast<const STPHeader*>(Data.Data());

      Areas.AddArea(HEADER, 0);
      Areas.AddArea(IDENTIFIER, sizeof(*header));
      Areas.AddArea(POSITIONS, fromLE(header->PositionsOffset));
      Areas.AddArea(PATTERNS, fromLE(header->PatternsOffset));
      Areas.AddArea(ORNAMENTS, fromLE(header->OrnamentsOffset));
      Areas.AddArea(SAMPLES, fromLE(header->SamplesOffset));
      Areas.AddArea(END, static_cast<uint_t>(data.Size()));
    }

    const IO::FastDump& GetOriginalData() const
    {
      return Data;
    }

    const STPHeader& GetHeader() const
    {
      return *safe_ptr_cast<const STPHeader*>(Data.Data());
    }

    const STPOrnaments& GetOrnaments() const
    {
      const uint_t offset = Areas.GetAreaAddress(ORNAMENTS);
      return *safe_ptr_cast<const STPOrnaments*>(&Data[offset]);
    }

    uint_t GetOrnamentsLimit() const
    {
      const uint_t offset = Areas.GetAreaAddress(ORNAMENTS);
      return offset + sizeof(STPOrnaments);
    }

    const STPOrnament& GetOrnament(uint_t offset) const
    {
      return *safe_ptr_cast<const STPOrnament*>(&Data[offset]);
    }

    const STPSamples& GetSamples() const
    {
      const uint_t offset = Areas.GetAreaAddress(SAMPLES);
      return *safe_ptr_cast<const STPSamples*>(&Data[offset]);
    }

    uint_t GetSamplesLimit() const
    {
      const uint_t offset = Areas.GetAreaAddress(SAMPLES);
      return offset + sizeof(STPSamples);
    }

    const STPSample& GetSample(uint_t offset) const
    {
      return *safe_ptr_cast<const STPSample*>(&Data[offset]);
    }

    RangeIterator<const STPPattern*> GetPatterns() const
    {
      const uint_t offset = Areas.GetAreaAddress(PATTERNS);
      const uint_t size = Areas.GetAreaSize(PATTERNS);
      const STPPattern* const begin = safe_ptr_cast<const STPPattern*>(&Data[offset]);
      const STPPattern* const end = begin + size / sizeof(*begin);
      return RangeIterator<const STPPattern*>(begin, end);
    }

    uint_t GetLoopPosition() const
    {
      const uint_t offset = Areas.GetAreaAddress(POSITIONS);
      const STPPositions* const positions = safe_ptr_cast<const STPPositions*>(&Data[offset]);
      return positions->Loop;
    }

    RangeIterator<const STPPositions::STPPosEntry*> GetPositions() const
    {
      const uint_t offset = Areas.GetAreaAddress(POSITIONS);
      const STPPositions* const positions = safe_ptr_cast<const STPPositions*>(&Data[offset]);
      const STPPositions::STPPosEntry* const entry = positions->Data;
      return RangeIterator<const STPPositions::STPPosEntry*>(entry, entry + positions->Lenght);
    }

    uint_t GetPositionsLimit() const
    {
      const uint_t offset = Areas.GetAreaAddress(POSITIONS);
      const STPPositions* const positions = safe_ptr_cast<const STPPositions*>(&Data[offset]);
      const uint_t size = sizeof(*positions) + sizeof(positions->Data[0]) * (positions->Lenght - 1);
      return offset + size;
    }
  protected:
    const IO::FastDump& Data;
    AreaController<AreaTypes, 1 + END, uint_t> Areas;
  };

  struct STPModuleData : public STPTrack::ModuleData
  {
    typedef boost::shared_ptr<STPModuleData> RWPtr;
    typedef boost::shared_ptr<const STPModuleData> Ptr;

    STPModuleData()
      : STPTrack::ModuleData()
    {
    }

    void ParseInformation(const STPAreas& areas, ModuleProperties& props)
    {
      const STPHeader& header = areas.GetHeader();
      InitialTempo = header.Tempo;
      LoopPosition = areas.GetLoopPosition();
      props.SetProgram(Text::STP_EDITOR);
      const STPId& id = *safe_ptr_cast<const STPId*>(&header + 1);
      if (id.Check())
      {
        props.SetTitle(OptimizeString(FromCharArray(id.Title)));
      }
      props.SetFreqtable(TABLE_SOUNDTRACKER);
    }

    uint_t ParseOrnaments(const STPAreas& areas)
    {
      const STPOrnaments& ornaments = areas.GetOrnaments();
      Ornaments.reserve(ornaments.Offsets.size());
      uint_t resLimit = areas.GetOrnamentsLimit();
      for (uint_t idx = 0; idx != ornaments.Offsets.size(); ++idx)
      {
        const uint_t offset = fromLE(ornaments.Offsets[idx]);
        const uint_t ornLimit = ParseOrnament(offset, areas);
        resLimit = std::max(resLimit, ornLimit);
      }
      return resLimit;
    }

    uint_t ParseSamples(const STPAreas& areas)
    {
      const STPSamples& samples = areas.GetSamples();
      Samples.reserve(samples.Offsets.size());
      for (uint_t idx = 0; idx != samples.Offsets.size(); ++idx)
      {
        const uint_t offset = fromLE(samples.Offsets[idx]);
        ParseSample(offset, areas);
      }
      return areas.GetSamplesLimit();
    }

    uint_t ParsePatterns(const STPAreas& areas)
    {
      assert(!Samples.empty());
      assert(!Ornaments.empty());
      uint_t resLimit = 0;

      Patterns.reserve(MAX_PATTERNS_COUNT);
      for (RangeIterator<const STPPattern*> iter = areas.GetPatterns(); iter; ++iter)
      {
        if (Patterns.size() >= MAX_PATTERNS_COUNT)
        {
          throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
        }
        const STPPattern& pattern = *iter;
        Patterns.push_back(STPTrack::Pattern());
        STPTrack::Pattern& dst = Patterns.back();
        const uint_t patLimit = ParsePattern(areas.GetOriginalData(), pattern, dst);
        resLimit = std::max(resLimit, patLimit);
      }
      return resLimit;
    }

    uint_t ParsePositions(const STPAreas& areas)
    {
      assert(!Patterns.empty());
      uint_t positionsCount = 0;
      for (RangeIterator<const STPPositions::STPPosEntry*> iter = areas.GetPositions();
        iter; ++iter, ++positionsCount)
      {
        const STPPositions::STPPosEntry& entry = *iter;
        if (CheckPosition(entry))
        {
          AddPosition(entry);
        }
      }
      if (Positions.empty())
      {
        throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
      }
      return areas.GetPositionsLimit();
    }
  private:
    uint_t ParseOrnament(uint_t offset, const STPAreas& areas)
    {
      const STPOrnament& ornament = areas.GetOrnament(offset);
      const uint_t realLoop = std::min<uint_t>(ornament.Loop, ornament.Size);
      Ornaments.push_back(SimpleOrnament(realLoop, ornament.Data, ornament.Data + ornament.Size));
      return offset + sizeof(STPOrnament) + (ornament.Size - 1) * sizeof(ornament.Data[0]);
    }

    uint_t ParseSample(uint_t offset, const STPAreas& areas)
    {
      const STPSample& sample = areas.GetSample(offset);
      const int_t realLoop = std::min<int_t>(sample.Loop, sample.Size);
      Samples.push_back(Sample(realLoop, sample.Data, sample.Data + sample.Size));
      return offset + sizeof(STPSample) + (sample.Size - 1) * sizeof(sample.Data[0]);
    }

    uint_t ParsePattern(const IO::FastDump& data, const STPPattern& pattern, STPTrack::Pattern& dst)
    {
      AYM::PatternCursors cursors;
      std::transform(pattern.Offsets.begin(), pattern.Offsets.end(), cursors.begin(), std::ptr_fun(&fromLE<uint16_t>));
      uint_t& channelACursor = cursors.front().Offset;
      do
      {
        STPTrack::Line& line = dst.AddLine();
        ParsePatternLine(data, cursors, line);
        //skip lines
        if (const uint_t linesToSkip = cursors.GetMinCounter())
        {
          cursors.SkipLines(linesToSkip);
          dst.AddLines(linesToSkip);
        }
      }
      while (channelACursor < data.Size() &&
             (0 != data[channelACursor] || 0 != cursors.front().Counter));
      const uint_t maxOffset = 1 + cursors.GetMaxOffset();
      return maxOffset;
    }

    void ParsePatternLine(const IO::FastDump& data, AYM::PatternCursors& cursors, STPTrack::Line& line)
    {
      assert(line.Channels.size() == cursors.size());
      STPTrack::Line::ChannelsArray::iterator channel(line.Channels.begin());
      for (AYM::PatternCursors::iterator cur = cursors.begin(); cur != cursors.end(); ++cur, ++channel)
      {
        ParsePatternLineChannel(data, *cur, *channel);
      }
    }

    void ParsePatternLineChannel(const IO::FastDump& data, PatternCursor& cur, STPTrack::Line::Chan& channel)
    {
      if (cur.Counter--)
      {
        return;//has to skip
      }

      for (const std::size_t dataSize = data.Size(); cur.Offset < dataSize;)
      {
        const uint_t cmd = data[cur.Offset++];
        const std::size_t restbytes = dataSize - cur.Offset;
        if (cmd >= 1 && cmd <= 0x60)//note
        {
          channel.SetNote(cmd - 1);
          channel.SetEnabled(true);
          break;
        }
        else if (cmd >= 0x61 && cmd <= 0x6f)//sample
        {
          channel.SetSample(cmd - 0x61);
        }
        else if (cmd >= 0x70 && cmd <= 0x7f)//ornament
        {
          channel.SetOrnament(cmd - 0x70);
          channel.Commands.push_back(STPTrack::Command(NOENVELOPE));
          channel.Commands.push_back(STPTrack::Command(GLISS, 0));
        }
        else if (cmd >= 0x80 && cmd <= 0xbf) //skip
        {
          cur.Period = cmd - 0x80;
        }
        else if (cmd >= 0xc0 && cmd <= 0xcf)
        {
          if (cmd != 0xc0 && restbytes < 1)
          {
            throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
          }
          channel.Commands.push_back(STPTrack::Command(ENVELOPE, cmd - 0xc0,
            cmd != 0xc0 ? data[cur.Offset++] : 0));
          channel.SetOrnament(0);
          channel.Commands.push_back(STPTrack::Command(GLISS, 0));
        }
        else if (cmd >= 0xd0 && cmd <= 0xdf)//reset
        {
          channel.SetEnabled(false);
          break;
        }
        else if (cmd >= 0xe0 && cmd <= 0xef)//empty
        {
          break;
        }
        else if (cmd == 0xf0) //glissade
        {
          if (restbytes < 1)
          {
            throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
          }
          channel.Commands.push_back(STPTrack::Command(GLISS, static_cast<int8_t>(data[cur.Offset++])));
        }
        else if (cmd >= 0xf1 && cmd <= 0xff) //volume
        {
          channel.SetVolume(cmd - 0xf1);
        }
      }
      cur.Counter = cur.Period;
    }

    bool CheckPosition(const STPPositions::STPPosEntry& entry) const
    {
      const uint_t patNum = entry.PatternOffset / 6;
      assert(0 == entry.PatternOffset % 6);
      return patNum < Patterns.size() && !Patterns[patNum].IsEmpty();
    }

    void AddPosition(const STPPositions::STPPosEntry& entry)
    {
      Positions.push_back(entry.PatternOffset / 6);
      Transpositions.push_back(entry.PatternHeight);
    }
  public:
    STPTransposition Transpositions;
  };

  struct STPChannelState
  {
    STPChannelState()
      : Enabled(false), Envelope(false), Volume(0)
      , Note(0), SampleNum(0), PosInSample(0)
      , OrnamentNum(0), PosInOrnament(0)
      , TonSlide(0), Glissade(0)
    {
    }
    bool Enabled;
    bool Envelope;
    uint_t Volume;
    uint_t Note;
    uint_t SampleNum;
    uint_t PosInSample;
    uint_t OrnamentNum;
    uint_t PosInOrnament;
    int_t TonSlide;
    int_t Glissade;
  };

  class STPDataRenderer : public AYM::DataRenderer
  {
  public:
    explicit STPDataRenderer(STPModuleData::Ptr data)
      : Data(data)
    {
    }

    virtual void Reset()
    {
      std::fill(PlayerState.begin(), PlayerState.end(), STPChannelState());
    }

    virtual void SynthesizeData(const TrackState& state, AYM::TrackBuilder& track)
    {
      if (0 == state.Quirk())
      {
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(state, track);
    }

  private:
    void GetNewLineState(const TrackState& state, AYM::TrackBuilder& track)
    {
      if (const STPTrack::Line* line = Data->Patterns[state.Pattern()].GetLine(state.Line()))
      {
        for (uint_t chan = 0; chan != line->Channels.size(); ++chan)
        {
          const STPTrack::Line::Chan& src = line->Channels[chan];
          if (src.Empty())
          {
            continue;
          }
          GetNewChannelState(src, PlayerState[chan], track);
        }
      }
    }

    void GetNewChannelState(const STPTrack::Line::Chan& src, STPChannelState& dst, AYM::TrackBuilder& track)
    {
      if (src.Enabled)
      {
        dst.Enabled = *src.Enabled;
        dst.PosInSample = 0;
        dst.PosInOrnament = 0;
      }
      if (src.Note)
      {
        dst.Note = *src.Note;
        dst.PosInSample = 0;
        dst.PosInOrnament = 0;
        dst.TonSlide = 0;
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
      for (STPTrack::CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
      {
        switch (it->Type)
        {
        case ENVELOPE:
          if (it->Param1)
          {
            track.SetEnvelopeType(it->Param1);
            track.SetEnvelopeTone(it->Param2);
          }
          dst.Envelope = true;
          break;
        case NOENVELOPE:
          dst.Envelope = false;
          break;
        case GLISS:
          dst.Glissade = it->Param1;
          break;
        default:
          assert(!"Invalid command");
        }
      }
    }

    void SynthesizeChannelsData(const TrackState& state, AYM::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        AYM::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(state, PlayerState[chan], channel, track);
      }
    }

    void SynthesizeChannel(const TrackState& state, STPChannelState& dst, AYM::ChannelBuilder& channel, AYM::TrackBuilder& track)
    {
      if (!dst.Enabled)
      {
        channel.SetLevel(0);
        return;
      }

      const STPTrack::Sample& curSample = Data->Samples[dst.SampleNum];
      const STPTrack::Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const STPTrack::Ornament& curOrnament = Data->Ornaments[dst.OrnamentNum];

      //calculate tone
      dst.TonSlide += dst.Glissade;

      //apply level
      channel.SetLevel(int_t(curSampleLine.Level) - dst.Volume);
      //apply envelope
      if (curSampleLine.EnvelopeMask && dst.Envelope)
      {
        channel.EnableEnvelope();
      }
      //apply tone
      if (!curSampleLine.ToneMask)
      {
        const int_t halftones = int_t(dst.Note) +
                                Data->Transpositions[state.Position()] +
                                (dst.Envelope ? 0 : curOrnament.GetLine(dst.PosInOrnament));
        channel.SetTone(halftones, dst.TonSlide + curSampleLine.Vibrato);
      }
      else
      {
        channel.DisableTone();
      }
      //apply noise
      if (!curSampleLine.NoiseMask)
      {
        track.SetNoise(curSampleLine.Noise);
      }
      else
      {
        channel.DisableNoise();
      }

      if (++dst.PosInOrnament >= curOrnament.GetSize())
      {
        dst.PosInOrnament = curOrnament.GetLoop();
      }

      if (++dst.PosInSample >= curSample.GetSize())
      {
        if (curSample.GetLoop() >= 0)
        {
          dst.PosInSample = curSample.GetLoop();
        }
        else
        {
          dst.Enabled = false;
        }
      }
    }
  private:
    const STPModuleData::Ptr Data;
    boost::array<STPChannelState, Devices::AYM::CHANNELS> PlayerState;
  };

  class STPChiptune : public AYM::Chiptune
  {
  public:
    STPChiptune(STPModuleData::Ptr data, ModuleProperties::Ptr properties)
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
      const StateIterator::Ptr iterator = CreateTrackStateIterator(Info, Data);
      const AYM::DataRenderer::Ptr renderer = boost::make_shared<STPDataRenderer>(Data);
      return AYM::CreateDataIterator(trackParams, iterator, renderer);
    }
  private:
    const STPModuleData::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };

  class STPAreasChecker : public STPAreas
  {
  public:
    explicit STPAreasChecker(const IO::FastDump& data)
      : STPAreas(data)
    {
    }

    bool CheckLayout() const
    {
      return sizeof(STPHeader) == Areas.GetAreaSize(STPAreas::HEADER) &&
             Areas.Undefined == Areas.GetAreaSize(STPAreas::END);
    }

    bool CheckHeader() const
    {
      const STPHeader& header = GetHeader();
      if (!in_range<uint_t>(header.Tempo, 1, 15))
      {
        return false;
      }
      return true;
    }

    bool CheckSamples() const
    {
      const uint_t size = Areas.GetAreaSize(STPAreas::SAMPLES);
      if (sizeof(STPSamples) > size)
      {
        return false;
      }

      const uint_t samplesOffset = Areas.GetAreaAddress(STPAreas::SAMPLES);
      const uint_t limit = Areas.GetAreaAddress(STPAreas::END);
      const STPSamples* const samples = safe_ptr_cast<const STPSamples*>(&Data[samplesOffset]);
      for (const uint16_t* smpOff = samples->Offsets.begin(); smpOff != samples->Offsets.end(); ++smpOff)
      {
        const uint_t offset = fromLE(*smpOff);
        if (!offset || offset + STPSample::GetMinimalSize() > limit)
        {
          return false;
        }
        const STPSample* const sample = safe_ptr_cast<const STPSample*>(&Data[offset]);
        //may be empty
        if (!sample->Size)
        {
          continue;
        }
        if (offset + sample->GetSize() > limit)
        {
          return false;
        }
      }
      return true;
    }

    bool CheckOrnaments() const
    {
      const uint_t size = Areas.GetAreaSize(STPAreas::ORNAMENTS);
      if (sizeof(STPOrnaments) != size)
      {
        return false;
      }

      const uint_t ornamentsOffset = Areas.GetAreaAddress(STPAreas::ORNAMENTS);
      const uint_t limit = Areas.GetAreaAddress(STPAreas::END);
      const STPOrnaments* const ornaments = safe_ptr_cast<const STPOrnaments*>(&Data[ornamentsOffset]);
      for (const uint16_t* ornOff = ornaments->Offsets.begin(); ornOff != ornaments->Offsets.end(); ++ornOff)
      {
        const uint_t offset = fromLE(*ornOff);
        if (!offset || offset + STPOrnament::GetMinimalSize() > limit)
        {
          return false;
        }
        const STPOrnament* const ornament = safe_ptr_cast<const STPOrnament*>(&Data[offset]);
        //may be empty
        if (!ornament->Size)
        {
          continue;
        }
        if (offset + ornament->GetSize() > limit)
        {
          return false;
        }
      }
      return true;
    }

    bool CheckPositions() const
    {
      const uint_t offset = Areas.GetAreaAddress(STPAreas::POSITIONS);
      const uint_t limit = Areas.GetAreaAddress(STPAreas::END);
      if (offset + sizeof(STPPositions) > limit)
      {
        return false;
      }
      // check positions
      const STPPositions* const positions = safe_ptr_cast<const STPPositions*>(&Data[offset]);
      if (!positions->Lenght ||
          positions->Data + positions->Lenght != std::find_if(positions->Data, positions->Data + positions->Lenght,
            boost::bind<uint8_t>(std::modulus<uint8_t>(), boost::bind(&STPPositions::STPPosEntry::PatternOffset, _1), 6))
         )
      {
        return false;
      }
      return true;
    }

    bool CheckPatterns() const
    {
      const uint_t offset = Areas.GetAreaAddress(STPAreas::PATTERNS);
      const uint_t limit = Areas.GetAreaAddress(STPAreas::END);
      if (offset + sizeof(STPPattern) > limit)
      {
        return false;
      }
      if (Areas.GetAreaSize(STPAreas::PATTERNS) < sizeof(STPPattern))
      {
        return false;
      }
      for (RangeIterator<const STPPattern*> pattern = GetPatterns(); pattern; ++pattern)
      {
        if (pattern->Offsets.end() != std::find_if(pattern->Offsets.begin(), pattern->Offsets.end(),
          boost::bind(&fromLE<uint16_t>, _1) > limit - 1))
        {
          return false;
        }
      }
      return true;
    }
  };

  bool CheckSTPModule(const uint8_t* data, std::size_t size)
  {
    const std::size_t limit = std::min(size, MAX_MODULE_SIZE);
    if (limit < sizeof(STPHeader))
    {
      return false;
    }

    const IO::FastDump dump(data, limit);
    const STPAreasChecker areas(dump);

    if (!areas.CheckLayout())
    {
      return false;
    }
    if (!areas.CheckHeader())
    {
      return false;
    }
    if (!areas.CheckSamples())
    {
      return false;
    }
    if (!areas.CheckOrnaments())
    {
      return false;
    }
    if (!areas.CheckPositions())
    {
      return false;
    }
    if (!areas.CheckPatterns())
    {
      return false;
    }
    return true;
  }
}

namespace
{
  using namespace ZXTune;

  //plugin attributes
  const Char ID[] = {'S', 'T', 'P', 0};
  const Char* const INFO = Text::STP_PLUGIN_INFO;
  const String VERSION(FromStdString("$Rev$"));
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | GetSupportedAYMFormatConvertors();

  const std::string STP_FORMAT(
    "01-0f"  // uint8_t Tempo; 0..15
    "?00-28" // uint16_t PositionsOffset; 0..MAX_MODULE_SIZE
    "?00-28" // uint16_t PatternsOffset; 0..MAX_MODULE_SIZE
    "?00-28" // uint16_t OrnamentsOffset; 0..MAX_MODULE_SIZE
    "?00-28" // uint16_t SamplesOffset; 0..MAX_MODULE_SIZE
  );

  //////////////////////////////////////////////////////////////////////////
  class STPModulesFactory : public ModulesFactory
  {
  public:
    STPModulesFactory()
      : Format(DataFormat::Create(STP_FORMAT))
    {
    }
    virtual bool Check(const IO::DataContainer& inputData) const
    {
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t size = inputData.Size();
      return Format->Match(data, size) && CheckSTPModule(data, size);
    }

    virtual DataFormat::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Parameters::Accessor::Ptr parameters, IO::DataContainer::Ptr rawData, std::size_t& usedSize) const
    {
      try
      {
        assert(Check(*rawData));

        //assume that data is ok
        const IO::FastDump& data = IO::FastDump(*rawData, 0, MAX_MODULE_SIZE);
        const STPAreas areas(data);

        const STPModuleData::RWPtr parsedData = boost::make_shared<STPModuleData>();
        parsedData->ParseInformation(areas, *properties);
        const uint_t ornLim = parsedData->ParseOrnaments(areas);
        const uint_t smpLim = parsedData->ParseSamples(areas);
        const uint_t patLim = parsedData->ParsePatterns(areas);
        const uint_t posLim = parsedData->ParsePositions(areas);

        const std::size_t maxLim = std::max(std::max(smpLim, ornLim), std::max(patLim, posLim));
        usedSize = std::min(data.Size(), maxLim);

        //meta properties
        {
          const std::size_t fixedOffset = sizeof(STPHeader);
          const ModuleRegion fixedRegion(fixedOffset, usedSize -  fixedOffset);
          properties->SetSource(usedSize, fixedRegion);
        }

        const AYM::Chiptune::Ptr chiptune = boost::make_shared<STPChiptune>(parsedData, properties);
        return AYM::CreateHolder(chiptune, parameters);
      }
      catch (const Error&/*e*/)
      {
        Log::Debug("Core::STPSupp", "Failed to create holder");
      }
      return Holder::Ptr();
    }
  private:
    const DataFormat::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterSTPSupport(PluginsRegistrator& registrator)
  {
    const ModulesFactory::Ptr factory = boost::make_shared<STPModulesFactory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, INFO, VERSION, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
