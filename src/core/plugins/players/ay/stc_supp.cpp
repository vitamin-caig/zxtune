/*
Abstract:
  STC modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_conversion.h"
#include "soundtracker.h"
#include "core/plugins/utils.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/creation_result.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <range_checker.h>
#include <tools.h>
//library includes
#include <core/error_codes.h>
#include <core/plugin_attrs.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

#define FILE_TAG D9281D1D

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
             dst.GetSize() <= SoundTracker::MAX_PATTERN_SIZE &&
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
  const String VERSION(FromStdString("$Rev$"));
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | Module::GetSupportedAYMFormatConvertors();

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
      : Format(Binary::Format::Create(STC_FORMAT))
    {
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t size = inputData.Size();
      return Format->Match(data, size) && STC::CheckModule(data, size);
    }

    virtual Binary::Format::Ptr GetFormat() const
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

        const std::size_t minLim = std::min(std::min(smpLim, ornLim), std::min(patLim, posLim));
        const std::size_t maxLim = std::max(std::max(smpLim, ornLim), std::max(patLim, posLim));
        usedSize = std::min(data.Size(), maxLim);

        //meta properties
        {
          const std::size_t fixedStart = posLim == minLim ? posLim : sizeof(STC::Header);
          const ModuleRegion fixedRegion(fixedStart, usedSize - fixedStart);
          properties->SetSource(usedSize, fixedRegion);
        }

        const Module::AYM::Chiptune::Ptr chiptune = SoundTracker::CreateChiptune(parsedData, properties);
        return Module::AYM::CreateHolder(chiptune, parameters);
      }
      catch (const Error&/*e*/)
      {
        Log::Debug("Core::STCSupp", "Failed to create holder");
      }
      return Module::Holder::Ptr();
    }
  private:
    const Binary::Format::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterSTCSupport(PluginsRegistrator& registrator)
  {
    const ModulesFactory::Ptr factory = boost::make_shared<STC::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(STC::ID, STC::INFO, STC::VERSION, STC::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
