/*
Abstract:
  STC modules playback support

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

#define FILE_TAG D9281D1D

namespace
{
  using namespace ZXTune;

  const String VERSION(FromStdString("$Rev$"));
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | Module::GetSupportedAYMFormatConvertors();
}

namespace SoundTracker
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const uint_t MAX_SAMPLES_COUNT = 16;
  const uint_t MAX_ORNAMENTS_COUNT = 16;
  const uint_t MAX_PATTERN_SIZE = 64;
  const uint_t MAX_PATTERNS_COUNT = 32;
  const uint_t SAMPLE_ORNAMENT_SIZE = 32;

  // currently used sample
  struct Sample
  {
    Sample() : Loop(), LoopLimit(), Lines()
    {
    }

    template<class T>
    Sample(uint_t loop, uint_t loopLimit, T from, T to)
      : Loop(loop)
      , LoopLimit(loopLimit)
      , Lines(from, to)
    {
    }

    struct Line
    {
      Line() : Level(), Noise(), NoiseMask(), EnvelopeMask(), Effect()
      {
      }

      uint_t Level;//0-15
      uint_t Noise;//0-31
      bool NoiseMask;
      bool EnvelopeMask;
      int_t Effect;
    };

    uint_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetLoopLimit() const
    {
      return LoopLimit;
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
    uint_t Loop;
    uint_t LoopLimit;
    std::vector<Line> Lines;
  };

  enum CmdType
  {
    EMPTY,
    ENVELOPE,     //2p
    NOENVELOPE,   //0p
  };

  typedef TrackingSupport<Devices::AYM::CHANNELS, CmdType, Sample> Track;

  class ModuleData : public Track::ModuleData
  {
  public:
    typedef boost::shared_ptr<const ModuleData> Ptr;

    std::vector<int_t> Transpositions;
  };

  class ChannelBuilder
  {
  public:
    ChannelBuilder(uint_t transposition, AYM::TrackBuilder& track, uint_t chanNum)
      : Transposition(transposition)
      , Track(track)
      , Channel(Track.GetChannel(chanNum))
    {
    }

    void SetLevel(uint_t level)
    {
      Channel.SetLevel(level);
    }

    void EnableEnvelope()
    {
      Channel.EnableEnvelope();
    }

    void SetTone(int_t halftones, int_t offset)
    {
      Channel.SetTone(halftones + Transposition, offset);
    }

    void DisableTone()
    {
      Channel.DisableTone();
    }

    void SetNoise(int_t level)
    {
      Track.SetNoise(level);
    }

    void DisableNoise()
    {
      Channel.DisableNoise();
    }

  private:
    const uint_t Transposition;
    AYM::TrackBuilder& Track;
    AYM::ChannelBuilder Channel;
  };

  struct ChannelState
  {
    explicit ChannelState(ModuleData::Ptr data)
      : Data(data)
      , Enabled(false), Envelope(false)
      , Note()
      , CurSample(GetStubSample()), PosInSample(0), LoopedInSample(false)
      , CurOrnament(GetStubOrnament())
    {
    }

    void Reset()
    {
      Enabled = false;
      Envelope = false;
      Note = 0;
      CurSample = GetStubSample();
      PosInSample = 0;
      LoopedInSample = false;
      CurOrnament = GetStubOrnament();
    }

    void SetNewState(const Track::Line::Chan& src)
    {
      if (src.Enabled)
      {
        SetEnabled(*src.Enabled);
      }
      if (src.Note)
      {
        SetNote(*src.Note);
      }
      if (src.SampleNum)
      {
        SetSample(*src.SampleNum);
      }
      if (src.OrnamentNum)
      {
        SetOrnament(*src.OrnamentNum);
      }
      if (!src.Commands.empty())
      {
        ApplyCommands(src.Commands);
      }
    }

    void Synthesize(ChannelBuilder& channel) const
    {
      if (!Enabled)
      {
        channel.SetLevel(0);
        return;
      }

      const Track::Sample::Line& curSampleLine = CurSample->GetLine(PosInSample);

      //apply level
      channel.SetLevel(curSampleLine.Level);
      //apply envelope
      if (Envelope)
      {
        channel.EnableEnvelope();
      }
      //apply tone
      const int_t halftones = int_t(Note) + CurOrnament->GetLine(PosInSample);
      if (!curSampleLine.EnvelopeMask)
      {
        channel.SetTone(halftones, curSampleLine.Effect);
      }
      else
      {
        channel.DisableTone();
      }
      //apply noise
      if (!curSampleLine.NoiseMask)
      {
        channel.SetNoise(curSampleLine.Noise);
      }
      else
      {
        channel.DisableNoise();
      }
    }

    void Iterate()
    {
      if (++PosInSample >= (LoopedInSample ? CurSample->GetLoopLimit() : CurSample->GetSize()))
      {
        if (CurSample->GetLoop() && CurSample->GetLoop() < CurSample->GetSize())
        {
          PosInSample = CurSample->GetLoop();
          LoopedInSample = true;
        }
        else
        {
          Enabled = false;
        }
      }
    }
  private:
    static const Track::Sample* GetStubSample()
    {
      static const Track::Sample stubSample;
      return &stubSample;
    }

    static const Track::Ornament* GetStubOrnament()
    {
      static const Track::Ornament stubOrnament;
      return &stubOrnament;
    }

    void SetEnabled(bool enabled)
    {
      Enabled = enabled;
      if (!Enabled)
      {
        PosInSample = 0;
      }
    }

    void SetNote(uint_t note)
    {
      Note = note;
      PosInSample = 0;
      LoopedInSample = false;
    }

    void SetSample(uint_t sampleNum)
    {
      CurSample = sampleNum < Data->Samples.size() ? &Data->Samples[sampleNum] : GetStubSample();
      PosInSample = 0;
    }

    void SetOrnament(uint_t ornamentNum)
    {
      CurOrnament = ornamentNum < Data->Ornaments.size() ? &Data->Ornaments[ornamentNum] : GetStubOrnament();
      PosInSample = 0;
    }

    void ApplyCommands(const Track::CommandsArray& commands)
    {
      std::for_each(commands.begin(), commands.end(), boost::bind(&ChannelState::ApplyCommand, this, _1));
    }

    void ApplyCommand(const Track::Command& command)
    {
      if (command == ENVELOPE)
      {
        Envelope = true;
      }
      else if (command == NOENVELOPE)
      {
        Envelope = false;
      }
    }
  private:
    const ModuleData::Ptr Data;
    bool Enabled;
    bool Envelope;
    uint_t Note;
    const Track::Sample* CurSample;
    uint_t PosInSample;
    bool LoopedInSample;
    const Track::Ornament* CurOrnament;
  };

  void ProcessEnvelopeCommands(const Track::CommandsArray& commands, AYM::TrackBuilder& track)
  {
    const Track::CommandsArray::const_iterator it = std::find(commands.begin(), commands.end(), ENVELOPE);
    if (it != commands.end())
    {
      track.SetEnvelopeType(it->Param1);
      track.SetEnvelopeTone(it->Param2);
    }
  }

  class DataIterator : public AYM::DataIterator
  {
  public:
    DataIterator(AYM::TrackParameters::Ptr trackParams, StateIterator::Ptr delegate, ModuleData::Ptr data)
      : TrackParams(trackParams)
      , Delegate(delegate)
      , State(Delegate->GetStateObserver())
      , Data(data)
      , StateA(Data)
      , StateB(Data)
      , StateC(Data)
    {
      SwitchToNewLine();
    }

    virtual void Reset()
    {
      Delegate->Reset();
      StateA.Reset();
      StateB.Reset();
      StateC.Reset();
      SwitchToNewLine();
    }

    virtual bool IsValid() const
    {
      return Delegate->IsValid();
    }

    virtual void NextFrame(bool looped)
    {
      if (Delegate->IsValid())
      {
        Delegate->NextFrame(looped);
        StateA.Iterate();
        StateB.Iterate();
        StateC.Iterate();
        if (Delegate->IsValid() && 0 == State->Quirk())
        {
          SwitchToNewLine();
        }
      }
    }

    virtual TrackState::Ptr GetStateObserver() const
    {
      return State;
    }

    virtual void GetData(Devices::AYM::DataChunk& chunk) const
    {
      if (Delegate->IsValid())
      {
        AYM::TrackBuilder track(TrackParams->FreqTable());

        if (0 == State->Quirk())
        {
          GetNewLineState(track);
        }
        SynthesizeChannelsData(track);
        track.GetResult(chunk);
      }
      else
      {
        assert(!"SoundTracker: invalid iterator access");
        chunk = Devices::AYM::DataChunk();
      }
    }
  private:
    void SwitchToNewLine()
    {
      assert(0 == State->Quirk());
      if (const Track::Line* line = Data->Patterns[State->Pattern()].GetLine(State->Line()))
      {
        if (const Track::Line::Chan& src = line->Channels[0])
        {
          StateA.SetNewState(src);
        }
        if (const Track::Line::Chan& src = line->Channels[1])
        {
          StateB.SetNewState(src);
        }
        if (const Track::Line::Chan& src = line->Channels[2])
        {
          StateC.SetNewState(src);
        }
      }
    }

    void GetNewLineState(AYM::TrackBuilder& track) const
    {
      if (const Track::Line* line = Data->Patterns[State->Pattern()].GetLine(State->Line()))
      {
        if (const Track::Line::Chan& src = line->Channels[0])
        {
          ProcessEnvelopeCommands(src.Commands, track);
        }
        if (const Track::Line::Chan& src = line->Channels[1])
        {
          ProcessEnvelopeCommands(src.Commands, track);
        }
        if (const Track::Line::Chan& src = line->Channels[2])
        {
          ProcessEnvelopeCommands(src.Commands, track);
        }
      }
    }

    void SynthesizeChannelsData(AYM::TrackBuilder& track) const
    {
      const uint_t transposition = Data->Transpositions[State->Position()];
      {
        ChannelBuilder channel(transposition, track, 0);
        StateA.Synthesize(channel);
      }
      {
        ChannelBuilder channel(transposition, track, 1);
        StateB.Synthesize(channel);
      }
      {
        ChannelBuilder channel(transposition, track, 2);
        StateC.Synthesize(channel);
      }
    }
  private:
    const AYM::TrackParameters::Ptr TrackParams;
    const StateIterator::Ptr Delegate;
    const TrackState::Ptr State;
    const ModuleData::Ptr Data;
    ChannelState StateA;
    ChannelState StateB;
    ChannelState StateC;
  };

  class Chiptune : public AYM::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties)
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
      const StateIterator::Ptr iter = CreateTrackStateIterator(Info, Data);
      return boost::make_shared<DataIterator>(trackParams, iter, Data);
    }
  private:
    const ModuleData::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };
}

namespace STC
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //hints
  const std::size_t MAX_MODULE_SIZE = 0x2500;

//////////////////////////////////////////////////////////////////////////
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Header
  {
    uint8_t Tempo;
    uint16_t PositionsOffset;
    uint16_t OrnamentsOffset;
    uint16_t PatternsOffset;
    char Identifier[18];
    uint16_t Size;
  } PACK_POST;

  PACK_PRE struct Positions
  {
    uint8_t Lenght;
    PACK_PRE struct PosEntry
    {
      uint8_t PatternNum;
      int8_t PatternHeight;

      bool Check() const
      {
        return in_range<uint_t>(PatternNum, 1, SoundTracker::MAX_PATTERNS_COUNT);
      }
    } PACK_POST;
    PosEntry Data[1];
  } PACK_POST;

  PACK_PRE struct Pattern
  {
    uint8_t Number;
    boost::array<uint16_t, Devices::AYM::CHANNELS> Offsets;

    bool Check() const
    {
      return in_range<uint_t>(Number, 1, SoundTracker::MAX_PATTERNS_COUNT);
    }

    uint_t GetIndex() const
    {
      return Number - 1;
    }
  } PACK_POST;

  PACK_PRE struct Ornament
  {
    uint8_t Number;
    int8_t Data[SoundTracker::SAMPLE_ORNAMENT_SIZE];

    bool Check() const
    {
      return in_range<uint_t>(Number, 0, SoundTracker::MAX_ORNAMENTS_COUNT - 1);
    }
  } PACK_POST;

  PACK_PRE struct Sample
  {
    uint8_t Number;
    // EEEEaaaa
    // NESnnnnn
    // eeeeeeee
    // Ee - effect
    // a - level
    // N - noise mask, n- noise value
    // E - envelope mask
    // S - effect sign
    PACK_PRE struct Line
    {
      uint8_t EffHiAndLevel;
      uint8_t NoiseAndMasks;
      uint8_t EffLo;

      uint_t GetLevel() const
      {
        return EffHiAndLevel & 15;
      }

      int_t GetEffect() const
      {
        const int_t val = int_t(EffHiAndLevel & 240) * 16 + EffLo;
        return (NoiseAndMasks & 32) ? val : -val;
      }

      uint_t GetNoise() const
      {
        return NoiseAndMasks & 31;
      }

      bool GetNoiseMask() const
      {
        return 0 != (NoiseAndMasks & 128);
      }

      bool GetEnvelopeMask() const
      {
        return 0 != (NoiseAndMasks & 64);
      }
    } PACK_POST;
    Line Data[SoundTracker::SAMPLE_ORNAMENT_SIZE];
    uint8_t Loop;
    uint8_t LoopSize;

    bool Check() const
    {
      return in_range<uint_t>(Number, 0, SoundTracker::MAX_SAMPLES_COUNT - 1);
    }
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(Header) == 27);
  BOOST_STATIC_ASSERT(sizeof(Positions) == 3);
  BOOST_STATIC_ASSERT(sizeof(Pattern) == 7);
  BOOST_STATIC_ASSERT(sizeof(Ornament) == 33);
  BOOST_STATIC_ASSERT(sizeof(Sample) == 99);

  typedef PatternCursorSet<Devices::AYM::CHANNELS> PatternCursors;

  class Areas
  {
  public:
    enum AreaTypes
    {
      HEADER,
      SAMPLES,
      POSITIONS,
      ORNAMENTS,
      PATTERNS,
      END
    };

    explicit Areas(const IO::FastDump& data)
      : Data(data)
    {
      const Header* const header(safe_ptr_cast<const Header*>(Data.Data()));

      Regions.AddArea(HEADER, 0);
      Regions.AddArea(SAMPLES, sizeof(*header));
      Regions.AddArea(POSITIONS, fromLE(header->PositionsOffset));
      Regions.AddArea(ORNAMENTS, fromLE(header->OrnamentsOffset));
      Regions.AddArea(PATTERNS, fromLE(header->PatternsOffset));
      Regions.AddArea(END, static_cast<uint_t>(data.Size()));
    }

    const IO::FastDump& GetOriginalData() const
    {
      return Data;
    }

    const Header& GetHeader() const
    {
      return *safe_ptr_cast<const Header*>(Data.Data());
    }

    RangeIterator<const Sample*> GetSamples() const
    {
      return GetIterator<Sample>(SAMPLES);
    }

    uint_t GetSamplesLimit() const
    {
      return GetLimit<Sample>(SAMPLES);
    }

    RangeIterator<const Ornament*> GetOrnaments() const
    {
      return GetIterator<Ornament>(ORNAMENTS);
    }

    uint_t GetOrnamentsLimit() const
    {
      return GetLimit<Ornament>(ORNAMENTS);
    }

    RangeIterator<const Positions::PosEntry*> GetPositions() const
    {
      const uint_t offset = Regions.GetAreaAddress(POSITIONS);
      const Positions* const positions = safe_ptr_cast<const Positions*>(&Data[offset]);
      const Positions::PosEntry* const entry = positions->Data;
      return RangeIterator<const Positions::PosEntry*>(entry, entry + positions->Lenght + 1);
    }

    uint_t GetPositionsLimit() const
    {
      const uint_t offset = Regions.GetAreaAddress(POSITIONS);
      const Positions* const positions = safe_ptr_cast<const Positions*>(&Data[offset]);
      const uint_t size = sizeof(*positions) + sizeof(positions->Data[0]) * positions->Lenght;
      return offset + size;
    }

    RangeIterator<const Pattern*> GetPatterns() const
    {
      return GetIterator<Pattern>(PATTERNS);
    }
  private:
    template<class T>
    RangeIterator<const T*> GetIterator(AreaTypes id) const
    {
      const uint_t offset = Regions.GetAreaAddress(id);
      const uint_t count = Regions.GetAreaSize(id) / sizeof(T);
      const T* const begin = safe_ptr_cast<const T*>(&Data[offset]);
      return RangeIterator<const T*>(begin, begin + count);
    }

    template<class T>
    uint_t GetLimit(AreaTypes id) const
    {
      const uint_t offset = Regions.GetAreaAddress(id);
      const uint_t count = Regions.GetAreaSize(id) / sizeof(T);
      return offset + count * sizeof(T);
    }
  protected:
    const IO::FastDump& Data;
    AreaController<AreaTypes, 1 + END, uint_t> Regions;
  };

  class ModuleData : public SoundTracker::ModuleData
  {
  public:
    typedef boost::shared_ptr<ModuleData> Ptr;

    void ParseInformation(const Areas& areas, ModuleProperties& properties)
    {
      const Header& header = areas.GetHeader();
      InitialTempo = header.Tempo;
      properties.SetProgram(OptimizeString(FromCharArray(header.Identifier)));
      properties.SetFreqtable(TABLE_SOUNDTRACKER);
    }

    uint_t ParseSamples(const Areas& areas)
    {
      Samples.resize(SoundTracker::MAX_SAMPLES_COUNT);
      for (RangeIterator<const Sample*> iter = areas.GetSamples(); iter; ++iter)
      {
        const Sample& sample = *iter;
        if (sample.Check())
        {
          AddSample(sample);
        }
      }
      return areas.GetSamplesLimit();
    }

    uint_t ParseOrnaments(const Areas& areas)
    {
      Ornaments.resize(SoundTracker::MAX_ORNAMENTS_COUNT);
      for (RangeIterator<const Ornament*> iter = areas.GetOrnaments(); iter; ++iter)
      {
        const Ornament& ornament = *iter;
        if (ornament.Check())
        {
          AddOrnament(ornament);
        }
      }
      return areas.GetOrnamentsLimit();
    }

    uint_t ParsePositions(const Areas& areas)
    {
      assert(!Patterns.empty());
      for (RangeIterator<const Positions::PosEntry*> iter = areas.GetPositions(); iter && iter->Check(); ++iter)
      {
        const Positions::PosEntry& entry = *iter;
        AddPosition(entry);
      }
      assert(!Positions.empty());
      return areas.GetPositionsLimit();
    }

    uint_t ParsePatterns(const Areas& areas)
    {
      assert(!Samples.empty());
      assert(!Ornaments.empty());
      uint_t resLimit = 0;
      Patterns.resize(SoundTracker::MAX_PATTERNS_COUNT);
      for (RangeIterator<const Pattern*> iter = areas.GetPatterns(); iter && iter->Check(); ++iter)
      {
        const Pattern& pattern = *iter;

        SoundTracker::Track::Pattern& dst(Patterns[pattern.GetIndex()]);
        const uint_t patLimit = ParsePattern(areas.GetOriginalData(), pattern, dst);
        resLimit = std::max(resLimit, patLimit);
      }
      return resLimit;
    }
  private:
    void AddSample(const Sample& sample)
    {
      assert(sample.Check());
      std::vector<SoundTracker::Sample::Line> lines(ArraySize(sample.Data));
      std::transform(sample.Data, ArrayEnd(sample.Data), lines.begin(), &BuildSampleLine);
      const uint_t loop = std::min<uint_t>(sample.Loop, static_cast<uint_t>(lines.size()));
      const uint_t loopLimit = std::min<uint_t>(sample.Loop + sample.LoopSize + 1, static_cast<uint_t>(lines.size()));
      Samples[sample.Number] = SoundTracker::Sample(loop, loopLimit, lines.begin(), lines.end());
    }

    static SoundTracker::Sample::Line BuildSampleLine(const Sample::Line& line)
    {
      SoundTracker::Sample::Line res;
      res.Level = line.GetLevel();
      res.Noise = line.GetNoise();
      res.NoiseMask = line.GetNoiseMask();
      res.EnvelopeMask = line.GetEnvelopeMask();
      res.Effect = line.GetEffect();
      return res;
    }

    void AddOrnament(const Ornament& ornament)
    {
      assert(ornament.Check());
      Ornaments[ornament.Number] =
        SoundTracker::Track::Ornament(static_cast<uint_t>(ArraySize(ornament.Data)), ornament.Data, ArrayEnd(ornament.Data));
    }

    void AddPosition(const Positions::PosEntry& entry)
    {
      assert(entry.Check());
      Positions.push_back(entry.PatternNum - 1);
      Transpositions.push_back(entry.PatternHeight);
    }

    uint_t ParsePattern(const IO::FastDump& data, const Pattern& pattern, SoundTracker::Track::Pattern& dst)
    {
      AYM::PatternCursors cursors;
      std::transform(pattern.Offsets.begin(), pattern.Offsets.end(), cursors.begin(), &fromLE<uint16_t>);
      uint_t& channelACursor = cursors.front().Offset;
      do
      {
        SoundTracker::Track::Line& line = dst.AddLine();
        ParsePatternLine(data, cursors, line);
        //skip lines
        if (const uint_t linesToSkip = cursors.GetMinCounter())
        {
          cursors.SkipLines(linesToSkip);
          dst.AddLines(linesToSkip);
        }
      }
      while (channelACursor < data.Size() &&
             (0xff != data[channelACursor] ||
             0 != cursors.front().Counter));
      const uint_t maxOffset = 1 + cursors.GetMaxOffset();
      return maxOffset;
    }

    void ParsePatternLine(const IO::FastDump& data, AYM::PatternCursors& cursors, SoundTracker::Track::Line& line)
    {
      assert(line.Channels.size() == cursors.size());
      SoundTracker::Track::Line::ChannelsArray::iterator channel(line.Channels.begin());
      for (AYM::PatternCursors::iterator cur = cursors.begin(); cur != cursors.end(); ++cur, ++channel)
      {
        ParsePatternLineChannel(data, *cur, *channel);
      }
    }

    void ParsePatternLineChannel(const IO::FastDump& data, PatternCursor& cur, SoundTracker::Track::Line::Chan& channel)
    {
      if (cur.Counter--)
      {
        return;//has to skip
      }

      for (const std::size_t dataSize = data.Size(); cur.Offset < dataSize;)
      {
        const uint_t cmd(data[cur.Offset++]);
        const std::size_t restbytes = dataSize - cur.Offset;
        //ornament==0 and sample==0 are valid - no ornament and no sample respectively
        //ornament==0 and sample==0 are valid - no ornament and no sample respectively
        if (cmd <= 0x5f)//note
        {
          channel.SetNote(cmd);
          channel.SetEnabled(true);
          break;
        }
        else if (cmd >= 0x60 && cmd <= 0x6f)//sample
        {
          const uint_t num = cmd - 0x60;
          channel.SetSample(num);
        }
        else if (cmd >= 0x70 && cmd <= 0x7f)//ornament
        {
          channel.Commands.push_back(SoundTracker::Track::Command(SoundTracker::NOENVELOPE));
          const uint_t num = cmd - 0x70;
          channel.SetOrnament(num);
        }
        else if (cmd == 0x80)//reset
        {
          channel.SetEnabled(false);
          break;
        }
        else if (cmd == 0x81)//empty
        {
          break;
        }
        else if (cmd >= 0x82 && cmd <= 0x8e)//orn 0, without/with envelope
        {
          channel.SetOrnament(0);
          if (cmd == 0x82)
          {
            channel.Commands.push_back(SoundTracker::Track::Command(SoundTracker::NOENVELOPE));
          }
          else if (!restbytes)
          {
            throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
          }
          else
          {
            channel.Commands.push_back(SoundTracker::Track::Command(SoundTracker::ENVELOPE, cmd - 0x80, data[cur.Offset++]));
          }
        }
        else //skip
        {
          cur.Period = (cmd - 0xa1) & 0xff;
        }
      }
      cur.Counter = cur.Period;
    }
  };

  class AreasChecker : public Areas
  {
  public:
    explicit AreasChecker(const IO::FastDump& data)
      : Areas(data)
    {
    }

    bool CheckLayout() const
    {
      return sizeof(Header) == Regions.GetAreaSize(Areas::HEADER) &&
             Regions.Undefined == Regions.GetAreaSize(Areas::END);
    }

    bool CheckHeader() const
    {
      const Header& header = GetHeader();
      if (!in_range<uint_t>(header.Tempo, 1, 15))
      {
        return false;
      }
      //do not check header.Size
      return true;
    }

    bool CheckSamples() const
    {
      const uint_t size = Regions.GetAreaSize(Areas::SAMPLES);
      return 0 != size &&
             Regions.Undefined != size &&
             0 == size % sizeof(Sample);
    }

    bool CheckOrnaments() const
    {
      const uint_t size = Regions.GetAreaSize(Areas::ORNAMENTS);
      return 0 != size &&
             Regions.Undefined != size &&
             0 == size % sizeof(Ornament);
    }

    bool CheckPositions() const
    {
      const uint_t size = Regions.GetAreaSize(Areas::POSITIONS);
      if (0 == size ||
          Regions.Undefined == size ||
          0 != (size - 1) % sizeof(Positions::PosEntry))
      {
        return false;
      }
      uint_t positions = 0;
      for (RangeIterator<const Positions::PosEntry*> iter = GetPositions(); iter && iter->Check(); ++iter)
      {
        ++positions;
      }
      if (!positions)
      {
        return false;
      }
      const uint_t limit = Regions.GetAreaAddress(Areas::END);
      return GetPositionsLimit() <= limit;
    }

    bool CheckPatterns() const
    {
      const uint_t size = Regions.GetAreaSize(Areas::PATTERNS);
      if (0 == size ||
          Regions.Undefined == size)
      {
        return false;
      }
      const uint_t limit = Regions.GetAreaAddress(Areas::END);
      uint_t usedPatterns = 0;
      for (RangeIterator<const Pattern*> pattern = GetPatterns(); pattern && pattern->Check(); ++pattern)
      {
        const uint_t patNum = pattern->GetIndex();
        const uint_t patMask = 1 << patNum;
        if (usedPatterns & patMask)
        {
          return false;
        }
        if (pattern->Offsets.end() != std::find_if(pattern->Offsets.begin(), pattern->Offsets.end(),
          boost::bind(&fromLE<uint16_t>, _1) > limit - 1))
        {
          return false;
        }
        usedPatterns |= patMask;
      }
      return usedPatterns != 0;
    }
  };

  bool CheckModule(const uint8_t* data, std::size_t size)
  {
    const std::size_t limit = std::min(size, MAX_MODULE_SIZE);
    if (limit < sizeof(Header))
    {
      return false;
    }

    const IO::FastDump dump(data, limit);
    const AreasChecker areas(dump);

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

namespace STC
{
  using namespace ZXTune;

  //plugin attributes
  const Char ID[] = {'S', 'T', 'C', 0};
  const Char* const INFO = Text::STC_PLUGIN_INFO;

  const std::string STC_FORMAT(
    "01-0f"       // uint8_t Tempo; 1..15
    "?00-25"      // uint16_t PositionsOffset; 0..MAX_MODULE_SIZE
    "?00-25"      // uint16_t OrnamentsOffset; 0..MAX_MODULE_SIZE
    "?00-25"      // uint16_t PatternsOffset; 0..MAX_MODULE_SIZE
  );

  //////////////////////////////////////////////////////////////////////////
  class Factory : public ModulesFactory
  {
  public:
    Factory()
      : Format(DataFormat::Create(STC_FORMAT))
    {
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t size = inputData.Size();
      return Format->Match(data, size) && STC::CheckModule(data, size);
    }

    virtual DataFormat::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Module::Holder::Ptr CreateModule(Module::ModuleProperties::RWPtr properties, Parameters::Accessor::Ptr parameters, IO::DataContainer::Ptr allData, std::size_t& usedSize) const
    {
      try
      {
        assert(Check(*allData));

        //assume that data is ok
        const IO::FastDump& data = IO::FastDump(*allData, 0, STC::MAX_MODULE_SIZE);
        const STC::Areas areas(data);

        const STC::ModuleData::Ptr parsedData = boost::make_shared<STC::ModuleData>();
        parsedData->ParseInformation(areas, *properties);
        const uint_t smpLim = parsedData->ParseSamples(areas);
        const uint_t ornLim = parsedData->ParseOrnaments(areas);
        const uint_t patLim = parsedData->ParsePatterns(areas);
        const uint_t posLim = parsedData->ParsePositions(areas);

        const std::size_t maxLim = std::max(std::max(smpLim, ornLim), std::max(patLim, posLim));
        usedSize = std::min(data.Size(), maxLim);

        //meta properties
        {
          const ModuleRegion fixedRegion(sizeof(STC::Header), usedSize - sizeof(STC::Header));
          properties->SetSource(usedSize, fixedRegion);
        }

        const Module::AYM::Chiptune::Ptr chiptune = boost::make_shared<SoundTracker::Chiptune>(parsedData, properties);
        return Module::AYM::CreateHolder(chiptune, parameters);
      }
      catch (const Error&/*e*/)
      {
        Log::Debug("Core::STCSupp", "Failed to create holder");
      }
      return Module::Holder::Ptr();
    }
  private:
    const DataFormat::Ptr Format;
  };
}


