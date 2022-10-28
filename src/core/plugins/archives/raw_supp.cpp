/**
 *
 * @file
 *
 * @brief  Scanner plugin implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/archives/raw_supp.h"
#include "core/plugins/archives/archived.h"
#include <core/plugins/archive_plugins_registrator.h>
#include <core/plugins/archives/l10n.h>
#include <core/plugins/players/plugin.h>
// common includes
#include <error_tools.h>
#include <make_ptr.h>
#include <progress_callback.h>
// library includes
#include <binary/container.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <debug/log.h>
#include <math/scale.h>
#include <strings/prefixed_index.h>
#include <time/duration.h>
#include <time/serialize.h>
#include <time/timer.h>
// std includes
#include <array>
#include <list>
#include <map>

#define FILE_TAG 7E0CBD98

namespace ZXTune
{
  template<std::size_t Fields>
  class StatisticBuilder
  {
  public:
    typedef std::array<String, Fields> Line;

    StatisticBuilder()
      : Lines()
      , Widths()
    {}

    void Add(const Line& line, std::size_t pos)
    {
      for (uint_t idx = 0; idx != Fields; ++idx)
      {
        Widths[idx] = std::max(Widths[idx], line[idx].size());
      }
      if (pos < Lines.size())
      {
        Lines[pos] = line;
      }
      else
      {
        Lines.resize(pos);
        Lines.push_back(line);
      }
    }

    String Get() const
    {
      String res;
      for (const auto& line : Lines)
      {
        res += ToString(line);
      }
      return res;
    }

  private:
    String ToString(const Line& line) const
    {
      String res;
      for (uint_t idx = 0; idx != Fields; ++idx)
      {
        res += Widen(line[idx], Widths[idx] + 1);
      }
      *res.rbegin() = '\n';
      return res;
    }

    static String Widen(const String& str, std::size_t wid)
    {
      String res(wid, ' ');
      std::copy(str.begin(), str.end(), res.begin());
      return res;
    }

  private:
    std::vector<Line> Lines;
    std::array<std::size_t, Fields> Widths;
  };

  class Statistic
  {
    using TimeUnit = Time::Timer::NativeUnit;

  public:
    Statistic()
      : TotalData(0)
      , ArchivedData(0)
      , ModulesData(0)
    {}

    ~Statistic()
    {
      const Debug::Stream Dbg("Core::RawScaner::Statistic");
      const auto spent = Timer.Elapsed();
      Dbg("Total processed: %1%", TotalData);
      Dbg("Time spent: %1%", Time::ToString(spent));
      const uint64_t useful = ArchivedData + ModulesData;
      Dbg("Useful detected: %1% (%2% archived + %3% modules)", useful, ArchivedData, ModulesData);
      Dbg("Coverage: %1%%%", useful * 100 / TotalData);
      Dbg("Speed: %1% b/s", spent.Get() ? (TotalData * spent.PER_SECOND / spent.Get()) : TotalData);
      StatisticBuilder<7> builder;
      builder.Add(MakeStatLine(), 0);
      StatItem total;
      total.Name = "Total";
      for (DetectMap::const_iterator it = Detection.begin(), lim = Detection.end(); it != lim; ++it)
      {
        builder.Add(MakeStatLine(it->second), 1 + it->second.Index);
        total += it->second;
      }
      builder.Add(MakeStatLine(total), 1 + Detection.size());
      Dbg(builder.Get().c_str());
    }

    void Enqueue(std::size_t size)
    {
      TotalData += size;
    }

    void AddArchived(std::size_t size)
    {
      ArchivedData += size;
    }

    void AddModule(std::size_t size)
    {
      ModulesData += size;
    }

    template<class PluginType>
    void AddAimed(const PluginType& plug, const Time::Timer& scanTimer)
    {
      StatItem& item = GetStat(plug);
      ++item.Aimed;
      item.AimedTime += scanTimer.Elapsed() + item.ScanTime;
      item.ScanTime = {};
    }

    template<class PluginType>
    void AddMissed(const PluginType& plug, const Time::Timer& scanTimer)
    {
      StatItem& item = GetStat(plug);
      ++item.Missed;
      item.MissedTime += scanTimer.Elapsed() + item.ScanTime;
      item.ScanTime = {};
    }

    template<class PluginType>
    void AddScanned(const PluginType& plug, const Time::Timer& scanTimer)
    {
      StatItem& item = GetStat(plug);
      item.ScanTime += scanTimer.Elapsed();
    }

    static Statistic& Self()
    {
      static Statistic self;
      return self;
    }

  private:
    struct StatItem
    {
      String Name;
      std::size_t Index;
      std::size_t Aimed;
      std::size_t Missed;
      Time::Duration<TimeUnit> AimedTime;
      Time::Duration<TimeUnit> MissedTime;
      Time::Duration<TimeUnit> ScanTime;

      StatItem()
        : Index()
        , Aimed()
        , Missed()
        , AimedTime()
        , MissedTime()
        , ScanTime()
      {}

      StatItem& operator+=(const StatItem& rh)
      {
        Aimed += rh.Aimed;
        Missed += rh.Missed;
        AimedTime += rh.AimedTime;
        MissedTime += rh.MissedTime;
        return *this;
      }
    };

    static std::array<String, 7> MakeStatLine()
    {
      std::array<String, 7> res;
      res[0] = "\nDetector";
      res[1] = "Missed detect";
      res[2] = "Total detect";
      res[3] = "DEff,%";
      res[4] = "Missed time,ms";
      res[5] = "Total time,ms";
      res[6] = "TEff,%";
      return res;
    }

    static std::array<String, 7> MakeStatLine(const StatItem& item)
    {
      std::array<String, 7> res;
      res[0] = item.Name;
      res[1] = std::to_string(item.Missed);
      res[2] = std::to_string(item.Aimed + item.Missed);
      res[3] = std::to_string(Percent(item.Aimed, item.Missed));
      res[4] = std::to_string(item.MissedTime.CastTo<Time::Millisecond>().Get());
      res[5] = std::to_string((item.MissedTime + item.AimedTime).CastTo<Time::Millisecond>().Get());
      res[6] = std::to_string(Percent(item.AimedTime.Get(), item.MissedTime.Get()));
      return res;
    }

    static uint_t Percent(uint_t aimed, uint_t missed)
    {
      if (const uint_t total = aimed + missed)
      {
        return static_cast<uint_t>(uint64_t(100) * aimed / total);
      }
      else
      {
        return 0;
      }
    }

    template<class PluginType>
    StatItem& GetStat(const PluginType& plug)
    {
      const void* const key = &plug;
      StatItem& res = Detection[key];
      if (res.Name.empty())
      {
        res.Name = plug.Description();
        res.Index = Detection.size() - 1;
      }
      return Detection[key];
    }

  private:
    const Time::Timer Timer;
    uint64_t TotalData;
    uint64_t ArchivedData;
    uint64_t ModulesData;
    typedef std::map<const void*, StatItem> DetectMap;
    DetectMap Detection;
  };
}  // namespace ZXTune

namespace ZXTune::Raw
{
  const Debug::Stream Dbg("Core::RawScaner");

  const auto PLUGIN_PREFIX = "+"_sv;

  const Char ID[] = {'R', 'A', 'W', 0};
  const Char* const INFO = ID;
  const uint_t CAPS = Capabilities::Category::CONTAINER | Capabilities::Container::Type::SCANER;

  const std::size_t SCAN_STEP = 1;
  const std::size_t MIN_MINIMAL_RAW_SIZE = 128;

  String CreateFilename(std::size_t offset)
  {
    assert(offset);
    return Strings::PrefixedIndex(PLUGIN_PREFIX, offset).ToString();
  }

  class PluginParameters
  {
  public:
    explicit PluginParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {}

    std::size_t GetMinimalSize() const
    {
      Parameters::IntType minRawSize = Parameters::ZXTune::Core::Plugins::Raw::MIN_SIZE_DEFAULT;
      if (Accessor.FindValue(Parameters::ZXTune::Core::Plugins::Raw::MIN_SIZE, minRawSize)
          && minRawSize < Parameters::IntType(MIN_MINIMAL_RAW_SIZE))
      {
        throw MakeFormattedError(THIS_LINE, translate("Specified minimal scan size (%1%). Should be more than %2%."),
                                 minRawSize, MIN_MINIMAL_RAW_SIZE);
      }
      return static_cast<std::size_t>(minRawSize);
    }

    bool GetDoubleAnalysis() const
    {
      Parameters::IntType doubleAnalysis = 0;
      Accessor.FindValue(Parameters::ZXTune::Core::Plugins::Raw::PLAIN_DOUBLE_ANALYSIS, doubleAnalysis);
      return doubleAnalysis != 0;
    }

  private:
    const Parameters::Accessor& Accessor;
  };

  class ScanProgress
  {
  public:
    ScanProgress(Log::ProgressCallback* delegate, std::size_t limit, const String& path)
      : Delegate(delegate)
      , ToPercent(limit, 100)
      , Text(ProgressMessage(ID, path))
    {}

    void Report(std::size_t offset)
    {
      if (Delegate)
      {
        Delegate->OnProgress(ToPercent(offset), Text);
      }
    }

  private:
    Log::ProgressCallback* const Delegate;
    const Math::ScaleFunctor<std::size_t> ToPercent;
    const String Text;
  };

  class ScanDataContainer : public Binary::Container
  {
  public:
    typedef std::shared_ptr<ScanDataContainer> Ptr;

    ScanDataContainer(Binary::Container::Ptr delegate, std::size_t offset)
      : Delegate(delegate)
      , OriginalSize(delegate->Size())
      , OriginalData(static_cast<const uint8_t*>(delegate->Start()))
      , Offset(offset)
    {}

    const void* Start() const override
    {
      return OriginalData + Offset;
    }

    std::size_t Size() const override
    {
      return OriginalSize - Offset;
    }

    Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
    {
      return Delegate->GetSubcontainer(offset + Offset, size);
    }

    bool HasToScan(std::size_t minSize) const
    {
      return Offset + minSize <= OriginalSize;
    }

    std::size_t GetOffset()
    {
      return Offset;
    }

    void Move(std::size_t step)
    {
      Offset += step;
    }

  private:
    const Binary::Container::Ptr Delegate;
    const std::size_t OriginalSize;
    const uint8_t* const OriginalData;
    std::size_t Offset;
  };

  class ScanDataLocation : public DataLocation
  {
  public:
    typedef std::shared_ptr<ScanDataLocation> Ptr;

    ScanDataLocation(DataLocation::Ptr parent, std::size_t offset)
      : Parent(std::move(parent))
      , Subdata(MakePtr<ScanDataContainer>(Parent->GetData(), offset))
    {}

    Binary::Container::Ptr GetData() const override
    {
      return Subdata;
    }

    Analysis::Path::Ptr GetPath() const override
    {
      const Analysis::Path::Ptr parentPath = Parent->GetPath();
      if (std::size_t offset = Subdata->GetOffset())
      {
        const auto subPath = CreateFilename(offset);
        return parentPath->Append(subPath);
      }
      return parentPath;
    }

    Analysis::Path::Ptr GetPluginsChain() const override
    {
      const Analysis::Path::Ptr parentPlugins = Parent->GetPluginsChain();
      if (Subdata->GetOffset())
      {
        return parentPlugins->Append(ID);
      }
      return parentPlugins;
    }

    bool HasToScan(std::size_t minSize) const
    {
      return Subdata->HasToScan(minSize);
    }

    std::size_t GetOffset()
    {
      return Subdata->GetOffset();
    }

    void Move(std::size_t step)
    {
      if (!Subdata.unique())
      {
        Dbg("Subdata is captured. Duplicate.");
        Subdata = MakePtr<ScanDataContainer>(Parent->GetData(), Subdata->GetOffset() + step);
      }
      else
      {
        Subdata->Move(step);
      }
    }

  private:
    const DataLocation::Ptr Parent;
    ScanDataContainer::Ptr Subdata;
  };

  class UnknownDataLocation : public DataLocation
  {
  public:
    UnknownDataLocation(DataLocation::Ptr parent, std::size_t start, std::size_t end)
      : Parent(std::move(parent))
      , Start(start)
      , End(end)
    {
      Dbg("Unknown data at %1%..%2%", Start, End);
    }

    Binary::Container::Ptr GetData() const override
    {
      return Parent->GetData()->GetSubcontainer(Start, End - Start);
    }

    Analysis::Path::Ptr GetPath() const override
    {
      return Parent->GetPath()->Append(CreateFilename(Start));
    }

    Analysis::Path::Ptr GetPluginsChain() const override
    {
      return Parent->GetPluginsChain()->Append(ID);
    }

  private:
    const DataLocation::Ptr Parent;
    const std::size_t Start;
    const std::size_t End;
  };

  template<class P>
  class LookaheadPluginsStorage
  {
    struct PluginEntry
    {
      typename P::Ptr Plugin;
      std::size_t Offset;

      explicit PluginEntry(typename P::Ptr plugin)
        : Plugin(std::move(plugin))
        , Offset()
      {}

      PluginEntry()
        : Offset()
      {}
    };
    typedef typename std::vector<PluginEntry> PluginsList;

  public:
    class Iterator
    {
    public:
      Iterator(typename PluginsList::iterator it, typename PluginsList::iterator lim, std::size_t offset)
        : Cur(std::move(it))
        , Lim(std::move(lim))
        , Offset(offset)
      {
        SkipUnaffected();
      }

      bool IsValid() const
      {
        return Cur != Lim;
      }

      const P& GetPlugin() const
      {
        assert(Cur != Lim);
        return *Cur->Plugin;
      }

      void SetLookahead(std::size_t offset)
      {
        assert(Cur != Lim);
        Cur->Offset += offset;
      }

      void Next()
      {
        assert(Cur != Lim);
        ++Cur;
        SkipUnaffected();
      }

    private:
      void SkipUnaffected()
      {
        while (Cur != Lim && Cur->Offset > Offset)
        {
          ++Cur;
        }
      }

    private:
      typename PluginsList::iterator Cur;
      const typename PluginsList::iterator Lim;
      const std::size_t Offset;
    };

    template<class Container>
    explicit LookaheadPluginsStorage(const Container& plugins)
      : Offset()
    {
      for (const auto& plugin : plugins)
      {
        if (plugin->Capabilities() != CAPS)
        {
          Plugins.emplace_back(PluginEntry(plugin));
        }
        else
        {
          Dbg("Ignore '%1%'", plugin->Description());
        }
      }
    }

    Iterator Enumerate()
    {
      return Iterator(Plugins.begin(), Plugins.end(), Offset);
    }

    std::size_t GetMinimalPluginLookahead() const
    {
      const auto it =
          std::min_element(Plugins.begin(), Plugins.end(),
                           [](const PluginEntry& lh, const PluginEntry& rh) { return lh.Offset < rh.Offset; });
      return it->Offset >= Offset ? it->Offset - Offset : 0;
    }

    void SetOffset(std::size_t offset)
    {
      Offset = offset;
    }

  private:
    std::size_t Offset;
    PluginsList Plugins;
  };

  class DoubleAnalyzedArchivePlugin : public ArchivePlugin
  {
  public:
    explicit DoubleAnalyzedArchivePlugin(ArchivePlugin::Ptr delegate)
      : Delegate(std::move(delegate))
    {}

    String Id() const override
    {
      return Delegate->Id();
    }

    String Description() const override
    {
      return Delegate->Description();
    }

    uint_t Capabilities() const override
    {
      return Delegate->Capabilities();
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Delegate->GetFormat();
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                                 ArchiveCallback& callback) const override
    {
      auto result = Delegate->Detect(params, std::move(inputData), callback);
      if (const auto matched = result->GetMatchedDataSize())
      {
        return Analysis::CreateUnmatchedResult(matched);
      }
      return result;
    }

    DataLocation::Ptr TryOpen(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                              const Analysis::Path& pathToOpen) const override
    {
      return Delegate->TryOpen(params, std::move(inputData), pathToOpen);
    }

  private:
    const ArchivePlugin::Ptr Delegate;
  };

  class DoubleAnalyzedArchives
  {
  public:
    static const std::vector<ArchivePlugin::Ptr>& GetPlugins()
    {
      static const DoubleAnalyzedArchives INSTANCE;
      return INSTANCE.Plugins;
    }

  private:
    DoubleAnalyzedArchives()
    {
      const auto& original = ArchivePlugin::Enumerate();
      Plugins.resize(original.size());
      std::transform(original.begin(), original.end(), Plugins.begin(), &MakeDoubleAnalyzed);
    }

    static ArchivePlugin::Ptr MakeDoubleAnalyzed(ArchivePlugin::Ptr plugin)
    {
      return 0 != (plugin->Capabilities() & Capabilities::Container::Traits::PLAIN)
                 ? MakePtr<DoubleAnalyzedArchivePlugin>(std::move(plugin))
                 : plugin;
    }

  private:
    std::vector<ArchivePlugin::Ptr> Plugins;
  };

  class RawDetectionPlugins
  {
  public:
    RawDetectionPlugins(const Parameters::Accessor& params, bool plainArchivesDoubleAnalysis)
      : Params(params)
      , Players(PlayerPlugin::Enumerate())
      , Archives(plainArchivesDoubleAnalysis ? DoubleAnalyzedArchives::GetPlugins() : ArchivePlugin::Enumerate())
      , Offset()
    {}

    std::pair<std::size_t, bool> Detect(DataLocation::Ptr input, ArchiveCallback& callback)
    {
      const auto detectedModules = DetectIn(Players, input, callback);
      if (const auto matched = detectedModules->GetMatchedDataSize())
      {
        Statistic::Self().AddModule(matched);
        return {matched, true};
      }
      const auto detectedArchives = DetectIn(Archives, input, callback);
      if (const auto matched = detectedArchives->GetMatchedDataSize())
      {
        Statistic::Self().AddArchived(matched);
        return {matched, true};
      }
      const auto archiveLookahead = detectedArchives->GetLookaheadOffset();
      const auto moduleLookahead = detectedModules->GetLookaheadOffset();
      Dbg("No archives for nearest %1% bytes, modules for %2% bytes", archiveLookahead, moduleLookahead);
      return {static_cast<std::size_t>(std::min(archiveLookahead, moduleLookahead)), false};
    }

    void SetOffset(std::size_t offset)
    {
      Offset = offset;
      Archives.SetOffset(offset);
      Players.SetOffset(offset);
    }

  private:
    template<class PluginType, class CallbackType>
    Analysis::Result::Ptr DetectIn(LookaheadPluginsStorage<PluginType>& container, DataLocation::Ptr input,
                                   CallbackType& callback) const
    {
      const bool initialScan = 0 == Offset;
      const std::size_t maxSize = input->GetData()->Size();
      for (auto iter = container.Enumerate(); iter.IsValid(); iter.Next())
      {
        const Time::Timer detectTimer;
        const auto& plugin = iter.GetPlugin();
        const auto result = plugin.Detect(Params, input, callback);
        const auto id = plugin.Id();
        if (const auto usedSize = result->GetMatchedDataSize())
        {
          Statistic::Self().AddAimed(plugin, detectTimer);
          Dbg("Detected %1% in %2% bytes at %3%.", id, usedSize, input->GetPath()->AsString());
          return result;
        }
        else if (!initialScan)
        {
          Statistic::Self().AddMissed(plugin, detectTimer);
          const Time::Timer scanTimer;
          const std::size_t lookahead = result->GetLookaheadOffset();
          iter.SetLookahead(lookahead);
          Dbg("Disabling check of %1% for neareast %2% bytes starting from %3%", id, lookahead, Offset);
          if (lookahead == maxSize)
          {
            Statistic::Self().AddAimed(plugin, scanTimer);
          }
          else
          {
            Statistic::Self().AddScanned(plugin, scanTimer);
          }
        }
      }
      const std::size_t minLookahead = initialScan ? std::size_t(1) : container.GetMinimalPluginLookahead();
      return Analysis::CreateUnmatchedResult(minLookahead);
    }

  private:
    const Parameters::Accessor& Params;
    LookaheadPluginsStorage<PlayerPlugin> Players;
    LookaheadPluginsStorage<ArchivePlugin> Archives;
    std::size_t Offset;
  };

  class Scaner : public ArchivePlugin
  {
  public:
    Scaner() = default;

    String Id() const override
    {
      return ID;
    }

    String Description() const override
    {
      return INFO;
    }

    uint_t Capabilities() const override
    {
      return CAPS;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return {};
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr input,
                                 ArchiveCallback& callback) const override
    {
      const auto rawData = input->GetData();
      const auto size = rawData->Size();
      Statistic::Self().Enqueue(size);
      if (size < MIN_MINIMAL_RAW_SIZE)
      {
        Dbg("Size is too small (%1%)", size);
        return Analysis::CreateUnmatchedResult(size);
      }

      const PluginParameters scanParams(params);
      const std::size_t minRawSize = scanParams.GetMinimalSize();

      const String currentPath = input->GetPath()->AsString();
      Dbg("Detecting modules in raw data at '%1%'", currentPath);
      ScanProgress progress(callback.GetProgress(), size, currentPath);

      RawDetectionPlugins usedPlugins(params, scanParams.GetDoubleAnalysis());

      auto subLocation = MakePtr<ScanDataLocation>(input, 0);

      std::size_t lastUsedEnd = 0;
      while (subLocation->HasToScan(minRawSize))
      {
        const std::size_t offset = subLocation->GetOffset();
        progress.Report(offset);
        usedPlugins.SetOffset(offset);
        const auto detectResult = usedPlugins.Detect(subLocation, callback);
        if (!subLocation.unique())
        {
          Dbg("Sublocation is captured. Duplicate.");
          subLocation = MakePtr<ScanDataLocation>(input, offset);
        }
        subLocation->Move(std::max(detectResult.first, SCAN_STEP));
        if (detectResult.second)
        {
          if (lastUsedEnd != offset)
          {
            callback.ProcessUnknownData(UnknownDataLocation(input, lastUsedEnd, offset));
          }
          lastUsedEnd = subLocation->GetOffset();
        }
      }
      if (lastUsedEnd != size)
      {
        callback.ProcessUnknownData(UnknownDataLocation(std::move(input), lastUsedEnd, size));
      }
      return Analysis::CreateMatchedResult(size);
    }

    DataLocation::Ptr TryOpen(const Parameters::Accessor& /*params*/, DataLocation::Ptr location,
                              const Analysis::Path& inPath) const override
    {
      const String& pathComp = inPath.GetIterator()->Get();
      const Strings::PrefixedIndex pathIndex(PLUGIN_PREFIX, pathComp);
      if (pathIndex.IsValid())
      {
        const auto offset = pathIndex.GetIndex();
        const auto inData = location->GetData();
        auto subData = inData->GetSubcontainer(offset, inData->Size() - offset);
        return CreateNestedLocation(std::move(location), std::move(subData), ID, pathComp);
      }
      return {};
    }
  };
}  // namespace ZXTune::Raw

namespace ZXTune
{
  void RegisterRawContainer(ArchivePluginsRegistrator& registrator)
  {
    auto plugin = MakePtr<Raw::Scaner>();
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
