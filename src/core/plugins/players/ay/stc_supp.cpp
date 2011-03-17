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
#include "core/plugins/detect_helper.h"
#include "core/plugins/utils.h"
#include "core/plugins/registrator.h"
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
#include <boost/bind.hpp>
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

  //detectors
  static const DataPrefixChecker PLAYERS[] =
  {
    DataPrefixChecker
    (
      "21??"    // ld hl,xxxx
      "c3??"    // jp xxxx
      "c3??"    // jp xxxx
      "f3"      // di
      "7e"      // ld a,(hl)
      "32??"    // ld (xxxx),a
      "22??"    // ld (xxxx),hl
      "23"      // inc hl
      "cd??"    // call xxxx
      "1a"      // ld a,(de)
      "13"      // inc de
      "3c"      // inc a
      "32??"    // ld (xxxx),a
      "ed53??"  // ld (xxxx),de
      "cd??"    // call xxxx
      "ed53??"  // ld (xxxx),de
      "d5"      // push de
      "cd??"    // call xxxx
      "ed53??"  // ld (xxxx),de
      "21??"    // ld hl,xxxx
      "cd??"    // call xxxx
      "eb"      // ex de,hl
      "22??"    // ld (xxxx),hl
      "21??"    // ld hl,xxxx
      "22??"    // ld(xxxx),hl
      "21??"    // ld hl,xxxx
      "11??"    // ld de,xxxx
      "01??"    // ld bc,xxxx
      "70"      // ld (hl),b
      "edb0"    // ldir
      "e1"      // pop hl
      "01??"    // ld bc,xxxx
      "af"      // xor a
      "cd??"    // call xxxx
      "3d"      // dec a
      "32??"    // ld (xxxx),a
      "32??"    // ld (xxxx),a
      "32??"    // ld (xxxx),a
      "3e?"     // ld a,x
      "32??"    // ld (xxxx),a
      "23"      // inc hl
      "22??"    // ld (xxxx),hl
      "22??"    // ld (xxxx),hl
      "22??"    // ld (xxxx),hl
      "cd??"    // call xxxx
      "fb"      // ei
      "c9"      // ret
      ,
      0x43c
    )
  };


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
    boost::array<uint16_t, AYM::CHANNELS> Offsets;

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
      : Loop(std::min<uint_t>(sample.Loop, ArraySize(sample.Data)))
      , LoopLimit(std::min<uint_t>(sample.Loop + sample.LoopSize + 1, ArraySize(sample.Data)))
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
      return Lines.size();
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

  typedef TrackingSupport<AYM::CHANNELS, CmdType, Sample> STCTrack;
  typedef std::vector<int_t> STCTransposition;

  typedef PatternCursorSet<AYM::CHANNELS> PatternCursors;

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
      Areas.AddArea(END, data.Size());
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

    ModuleProperties::Ptr ParseInformation(const STCAreas& areas)
    {
      const STCHeader& header = areas.GetHeader();
      InitialTempo = header.Tempo;
      const ModuleProperties::Ptr props = ModuleProperties::Create(STC_PLUGIN_ID);
      props->SetProgram(OptimizeString(FromCharArray(header.Identifier)));
      return props;
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
        STCTrack::Ornament(ArraySize(ornament.Data), ornament.Data, ArrayEnd(ornament.Data));
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

  Player::Ptr CreateSTCPlayer(Information::Ptr info, STCModuleData::Ptr data, AYM::Chip::Ptr device);

  class STCHolder : public Holder
  {
  public:
    STCHolder(Plugin::Ptr plugin, Parameters::Accessor::Ptr parameters, const DataLocation& location, ModuleRegion& region)
      : SrcPlugin(plugin)
      , Data(boost::make_shared<STCModuleData>())
      , Info(TrackInfo::Create(Data))
    {
      //assume that data is ok
      const IO::DataContainer::Ptr allData = location.GetData();
      const IO::FastDump& data = IO::FastDump(*allData, region.Offset, MAX_MODULE_SIZE);
      const STCAreas areas(data);

      const ModuleProperties::Ptr props = Data->ParseInformation(areas);
      const uint_t smpLim = Data->ParseSamples(areas);
      const uint_t ornLim = Data->ParseOrnaments(areas);
      const uint_t patLim = Data->ParsePatterns(areas);
      const uint_t posLim = Data->ParsePositions(areas);

      const std::size_t maxLim = std::max(std::max(smpLim, ornLim), std::max(patLim, posLim));
      //fill region
      region.Size = std::min(data.Size(), maxLim);

      RawData = region.Extract(*allData);

      //meta properties
      {
        const ModuleRegion fixedRegion(sizeof(STCHeader), region.Size - sizeof(STCHeader));
        props->SetSource(RawData, fixedRegion);
      }
      props->SetPlugins(location.GetPlugins());
      props->SetPath(location.GetPath());

      Info->SetLogicalChannels(AYM::LOGICAL_CHANNELS);
      Info->SetModuleProperties(Parameters::CreateMergedAccessor(parameters, props));
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return SrcPlugin;
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Player::Ptr CreatePlayer() const
    {
      return CreateSTCPlayer(Info, Data, AYM::CreateChip());
    }

    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      Error result;
      if (parameter_cast<RawConvertParam>(&param))
      {
        const uint8_t* const data = static_cast<const uint8_t*>(RawData->Data());
        dst.assign(data, data + RawData->Size());
      }
      else if (!ConvertAYMFormat(boost::bind(&CreateSTCPlayer, boost::cref(Info), boost::cref(Data), _1),
        param, dst, result))
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return result;
    }
  private:
    const Plugin::Ptr SrcPlugin;
    const STCModuleData::RWPtr Data;
    const TrackInfo::Ptr Info;
    IO::DataContainer::Ptr RawData;
  };

  class STCChannelSynthesizer
  {
  public:
    STCChannelSynthesizer(uint_t transposition, AYMTrackSynthesizer& synth, uint_t chanNum)
      : Transposition(transposition)
      , MainSynth(synth)
      , ChanSynth(synth.GetChannel(chanNum))
    {
    }

    void SetLevel(uint_t level)
    {
      ChanSynth.SetLevel(level);
    }

    void EnableEnvelope()
    {
      ChanSynth.EnableEnvelope();
    }

    void SetTone(int_t halftones, int_t offset)
    {
      ChanSynth.SetTone(halftones + Transposition, offset);
    }

    void DisableTone()
    {
      ChanSynth.DisableTone();
    }

    void SetNoise(int_t level)
    {
      MainSynth.SetNoise(level);
    }

    void DisableNoise()
    {
      ChanSynth.DisableNoise();
    }

  private:
    const uint_t Transposition;
    AYMTrackSynthesizer& MainSynth;
    const AYMChannelSynthesizer ChanSynth;
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

    void Synthesize(STCChannelSynthesizer& synthesizer) const
    {
      if (!Enabled)
      {
        synthesizer.SetLevel(0);
        return;
      }

      const STCTrack::Sample::Line& curSampleLine = CurSample->GetLine(PosInSample);

      //apply level
      synthesizer.SetLevel(curSampleLine.Level);
      //apply envelope
      if (Envelope)
      {
        synthesizer.EnableEnvelope();
      }
      //apply tone
      const int_t halftones = int_t(Note) + CurOrnament->GetLine(PosInSample);
      if (!curSampleLine.EnvelopeMask)
      {
        synthesizer.SetTone(halftones, curSampleLine.Effect);
      }
      else
      {
        synthesizer.DisableTone();
      }
      //apply noise
      if (!curSampleLine.NoiseMask)
      {
        synthesizer.SetNoise(curSampleLine.Noise);
      }
      else
      {
        synthesizer.DisableNoise();
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

  void ProcessEnvelopeCommands(const STCTrack::CommandsArray& commands, AYMTrackSynthesizer& synthesizer)
  {
    const STCTrack::CommandsArray::const_iterator it = std::find(commands.begin(), commands.end(), ENVELOPE);
    if (it != commands.end())
    {
      synthesizer.SetEnvelopeType(it->Param1);
      synthesizer.SetEnvelopeTone(it->Param2);
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

    virtual void SynthesizeData(const TrackState& state, AYMTrackSynthesizer& synthesizer)
    {
      if (0 == state.Quirk())
      {
        GetNewLineState(state, synthesizer);
      }
      SynthesizeChannelsData(state, synthesizer);
      StateA.Iterate();
      StateB.Iterate();
      StateC.Iterate();
    }

  private:
    void GetNewLineState(const TrackState& state, AYMTrackSynthesizer& synthesizer)
    {
      if (const STCTrack::Line* line = Data->Patterns[state.Pattern()].GetLine(state.Line()))
      {
        if (const STCTrack::Line::Chan& src = line->Channels[0])
        {
          StateA.SetNewState(src);
          ProcessEnvelopeCommands(src.Commands, synthesizer);
        }
        if (const STCTrack::Line::Chan& src = line->Channels[1])
        {
          StateB.SetNewState(src);
          ProcessEnvelopeCommands(src.Commands, synthesizer);
        }
        if (const STCTrack::Line::Chan& src = line->Channels[2])
        {
          StateC.SetNewState(src);
          ProcessEnvelopeCommands(src.Commands, synthesizer);
        }
      }
    }

    void SynthesizeChannelsData(const TrackState& state, AYMTrackSynthesizer& synthesizer)
    {
      const uint_t transposition = Data->Transpositions[state.Position()];
      {
        STCChannelSynthesizer synth(transposition, synthesizer, 0);
        StateA.Synthesize(synth);
      }
      {
        STCChannelSynthesizer synth(transposition, synthesizer, 1);
        StateB.Synthesize(synth);
      }
      {
        STCChannelSynthesizer synth(transposition, synthesizer, 2);
        StateC.Synthesize(synth);
      }
    }
  private:
    const STCModuleData::Ptr Data;
    STCChannelState StateA;
    STCChannelState StateB;
    STCChannelState StateC;
  };

  Player::Ptr CreateSTCPlayer(Information::Ptr info, STCModuleData::Ptr data, AYM::Chip::Ptr device)
  {
    const AYMDataRenderer::Ptr renderer = boost::make_shared<STCDataRenderer>(data);
    return CreateAYMTrackPlayer(info, data, renderer, device, TABLE_SOUNDTRACKER);
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

  //////////////////////////////////////////////////////////////////////////
  class STCPlugin : public PlayerPlugin
                  , public boost::enable_shared_from_this<STCPlugin>
                  , private ModuleDetector
  {
  public:
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
      return CheckDataFormat(*this, inputData);
    }

    Module::Holder::Ptr CreateModule(Parameters::Accessor::Ptr parameters, const DataLocation& location, std::size_t& usedSize) const
    {
      return CreateModuleFromData(*this, parameters, location, usedSize);
    }
  private:
    virtual DataPrefixIterator GetPrefixes() const
    {
      return DataPrefixIterator(PLAYERS, ArrayEnd(PLAYERS));
    }

    virtual bool CheckData(const uint8_t* data, std::size_t size) const
    {
      return CheckSTCModule(data, size);
    }

    virtual Holder::Ptr TryToCreateModule(Parameters::Accessor::Ptr parameters,
      const DataLocation& location, ModuleRegion& region) const
    {
      const Plugin::Ptr plugin = shared_from_this();
      try
      {
        const Holder::Ptr holder(new STCHolder(plugin, parameters, location, region));
#ifdef SELF_TEST
        holder->CreatePlayer();
#endif
        return holder;
      }
      catch (const Error&/*e*/)
      {
        Log::Debug("Core::STCSupp", "Failed to create holder");
      }
      return Holder::Ptr();
    }
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