namespace ST11
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Sample
  {
    uint8_t Volume[SoundTracker::SAMPLE_ORNAMENT_SIZE];
    uint8_t Noise[SoundTracker::SAMPLE_ORNAMENT_SIZE];
    uint16_t Effect[SoundTracker::SAMPLE_ORNAMENT_SIZE];
    uint8_t Loop;
    uint8_t LoopSize;
  } PACK_POST;

  PACK_PRE struct PosEntry
  {
    uint8_t Pattern;
    int8_t Transposition;
  } PACK_POST;

  PACK_PRE struct Ornament
  {
    int8_t Offsets[SoundTracker::SAMPLE_ORNAMENT_SIZE];
  } PACK_POST;

  PACK_PRE struct Pattern
  {
    PACK_PRE struct Line
    {
      PACK_PRE struct Channel
      {
        //RNNN#OOO
        uint8_t Note;
        //SSSSEEEE
        uint8_t EffectSample;
        //EEEEeeee
        uint8_t EffectOrnament;

        bool IsEmpty() const
        {
          return 0 == (Note & 128) && 0 == (Note & 0x70) && 0 == EffectSample;
        }
      } PACK_POST;

      Channel Channels[3];

      bool IsEmpty() const
      {
        return Channels[0].IsEmpty() && Channels[1].IsEmpty() && Channels[2].IsEmpty();
      }
    } PACK_POST;

    Line Lines[SoundTracker::MAX_PATTERN_SIZE];
  } PACK_POST;

  PACK_PRE struct Header
  {
    Sample Samples[15];
    PosEntry Positions[256];
    uint8_t Lenght;
    Ornament Ornaments[17];
    uint8_t Tempo;
    uint8_t PatternsSize;
    Pattern Patterns[1];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(Sample) == 130);
  BOOST_STATIC_ASSERT(sizeof(PosEntry) == 2);
  BOOST_STATIC_ASSERT(sizeof(Ornament) == 32);
  BOOST_STATIC_ASSERT(sizeof(Pattern) == 576);
  BOOST_STATIC_ASSERT(sizeof(Header) == 3009 + 576);
  BOOST_STATIC_ASSERT(offsetof(Header, Positions) == 1950);
  BOOST_STATIC_ASSERT(offsetof(Header, Lenght) == 2462);
  BOOST_STATIC_ASSERT(offsetof(Header, Tempo) == 3007);
  BOOST_STATIC_ASSERT(offsetof(Header, Patterns) == 3009);

  class ModuleData : public SoundTracker::ModuleData
  {
  public:
    typedef boost::shared_ptr<ModuleData> Ptr;

    void ParseInformation(const Header& header, ModuleProperties& properties)
    {
      InitialTempo = header.Tempo;
      properties.SetProgram(Text::ST_EDITOR);
      properties.SetFreqtable(TABLE_SOUNDTRACKER);
    }

    void ParseSamples(const Header& header)
    {
      Samples.resize(1);
      std::for_each(header.Samples, ArrayEnd(header.Samples), boost::bind(&ModuleData::AddSample, this, _1));
    }

    void ParseOrnaments(const Header& header)
    {
      Ornaments.clear();
      std::for_each(header.Ornaments, ArrayEnd(header.Ornaments), boost::bind(&ModuleData::AddOrnament, this, _1));
    }

    std::size_t ParsePositionsAndPatterns(const Header& header)
    {
      const uint_t patternsCount = ParsePositions(header);
      ParsePatterns(header, patternsCount);
      return sizeof(Header) + (patternsCount - 1) * sizeof(Pattern);
    }
  private:
    void AddSample(const Sample& sample)
    {
      std::vector<SoundTracker::Sample::Line> lines;
      for (uint_t idx = 0; idx < SoundTracker::SAMPLE_ORNAMENT_SIZE; ++idx)
      {
        SoundTracker::Sample::Line res;
        res.Level = sample.Volume[idx];
        res.Noise = sample.Noise[idx] & 31;
        res.NoiseMask = 0 != (sample.Noise[idx] & 128);
        res.EnvelopeMask = 0 != (sample.Noise[idx] & 64);
        const int16_t eff = fromLE(sample.Effect[idx]);
        res.Effect = 0 != (eff & 0x1000) ? (eff & 0xfff) : -(eff & 0xfff);
        lines.push_back(res);
      }
      const uint_t loop = std::min<uint_t>(sample.Loop, static_cast<uint_t>(lines.size()));
      const uint_t loopLimit = std::min<uint_t>(sample.Loop + sample.LoopSize + 1, static_cast<uint_t>(lines.size()));
      Samples.push_back(SoundTracker::Sample(loop, loopLimit, lines.begin(), lines.end()));
    }

    void AddOrnament(const Ornament& ornament)
    {
      Ornaments.push_back(SoundTracker::Track::Ornament(0, ornament.Offsets, ArrayEnd(ornament.Offsets)));
    }

    uint_t ParsePositions(const Header& header)
    {
      const std::size_t posCount = header.Lenght + 1;
      Positions.resize(posCount);
      std::transform(header.Positions, header.Positions + posCount, Positions.begin(), 
        boost::bind(std::minus<uint_t>(), boost::bind(&PosEntry::Pattern, _1), uint_t(1)));
      Transpositions.resize(posCount);
      std::transform(header.Positions, header.Positions + posCount, Transpositions.begin(), boost::mem_fn(&PosEntry::Transposition));
      return 1 + *std::max_element(Positions.begin(), Positions.end());
    }

    void ParsePatterns(const Header& header, uint_t count)
    {
      Patterns.clear();
      const uint_t patSize = header.PatternsSize;
      std::for_each(header.Patterns, header.Patterns + count, boost::bind(&ModuleData::AddPattern, this, _1, patSize));
    }

    void AddPattern(const Pattern& pattern, uint_t patSize)
    {
      SoundTracker::Track::Pattern result;
      uint_t linesToSkip = 0;
      for (uint_t idx = 0; idx < patSize; ++idx)
      {
        const Pattern::Line& srcLine = pattern.Lines[idx];
        if (srcLine.IsEmpty())
        {
          ++linesToSkip;
          continue;
        }
        if (linesToSkip)
        {
          result.AddLines(linesToSkip);
          linesToSkip = 0;
        }
        SoundTracker::Track::Line& dstLine = result.AddLine();
        ConvertLine(srcLine, dstLine);
      }
      Patterns.push_back(result);
    }

    void ConvertLine(const Pattern::Line& srcLine, SoundTracker::Track::Line& dstLine)
    {
      for (uint_t chan = 0; chan < dstLine.Channels.size(); ++chan)
      {
        ConvertChannel(srcLine.Channels[chan], dstLine.Channels[chan]);
      }
    }

    void ConvertChannel(const Pattern::Line::Channel& srcChan, SoundTracker::Track::Line::Chan& dstChan)
    {
      if (srcChan.IsEmpty())
      {
        return;
      }
      if (0 != (srcChan.Note & 128))
      {
        dstChan.SetEnabled(false);
        return;
      }
      else if (srcChan.Note)
      {
        dstChan.SetNote(ConvertNote(srcChan.Note));
        dstChan.SetEnabled(true);
      }
      if (uint_t sample = srcChan.EffectSample >> 4)
      {
        dstChan.SetSample(sample);
      }
      switch (const uint_t effect = (srcChan.EffectSample & 15))
      {
      case 15:
        dstChan.SetOrnament(srcChan.EffectOrnament & 15);
        dstChan.Commands.push_back(SoundTracker::Track::Command(SoundTracker::NOENVELOPE));
        break;
      default:
        if (effect && srcChan.EffectOrnament)
        {
          dstChan.SetOrnament(0);
          dstChan.Commands.push_back(SoundTracker::Track::Command(SoundTracker::ENVELOPE, effect, srcChan.EffectOrnament));
        }
        break;
      }
    }

    uint_t ConvertNote(uint8_t note)
    {
      //                                 A   B  C  D  E  F  G
      static const uint_t halftones[] = {9, 11, 0, 2, 4, 5, 7};
      const uint_t NOTES_PER_OCTAVE = 12;
      const uint_t octave = note & 7;
      const bool flat = 0 != (note & 8);
      const uint_t name = (note & 0x70) >> 4;
      assert(name);
      return halftones[name - 1] + flat + NOTES_PER_OCTAVE * octave;
    }
  };

  bool CheckModule(const void* data, std::size_t size)
  {
    if (size < sizeof(Header))
    {
      return false;
    }
    const Header& header = *static_cast<const Header*>(data);
    std::vector<uint_t> positions(header.Lenght + 1);
    std::transform(header.Positions, header.Positions + positions.size(), positions.begin(), boost::mem_fn(&PosEntry::Pattern));
    if (0 == *std::min_element(positions.begin(), positions.end()))
    {
      return false;
    }
    const std::size_t patternsCount = *std::max_element(positions.begin(), positions.end());
    if (size < sizeof(Header) + sizeof(Pattern) * (patternsCount - 1))
    {
      return false;
    }
    return true;
  }
}

