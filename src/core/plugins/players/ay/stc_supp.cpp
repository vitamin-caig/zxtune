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
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>
#include <core/text/warnings.h>

#define FILE_TAG D9281D1D

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char STC_PLUGIN_ID[] = {'S', 'T', 'C', 0};
  const String STC_PLUGIN_VERSION(FromStdString("$Rev$"));

  //hints
  const std::size_t MAX_MODULE_SIZE = 16384;
  const uint_t MAX_SAMPLES_COUNT = 16;
  const uint_t MAX_ORNAMENTS_COUNT = 16;
  const uint_t MAX_PATTERN_SIZE = 64;
  const uint_t MAX_PATTERNS_COUNT = 32;
  const uint_t SAMPLE_ORNAMENT_SIZE = 32;

//////////////////////////////////////////////////////////////////////////
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct STCHeader
  {
    uint8_t Tempo;
    uint16_t PositionsOffset;
    uint16_t OrnamentsOffset;
    uint16_t PatternsOffset;
    char Identifier[18];
    uint16_t Size;
  } PACK_POST;

  PACK_PRE struct STCPositions
  {
    uint8_t Lenght;
    PACK_PRE struct STCPosEntry
    {
      uint8_t PatternNum;
      int8_t PatternHeight;

      bool Check() const
      {
        return in_range<uint_t>(PatternNum, 1, MAX_PATTERNS_COUNT);
      }
    } PACK_POST;
    STCPosEntry Data[1];
  } PACK_POST;

  PACK_PRE struct STCPattern
  {
    uint8_t Number;
    boost::array<uint16_t, Devices::AYM::CHANNELS> Offsets;

    bool Check() const
    {
      return in_range<uint_t>(Number, 1, MAX_PATTERNS_COUNT);
    }

    uint_t GetIndex() const
    {
      return Number - 1;
    }
  } PACK_POST;

  PACK_PRE struct STCOrnament
  {
    uint8_t Number;
    int8_t Data[SAMPLE_ORNAMENT_SIZE];

    bool Check() const
    {
      return in_range<uint_t>(Number, 0, MAX_ORNAMENTS_COUNT - 1);
    }
  } PACK_POST;

  PACK_PRE struct STCSample
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
    Line Data[SAMPLE_ORNAMENT_SIZE];
    uint8_t Loop;
    uint8_t LoopSize;

    bool Check() const
    {
      return in_range<uint_t>(Number, 0, MAX_SAMPLES_COUNT - 1);
    }
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(STCHeader) == 27);
  BOOST_STATIC_ASSERT(sizeof(STCPositions) == 3);
  BOOST_STATIC_ASSERT(sizeof(STCPattern) == 7);
  BOOST_STATIC_ASSERT(sizeof(STCOrnament) == 33);
  BOOST_STATIC_ASSERT(sizeof(STCSample) == 99);

  // currently used sample
  struct Sample
  {
    Sample() : Loop(), LoopLimit(), Lines()
    {
    }

    explicit Sample(const STCSample& sample)
      : Loop(std::min<uint_t>(sample.Loop, static_cast<uint_t>(ArraySize(sample.Data))))
      , LoopLimit(std::min<uint_t>(sample.Loop + sample.LoopSize + 1, static_cast<uint_t>(ArraySize(sample.Data))))
      , Lines(sample.Data, ArrayEnd(sample.Data))
    {
    }

    struct Line
    {
      Line() : Level(), Noise(), NoiseMask(), EnvelopeMask(), Effect()
      {
      }

      /*explicit*/Line(const STCSample::Line& line)
        : Level(line.GetLevel()), Noise(line.GetNoise()), NoiseMask(line.GetNoiseMask())
        , EnvelopeMask(line.GetEnvelopeMask()), Effect(line.GetEffect())
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

  typedef TrackingSupport<Devices::AYM::CHANNELS, CmdType, Sample> STCTrack;
  typedef std::vector<int_t> STCTransposition;

  typedef PatternCursorSet<Devices::AYM::CHANNELS> PatternCursors;

  class STCAreas
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

    explicit STCAreas(const IO::FastDump& data)
      : Data(data)
    {
      const STCHeader* const header(safe_ptr_cast<const STCHeader*>(Data.Data()));

      Areas.AddArea(HEADER, 0);
      Areas.AddArea(SAMPLES, sizeof(*header));
      Areas.AddArea(POSITIONS, fromLE(header->PositionsOffset));
      Areas.AddArea(ORNAMENTS, fromLE(header->OrnamentsOffset));
      Areas.AddArea(PATTERNS, fromLE(header->PatternsOffset));
      Areas.AddArea(END, static_cast<uint_t>(data.Size()));
    }

    const IO::FastDump& GetOriginalData() const
    {
      return Data;
    }

    const STCHeader& GetHeader() const
    {
      return *safe_ptr_cast<const STCHeader*>(Data.Data());
    }

    RangeIterator<const STCSample*> GetSamples() const
    {
      return GetIterator<STCSample>(SAMPLES);
    }

    uint_t GetSamplesLimit() const
    {
      return GetLimit<STCSample>(SAMPLES);
    }

    RangeIterator<const STCOrnament*> GetOrnaments() const
    {
      return GetIterator<STCOrnament>(ORNAMENTS);
    }

    uint_t GetOrnamentsLimit() const
    {
      return GetLimit<STCOrnament>(ORNAMENTS);
    }

    RangeIterator<const STCPositions::STCPosEntry*> GetPositions() const
    {
      const uint_t offset = Areas.GetAreaAddress(POSITIONS);
      const STCPositions* const positions = safe_ptr_cast<const STCPositions*>(&Data[offset]);
      const STCPositions::STCPosEntry* const entry = positions->Data;
      return RangeIterator<const STCPositions::STCPosEntry*>(entry, entry + positions->Lenght + 1);
    }

    uint_t GetPositionsLimit() const
    {
      const uint_t offset = Areas.GetAreaAddress(POSITIONS);
      const STCPositions* const positions = safe_ptr_cast<const STCPositions*>(&Data[offset]);
      const uint_t size = sizeof(*positions) + sizeof(positions->Data[0]) * positions->Lenght;
      return offset + size;
    }

    RangeIterator<const STCPattern*> GetPatterns() const
    {
      return GetIterator<STCPattern>(PATTERNS);
    }
  private:
    template<class T>
    RangeIterator<const T*> GetIterator(AreaTypes id) const
    {
      const uint_t offset = Areas.GetAreaAddress(id);
      const uint_t count = Areas.GetAreaSize(id) / sizeof(T);
      const T* const begin = safe_ptr_cast<const T*>(&Data[offset]);
      return RangeIterator<const T*>(begin, begin + count);
    }

    template<class T>
    uint_t GetLimit(AreaTypes id) const
    {
      const uint_t offset = Areas.GetAreaAddress(id);
      const uint_t count = Areas.GetAreaSize(id) / sizeof(T);
      return offset + count * sizeof(T);
    }
  protected:
    const IO::FastDump& Data;
    AreaController<AreaTypes, 1 + END, uint_t> Areas;
  };

  struct STCModuleData : public STCTrack::ModuleData
  {
    typedef boost::shared_ptr<STCModuleData> RWPtr;
    typedef boost::shared_ptr<const STCModuleData> Ptr;

    STCModuleData()
      : STCTrack::ModuleData()
    {
    }

    void ParseInformation(const STCAreas& areas, ModuleProperties& properties)
    {
      const STCHeader& header = areas.GetHeader();
      InitialTempo = header.Tempo;
      properties.SetProgram(OptimizeString(FromCharArray(header.Identifier)));
    }

    uint_t ParseSamples(const STCAreas& areas)
    {
      Samples.resize(MAX_SAMPLES_COUNT);
      for (RangeIterator<const STCSample*> iter = areas.GetSamples(); iter; ++iter)
      {
        const STCSample& sample = *iter;
        if (sample.Check())
        {
          AddSample(sample);
        }
      }
      return areas.GetSamplesLimit();
    }

    uint_t ParseOrnaments(const STCAreas& areas)
    {
      Ornaments.resize(MAX_ORNAMENTS_COUNT);
      for (RangeIterator<const STCOrnament*> iter = areas.GetOrnaments(); iter; ++iter)
      {
        const STCOrnament& ornament = *iter;
        if (ornament.Check())
        {
          AddOrnament(ornament);
        }
      }
      return areas.GetOrnamentsLimit();
    }

    uint_t ParsePositions(const STCAreas& areas)
    {
      assert(!Patterns.empty());
      for (RangeIterator<const STCPositions::STCPosEntry*> iter = areas.GetPositions(); iter && iter->Check(); ++iter)
      {
        const STCPositions::STCPosEntry& entry = *iter;
        AddPosition(entry);
      }
      assert(!Positions.empty());
      return areas.GetPositionsLimit();
    }

    uint_t ParsePatterns(const STCAreas& areas)
    {
      assert(!Samples.empty());
      assert(!Ornaments.empty());
      uint_t resLimit = 0;
      Patterns.resize(MAX_PATTERNS_COUNT);
      for (RangeIterator<const STCPattern*> iter = areas.GetPatterns(); iter && iter->Check(); ++iter)
      {
        const STCPattern& pattern = *iter;

        STCTrack::Pattern& dst(Patterns[pattern.GetIndex()]);
        const uint_t patLimit = ParsePattern(areas.GetOriginalData(), pattern, dst);
        resLimit = std::max(resLimit, patLimit);
      }
      return resLimit;
    }
  private:
    void AddSample(const STCSample& sample)
    {
      assert(sample.Check());
      Samples[sample.Number] = Sample(sample);
    }

    void AddOrnament(const STCOrnament& ornament)
    {
      assert(ornament.Check());
      Ornaments[ornament.Number] =
        STCTrack::Ornament(static_cast<uint_t>(ArraySize(ornament.Data)), ornament.Data, ArrayEnd(ornament.Data));
    }

    void AddPosition(const STCPositions::STCPosEntry& entry)
    {
      assert(entry.Check());
      Positions.push_back(entry.PatternNum - 1);
      Transpositions.push_back(entry.PatternHeight);
    }

    uint_t ParsePattern(const IO::FastDump& data, const STCPattern& pattern, STCTrack::Pattern& dst)
    {
      AYMPatternCursors cursors;
      std::transform(pattern.Offsets.begin(), pattern.Offsets.end(), cursors.begin(), &fromLE<uint16_t>);
      uint_t& channelACursor = cursors.front().Offset;
      do
      {
        STCTrack::Line& line = dst.AddLine();
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

    void ParsePatternLine(const IO::FastDump& data, AYMPatternCursors& cursors, STCTrack::Line& line)
    {
      assert(line.Channels.size() == cursors.size());
      STCTrack::Line::ChannelsArray::iterator channel(line.Channels.begin());
      for (AYMPatternCursors::iterator cur = cursors.begin(); cur != cursors.end(); ++cur, ++channel)
      {
        ParsePatternLineChannel(data, *cur, *channel);
      }
    }

    void ParsePatternLineChannel(const IO::FastDump& data, PatternCursor& cur, STCTrack::Line::Chan& channel)
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
          channel.Commands.push_back(STCTrack::Command(NOENVELOPE));
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
            channel.Commands.push_back(STCTrack::Command(NOENVELOPE));
          }
          else if (!restbytes)
          {
            throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
          }
          else
          {
            channel.Commands.push_back(STCTrack::Command(ENVELOPE, cmd - 0x80, data[cur.Offset++]));
          }
        }
        else //skip
        {
          cur.Period = (cmd - 0xa1) & 0xff;
        }
      }
      cur.Counter = cur.Period;
    }
  public:
    STCTransposition Transpositions;
  };

  Renderer::Ptr CreateSTCRenderer(AYM::TrackParameters::Ptr params, Information::Ptr info, STCModuleData::Ptr data, Devices::AYM::Chip::Ptr device);

  class STCHolder : public Holder
                  , private ConversionFactory
  {
  public:
    STCHolder(ModuleProperties::Ptr properties, Parameters::Accessor::Ptr parameters, IO::DataContainer::Ptr allData, std::size_t& usedSize)
      : Data(boost::make_shared<STCModuleData>())
      , Properties(properties)
      , Info(CreateTrackInfo(Data, Devices::AYM::CHANNELS))
      , Params(parameters)
    {
      //assume that data is ok
      const IO::FastDump& data = IO::FastDump(*allData, 0, MAX_MODULE_SIZE);
      const STCAreas areas(data);

      Data->ParseInformation(areas, *Properties);
      const uint_t smpLim = Data->ParseSamples(areas);
      const uint_t ornLim = Data->ParseOrnaments(areas);
      const uint_t patLim = Data->ParsePatterns(areas);
      const uint_t posLim = Data->ParsePositions(areas);

      const std::size_t maxLim = std::max(std::max(smpLim, ornLim), std::max(patLim, posLim));
      usedSize = std::min(data.Size(), maxLim);

      //meta properties
      {
        const ModuleRegion fixedRegion(sizeof(STCHeader), usedSize - sizeof(STCHeader));
        Properties->SetSource(usedSize, fixedRegion);
      }
      Properties->SetFreqtable(TABLE_SOUNDTRACKER);
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return Properties->GetPlugin();
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Parameters::CreateMergedAccessor(Params, Properties);
    }

    virtual Renderer::Ptr CreateRenderer(Sound::MultichannelReceiver::Ptr target) const
    {
      const Parameters::Accessor::Ptr params = GetModuleProperties();

      const AYM::TrackParameters::Ptr trackParams = AYM::TrackParameters::Create(params);
      const Devices::AYM::Receiver::Ptr receiver = CreateAYMReceiver(trackParams, target);
      const Devices::AYM::ChipParameters::Ptr chipParams = AYM::CreateChipParameters(params);
      const Devices::AYM::Chip::Ptr chip = Devices::AYM::CreateChip(chipParams, receiver);
      return CreateSTCRenderer(trackParams, Info, Data, chip);
    }

    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      Error result;
      if (parameter_cast<RawConvertParam>(&param))
      {
        Properties->GetData(dst);
      }
      else if (!ConvertAYMFormat(param, *this, dst, result))
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return result;
    }
  private:
    virtual Information::Ptr GetInformation() const
    {
      return GetModuleInformation();
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return GetModuleProperties();
    }

    virtual Renderer::Ptr CreateRenderer(Devices::AYM::Chip::Ptr chip) const
    {
      const Parameters::Accessor::Ptr params = GetModuleProperties();

      const AYM::TrackParameters::Ptr trackParams = AYM::TrackParameters::Create(params);
      return CreateSTCRenderer(trackParams, Info, Data, chip);
    }
  private:
    const STCModuleData::RWPtr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Params;
  };

  class STCChannelBuilder
  {
  public:
    STCChannelBuilder(uint_t transposition, const AYM::TrackBuilder& track, uint_t chanNum)
      : Transposition(transposition)
      , Track(track)
      , Channel(Track.GetChannel(chanNum))
    {
    }

    void SetLevel(uint_t level) const
    {
      Channel.SetLevel(level);
    }

    void EnableEnvelope() const
    {
      Channel.EnableEnvelope();
    }

    void SetTone(int_t halftones, int_t offset) const
    {
      Channel.SetTone(halftones + Transposition, offset);
    }

    void DisableTone() const
    {
      Channel.DisableTone();
    }

    void SetNoise(int_t level) const
    {
      Track.SetNoise(level);
    }

    void DisableNoise() const
    {
      Channel.DisableNoise();
    }

  private:
    const uint_t Transposition;
    const AYM::TrackBuilder& Track;
    const AYM::ChannelBuilder Channel;
  };

  struct STCChannelState
  {
    explicit STCChannelState(STCModuleData::Ptr data)
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

    void SetNewState(const STCTrack::Line::Chan& src)
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

    void Synthesize(const STCChannelBuilder& channel) const
    {
      if (!Enabled)
      {
        channel.SetLevel(0);
        return;
      }

      const STCTrack::Sample::Line& curSampleLine = CurSample->GetLine(PosInSample);

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
    static const STCTrack::Sample* GetStubSample()
    {
      static const STCTrack::Sample stubSample;
      return &stubSample;
    }

    static const STCTrack::Ornament* GetStubOrnament()
    {
      static const STCTrack::Ornament stubOrnament;
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

    void ApplyCommands(const STCTrack::CommandsArray& commands)
    {
      std::for_each(commands.begin(), commands.end(), boost::bind(&STCChannelState::ApplyCommand, this, _1));
    }

    void ApplyCommand(const STCTrack::Command& command)
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
    const STCModuleData::Ptr Data;
    bool Enabled;
    bool Envelope;
    uint_t Note;
    const STCTrack::Sample* CurSample;
    uint_t PosInSample;
    bool LoopedInSample;
    const STCTrack::Ornament* CurOrnament;
  };

  void ProcessEnvelopeCommands(const STCTrack::CommandsArray& commands, const AYM::TrackBuilder& track)
  {
    const STCTrack::CommandsArray::const_iterator it = std::find(commands.begin(), commands.end(), ENVELOPE);
    if (it != commands.end())
    {
      track.SetEnvelopeType(it->Param1);
      track.SetEnvelopeTone(it->Param2);
    }
  }

  class STCDataRenderer : public AYMDataRenderer
  {
  public:
    explicit STCDataRenderer(STCModuleData::Ptr data)
      : Data(data)
      , StateA(Data)
      , StateB(Data)
      , StateC(Data)
    {
    }

    virtual void Reset()
    {
      StateA.Reset();
      StateB.Reset();
      StateC.Reset();
    }

    virtual void SynthesizeData(const TrackState& state, const AYM::TrackBuilder& track)
    {
      if (0 == state.Quirk())
      {
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(state, track);
      StateA.Iterate();
      StateB.Iterate();
      StateC.Iterate();
    }

  private:
    void GetNewLineState(const TrackState& state, const AYM::TrackBuilder& track)
    {
      if (const STCTrack::Line* line = Data->Patterns[state.Pattern()].GetLine(state.Line()))
      {
        if (const STCTrack::Line::Chan& src = line->Channels[0])
        {
          StateA.SetNewState(src);
          ProcessEnvelopeCommands(src.Commands, track);
        }
        if (const STCTrack::Line::Chan& src = line->Channels[1])
        {
          StateB.SetNewState(src);
          ProcessEnvelopeCommands(src.Commands, track);
        }
        if (const STCTrack::Line::Chan& src = line->Channels[2])
        {
          StateC.SetNewState(src);
          ProcessEnvelopeCommands(src.Commands, track);
        }
      }
    }

    void SynthesizeChannelsData(const TrackState& state, const AYM::TrackBuilder& track)
    {
      const uint_t transposition = Data->Transpositions[state.Position()];
      {
        STCChannelBuilder channel(transposition, track, 0);
        StateA.Synthesize(channel);
      }
      {
        STCChannelBuilder channel(transposition, track, 1);
        StateB.Synthesize(channel);
      }
      {
        STCChannelBuilder channel(transposition, track, 2);
        StateC.Synthesize(channel);
      }
    }
  private:
    const STCModuleData::Ptr Data;
    STCChannelState StateA;
    STCChannelState StateB;
    STCChannelState StateC;
  };

  Renderer::Ptr CreateSTCRenderer(AYM::TrackParameters::Ptr params, Information::Ptr info, STCModuleData::Ptr data, Devices::AYM::Chip::Ptr device)
  {
    const AYMDataRenderer::Ptr renderer = boost::make_shared<STCDataRenderer>(data);
    return CreateAYMTrackRenderer(params, info, data, renderer, device);
  }

  class STCAreasChecker : public STCAreas
  {
  public:
    explicit STCAreasChecker(const IO::FastDump& data)
      : STCAreas(data)
    {
    }

    bool CheckLayout() const
    {
      return sizeof(STCHeader) == Areas.GetAreaSize(STCAreas::HEADER) &&
             Areas.Undefined == Areas.GetAreaSize(STCAreas::END);
    }

    bool CheckHeader() const
    {
      const STCHeader& header = GetHeader();
      if (!in_range<uint_t>(header.Tempo, 1, 15))
      {
        return false;
      }
      //do not check header.Size
      return true;
    }

    bool CheckSamples() const
    {
      const uint_t size = Areas.GetAreaSize(STCAreas::SAMPLES);
      return 0 != size &&
             Areas.Undefined != size &&
             0 == size % sizeof(STCSample);
    }

    bool CheckOrnaments() const
    {
      const uint_t size = Areas.GetAreaSize(STCAreas::ORNAMENTS);
      return 0 != size &&
             Areas.Undefined != size &&
             0 == size % sizeof(STCOrnament);
    }

    bool CheckPositions() const
    {
      const uint_t size = Areas.GetAreaSize(STCAreas::POSITIONS);
      if (0 == size ||
          Areas.Undefined == size ||
          0 != (size - 1) % sizeof(STCPositions::STCPosEntry))
      {
        return false;
      }
      uint_t positions = 0;
      for (RangeIterator<const STCPositions::STCPosEntry*> iter = GetPositions(); iter && iter->Check(); ++iter)
      {
        ++positions;
      }
      if (!positions)
      {
        return false;
      }
      const uint_t limit = Areas.GetAreaAddress(STCAreas::END);
      return GetPositionsLimit() <= limit;
    }

    bool CheckPatterns() const
    {
      const uint_t size = Areas.GetAreaSize(STCAreas::PATTERNS);
      if (0 == size ||
          Areas.Undefined == size)
      {
        return false;
      }
      const uint_t limit = Areas.GetAreaAddress(STCAreas::END);
      uint_t usedPatterns = 0;
      for (RangeIterator<const STCPattern*> pattern = GetPatterns(); pattern && pattern->Check(); ++pattern)
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

  bool CheckSTCModule(const uint8_t* data, std::size_t size)
  {
    const std::size_t limit = std::min(size, MAX_MODULE_SIZE);
    if (limit < sizeof(STCHeader))
    {
      return false;
    }

    const IO::FastDump dump(data, limit);
    const STCAreasChecker areas(dump);

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

  const std::string STC_FORMAT(
    "01-0f"       // uint8_t Tempo; 1..15
    "?00-3f"      // uint16_t PositionsOffset; 0..3fff
    "?00-3f"      // uint16_t OrnamentsOffset; 0..3fff
    "?00-3f"      // uint16_t PatternsOffset; 0..3fff
  );

  //////////////////////////////////////////////////////////////////////////
  class STCPlugin : public PlayerPlugin
                  , public ModulesFactory
                  , public boost::enable_shared_from_this<STCPlugin>
  {
  public:
    typedef boost::shared_ptr<const STCPlugin> Ptr;
    
    STCPlugin()
      : Format(DataFormat::Create(STC_FORMAT))
    {
    }

    virtual String Id() const
    {
      return STC_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::STC_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return STC_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | GetSupportedAYMFormatConvertors();
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t size = inputData.Size();
      return Format->Match(data, size) && CheckSTCModule(data, size);
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      const STCPlugin::Ptr self = shared_from_this();
      return DetectModuleInLocation(self, self, inputData, callback);
    }
  private:
    virtual DataFormat::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Holder::Ptr CreateModule(ModuleProperties::Ptr properties, Parameters::Accessor::Ptr parameters, IO::DataContainer::Ptr data, std::size_t& usedSize) const
    {
      try
      {
        assert(Check(*data));
        const Holder::Ptr holder(new STCHolder(properties, parameters, data, usedSize));
        return holder;
      }
      catch (const Error&/*e*/)
      {
        Log::Debug("Core::STCSupp", "Failed to create holder");
      }
      return Holder::Ptr();
    }
  private:
    const DataFormat::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterSTCSupport(PluginsRegistrator& registrator)
  {
    const PlayerPlugin::Ptr plugin(new STCPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