namespace ST11
{
  using namespace ZXTune;

  //plugin attributes
  const Char ID[] = {'S', 'T', '1', '1', 0};
  const Char* const INFO = Text::ST11_PLUGIN_INFO;

  const std::string ST11_FORMAT(
    //samples
    "("
      //levels
      "0x{32}"
      //noises
      "%xx0xxxxx{32}"
      //additions
      "(?%000xxxxx){32}"
      //loop, loop limit
      "00-1f{2}"
    "){15}"
    //positions
    "(01-20?){256}"
    //length
    "?"
    //ornaments
    "(?{32}){17}"
    //delay
    "01-0f"
    //patterns size
    "01-40"
  );

  //////////////////////////////////////////////////////////////////////////
  class Factory : public ModulesFactory
  {
  public:
    Factory()
      : Format(DataFormat::Create(ST11_FORMAT))
    {
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t size = inputData.Size();
      return Format->Match(data, size) && ST11::CheckModule(data, size);
    }

    virtual DataFormat::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Module::Holder::Ptr CreateModule(Module::ModuleProperties::RWPtr properties, Parameters::Accessor::Ptr parameters, IO::DataContainer::Ptr allData, std::size_t& usedSize) const
    {
      try
      {
        assert(Check(*allData));

        //assume that data is ok
        const ST11::Header& header = *static_cast<const ST11::Header*>(allData->Data());

        const ST11::ModuleData::Ptr parsedData = boost::make_shared<ST11::ModuleData>();
        parsedData->ParseInformation(header, *properties);
        parsedData->ParseSamples(header);
        parsedData->ParseOrnaments(header);
        usedSize = parsedData->ParsePositionsAndPatterns(header);
        //meta properties
        {
          const std::size_t fixedOffset = sizeof(ST11::Header) - sizeof(ST11::Pattern);
          const ModuleRegion fixedRegion(fixedOffset, usedSize - fixedOffset);
          properties->SetSource(usedSize, fixedRegion);
        }
        const Module::AYM::Chiptune::Ptr chiptune = boost::make_shared<SoundTracker::Chiptune>(parsedData, properties);
        return Module::AYM::CreateHolder(chiptune, parameters);
      }
      catch (const Error& /*e*/)
      {
        Log::Debug("Core::ST11Supp", "Failed to create holder");
      }
      return Module::Holder::Ptr();
    }
  private:
    const DataFormat::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterSTCSupport(PluginsRegistrator& registrator)
  {
    const ModulesFactory::Ptr factory = boost::make_shared<STC::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(STC::ID, STC::INFO, VERSION, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }

  void RegisterST11Support(PluginsRegistrator& registrator)
  {
    const ModulesFactory::Ptr factory = boost::make_shared<ST11::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ST11::ID, ST11::INFO, VERSION, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
