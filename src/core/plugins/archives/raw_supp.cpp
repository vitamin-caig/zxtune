/**
 *
 * @file
 *
 * @brief  Scanner plugin implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/archives/raw_supp.h"

#include "core/plugins/archive_plugins_registrator.h"
#include "core/plugins/archives/archived.h"
#include "core/plugins/archives/l10n.h"
#include "core/plugins/players/plugin.h"

#include "binary/container.h"
#include "core/plugin.h"
#include "core/plugin_attrs.h"
#include "core/plugins_parameters.h"
#include "debug/log.h"
#include "math/scale.h"
#include "strings/conversion.h"
#include "strings/prefixed_index.h"
#include "time/duration.h"
#include "time/serialize.h"
#include "time/timer.h"
#include "tools/progress_callback.h"

#include "error_tools.h"
#include "make_ptr.h"
#include "string_view.h"

#include <algorithm>
#include <array>
#include <list>
#include <map>

namespace ZXTune
{
  template<std::size_t Fields>
  class StatisticBuilder
  {
  public:
    using Line = std::array<String, Fields>;

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

    static String Widen(StringView str, std::size_t wid)
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
    Statistic() = default;

    ~Statistic()
    {
      const Debug::Stream Dbg("Core::RawScaner::Statistic");
      const auto spent = Timer.Elapsed();
      Dbg("Total processed: {}", TotalData);
      Dbg("Time spent: {}", Time::ToString(spent));
      const uint64_t useful = ArchivedData + ModulesData;
      Dbg("Useful detected: {} ({} archived + {} modules)", useful, ArchivedData, ModulesData);
      Dbg("Coverage: {}%", useful * 100 / TotalData);
      Dbg("Speed: {} b/s", spent.Get() ? (TotalData * spent.PER_SECOND / spent.Get()) : TotalData);
      StatisticBuilder<7> builder;
      builder.Add(MakeStatLine(), 0);
      StatItem total;
      total.Name = "Total";
      for (const auto& it : Detection)
      {
        builder.Add(MakeStatLine(it.second), 1 + it.second.Index);
        total += it.second;
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

    void AddAimed(const Plugin& plugin, const Time::Timer& scanTimer)
    {
      StatItem& item = GetStat(plugin);
      ++item.Aimed;
      item.AimedTime += scanTimer.Elapsed() + item.ScanTime;
      item.ScanTime = {};
    }

    void AddMissed(const Plugin& plugin, const Time::Timer& scanTimer)
    {
      StatItem& item = GetStat(plugin);
      ++item.Missed;
      item.MissedTime += scanTimer.Elapsed() + item.ScanTime;
      item.ScanTime = {};
    }

    void AddScanned(const Plugin& plugin, const Time::Timer& scanTimer)
    {
      StatItem& item = GetStat(plugin);
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
      std::size_t Index = 0;
      std::size_t Aimed = 0;
      std::size_t Missed = 0;
      Time::Duration<TimeUnit> AimedTime;
      Time::Duration<TimeUnit> MissedTime;
      Time::Duration<TimeUnit> ScanTime;

      StatItem() = default;

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
      res[1] = Strings::ConvertFrom(item.Missed);
      res[2] = Strings::ConvertFrom(item.Aimed + item.Missed);
      res[3] = Strings::ConvertFrom(Percent(item.Aimed, item.Missed));
      res[4] = Strings::ConvertFrom(item.MissedTime.CastTo<Time::Millisecond>().Get());
      res[5] = Strings::ConvertFrom((item.MissedTime + item.AimedTime).CastTo<Time::Millisecond>().Get());
      res[6] = Strings::ConvertFrom(Percent(item.AimedTime.Get(), item.MissedTime.Get()));
      return res;
    }

    //! @brief 64 bit arguments required, native time unit is nanoseconds and may overflow 32 bit
    static uint_t Percent(uint64_t aimed, uint64_t missed)
    {
      if (const uint64_t total = aimed + missed)
      {
        return static_cast<uint_t>(100 * aimed / total);
      }
      else
      {
        return 0;
      }
    }

    //! @brief the report lists the plugins in the order they were met in
    StatItem& GetStat(const Plugin& plugin)
    {
      StatItem& res = Detection[&plugin];
      if (res.Name.empty())
      {
        res.Name = plugin.Description();
        res.Index = Detection.size() - 1;
      }
      return res;
    }

  private:
    const Time::Timer Timer;
    uint64_t TotalData = 0;
    uint64_t ArchivedData = 0;
    uint64_t ModulesData = 0;
    using DetectMap = std::map<const void*, StatItem>;
    DetectMap Detection;
  };
}  // namespace ZXTune

namespace ZXTune::Raw
{
  const Debug::Stream Dbg("Core::RawScaner");

  const auto PLUGIN_PREFIX = "+"sv;

  const auto ID = "RAW"_id;
  const auto INFO = "Raw scaner"sv;
  const uint_t CAPS = Capabilities::Category::CONTAINER | Capabilities::Container::Type::SCANER;

  const std::size_t SCAN_STEP = 1;
  const std::size_t MIN_MINIMAL_RAW_SIZE = 128;

  String CreateFilename(std::size_t offset)
  {
    assert(offset);
    return Strings::PrefixedIndex::Create(PLUGIN_PREFIX, offset).ToString();
  }

  //! @brief offset tracking and lookahead bookkeeping belong to the scaner, which treats the two
  //!        plugin kinds uniformly, so only the delegate tells them apart
  class Detector
  {
  public:
    using Ptr = std::unique_ptr<const Detector>;

    explicit Detector(Plugin::Ptr delegate)
      : Delegate(std::move(delegate))
    {}

    virtual ~Detector() = default;

    template<class T = Plugin>
    const T& GetPlugin() const
    {
      return static_cast<const T&>(*Delegate);
    }

    //! @brief the delegates differ in nothing but what they detect, so all of them are measured
    //!        and accounted here
    Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                                 ArchiveCallback& callback) const
    {
      const Time::Timer detectTimer;
      auto result = DetectImpl(params, std::move(inputData), callback);
      if (const auto matched = result->GetMatchedDataSize())
      {
        Statistic::Self().AddAimed(GetPlugin(), detectTimer);
        // the plugin found something the scaner must not take, so the scan is not a miss either
        if (HideMatch(params))
        {
          return Analysis::CreateUnmatchedResult(matched);
        }
        return result;
      }
      Statistic::Self().AddMissed(GetPlugin(), detectTimer);
      return result;
    }

  protected:
    //! @brief ArchiveCallback is a Module::DetectCallback, so a single signature serves both
    //!        delegate types
    virtual Analysis::Result::Ptr DetectImpl(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                                             ArchiveCallback& callback) const = 0;

    virtual bool HideMatch(const Parameters::Accessor& /*params*/) const
    {
      return false;
    }

  private:
    const Plugin::Ptr Delegate;
  };

  class PlayerDetector : public Detector
  {
  public:
    explicit PlayerDetector(PlayerPlugin::Ptr plugin)
      : Detector(std::move(plugin))
    {}

  private:
    Analysis::Result::Ptr DetectImpl(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                                     ArchiveCallback& callback) const override
    {
      return GetPlugin<PlayerPlugin>().Detect(params, std::move(inputData), callback);
    }
  };

  class ArchiveDetector : public Detector
  {
  public:
    explicit ArchiveDetector(ArchivePlugin::Ptr plugin)
      : Detector(std::move(plugin))
      // the delegate is moved into the base, which is constructed first, so the trait is read
      // through it and not through the moved-from argument
      , Plain(0 != (GetPlugin<ArchivePlugin>().Capabilities() & Capabilities::Container::Traits::PLAIN))
    {}

  private:
    Analysis::Result::Ptr DetectImpl(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                                     ArchiveCallback& callback) const override
    {
      return GetPlugin<ArchivePlugin>().Detect(params, std::move(inputData), callback);
    }

    //! @brief a plain archive reports a match for the data it merely skips
    bool HideMatch(const Parameters::Accessor& params) const override
    {
      return Plain && DoubleAnalysis(params);
    }

    static bool DoubleAnalysis(const Parameters::Accessor& params)
    {
      return 0 != Parameters::GetInteger(params, Parameters::ZXTune::Core::Plugins::Raw::PLAIN_DOUBLE_ANALYSIS);
    }

    //! @brief a container trait never changes, unlike the parameter checked against it
    const bool Plain;
  };

  class PluginParameters
  {
  public:
    explicit PluginParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {}

    std::size_t GetMinimalSize() const
    {
      using namespace Parameters::ZXTune::Core::Plugins::Raw;
      const auto minRawSize = Parameters::GetInteger<std::size_t>(Accessor, MIN_SIZE, MIN_SIZE_DEFAULT);
      if (minRawSize < MIN_MINIMAL_RAW_SIZE)
      {
        throw MakeFormattedError(THIS_LINE, translate("Specified minimal scan size ({0}). Should be more than {1}."),
                                 minRawSize, MIN_MINIMAL_RAW_SIZE);
      }
      return minRawSize;
    }

  private:
    const Parameters::Accessor& Accessor;
  };

  //! @brief built on demand because the scaner is constructed while the global plugin list is still
  //!        being filled and cannot enumerate it yet
  class Detectors
  {
  public:
    using List = std::vector<Detector::Ptr>;

    const List& GetPlayers() const
    {
      if (PlayerDetectors.empty())
      {
        const auto& original = PlayerPlugin::Enumerate();
        PlayerDetectors.reserve(original.size());
        // a player plugin is never a scaner, so no filtering here
        for (const auto& plugin : original)
        {
          PlayerDetectors.emplace_back(MakePtr<PlayerDetector>(plugin));
        }
      }
      return PlayerDetectors;
    }

    const List& GetArchives() const
    {
      if (ArchiveDetectors.empty())
      {
        const auto& original = ArchivePlugin::Enumerate();
        ArchiveDetectors.reserve(original.size());
        for (const auto& plugin : original)
        {
          // the scaner is registered as an archive plugin as well and would detect itself
          if (plugin->Capabilities() == CAPS)
          {
            Dbg("Ignore '{}'", plugin->Description());
          }
          else
          {
            ArchiveDetectors.emplace_back(MakePtr<ArchiveDetector>(plugin));
          }
        }
      }
      return ArchiveDetectors;
    }

  private:
    mutable List PlayerDetectors;
    mutable List ArchiveDetectors;
  };

  class ScanProgress
  {
  public:
    ScanProgress(Log::ProgressCallback* delegate, std::size_t limit, StringView path)
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
    const Math::ScaleFunctor<uint64_t> ToPercent;
    const String Text;
  };

  class ScanDataContainer : public Binary::Container
  {
  public:
    using Ptr = std::shared_ptr<ScanDataContainer>;

    ScanDataContainer(Binary::Container::Ptr delegate, std::size_t offset)
      : Delegate(std::move(delegate))
      , OriginalSize(Delegate->Size())
      , OriginalData(static_cast<const uint8_t*>(Delegate->Start()))
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

    std::size_t GetOffset() const
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
    using Ptr = std::shared_ptr<ScanDataLocation>;

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
      auto parentPath = Parent->GetPath();
      if (const auto offset = Subdata->GetOffset())
      {
        const auto subPath = CreateFilename(offset);
        return parentPath->Append(subPath);
      }
      return parentPath;
    }

    Analysis::Path::Ptr GetPluginsChain() const override
    {
      auto parentPlugins = Parent->GetPluginsChain();
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
      Dbg("Unknown data at {}..{}", Start, End);
    }

    Binary::Container::Ptr GetData() const override
    {
      return Parent->GetData()->GetSubcontainer(Start, End - Start);
    }

    Analysis::Path::Ptr GetPath() const override
    {
      auto parentPath = Parent->GetPath();
      if (Start)
      {
        const auto subPath = CreateFilename(Start);
        return parentPath->Append(subPath);
      }
      return parentPath;
    }

    Analysis::Path::Ptr GetPluginsChain() const override
    {
      auto parentPlugins = Parent->GetPluginsChain();
      if (Start)
      {
        return parentPlugins->Append(ID);
      }
      return parentPlugins;
    }

  private:
    const DataLocation::Ptr Parent;
    const std::size_t Start;
    const std::size_t End;
  };

  //! @brief the detectors outlive the scans, so they are only observed here
  class LookaheadPlugins
  {
    struct PluginEntry
    {
      const Detector* Plugin;
      std::size_t Offset = 0;

      explicit PluginEntry(const Detector* plugin)
        : Plugin(plugin)
      {}
    };
    using PluginsList = std::vector<PluginEntry>;

  public:
    class Iterator
    {
    public:
      Iterator(PluginsList::iterator it, PluginsList::iterator lim, std::size_t offset)
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

      const Detector& GetDetector() const
      {
        assert(Cur != Lim);
        return *Cur->Plugin;
      }

      const Plugin& GetPlugin() const
      {
        return GetDetector().GetPlugin();
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
      PluginsList::iterator Cur;
      const PluginsList::iterator Lim;
      const std::size_t Offset;
    };

    explicit LookaheadPlugins(const Detectors::List& plugins)
    {
      Plugins.reserve(plugins.size());
      for (const auto& plugin : plugins)
      {
        Plugins.emplace_back(PluginEntry(plugin.get()));
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
      for (auto& plugin : Plugins)
      {
        plugin.Offset = std::max(plugin.Offset, offset);
      }
    }

  private:
    std::size_t Offset = 0;
    PluginsList Plugins;
  };

  class RawDetectionPlugins
  {
  public:
    RawDetectionPlugins(const Parameters::Accessor& params, const Detectors& detectors)
      : Params(params)
      , Players(detectors.GetPlayers())
      , Archives(detectors.GetArchives())
    {}

    std::pair<std::size_t, bool> Detect(DataLocation::Ptr input, ArchiveCallback& callback)
    {
      const auto detectedModules = DetectIn(Players, input, callback);
      if (const auto matched = detectedModules->GetMatchedDataSize())
      {
        Statistic::Self().AddModule(matched);
        return {matched, true};
      }
      const auto detectedArchives = DetectIn(Archives, std::move(input), callback);
      if (const auto matched = detectedArchives->GetMatchedDataSize())
      {
        Statistic::Self().AddArchived(matched);
        return {matched, true};
      }
      const auto archiveLookahead = detectedArchives->GetLookaheadOffset();
      const auto moduleLookahead = detectedModules->GetLookaheadOffset();
      Dbg("No archives for nearest {} bytes, modules for {} bytes", archiveLookahead, moduleLookahead);
      return {static_cast<std::size_t>(std::min(archiveLookahead, moduleLookahead)), false};
    }

    void SetOffset(std::size_t offset)
    {
      Offset = offset;
      Archives.SetOffset(offset);
      Players.SetOffset(offset);
    }

  private:
    Analysis::Result::Ptr DetectIn(LookaheadPlugins& container, DataLocation::Ptr input,
                                   ArchiveCallback& callback) const
    {
      const std::size_t maxSize = input->GetData()->Size();
      auto unmatched = std::make_unique<DelayScanResult>(container, Offset, maxSize);
      for (auto iter = container.Enumerate(); iter.IsValid(); iter.Next())
      {
        const auto& detector = iter.GetDetector();
        auto result = detector.Detect(Params, input, callback);
        if (const auto usedSize = result->GetMatchedDataSize())
        {
          Dbg("Detected {} in {} bytes at {}", iter.GetPlugin().Id(), usedSize, input->GetPath()->AsString());
          return result;
        }
        else
        {
          unmatched->AddPendingScan(std::move(result), iter);
        }
      }
      return unmatched;
    }

    class DelayScanResult : public Analysis::Result
    {
    public:
      DelayScanResult(LookaheadPlugins& plugins, std::size_t offset, std::size_t maxSize)
        : Plugins(plugins)
        , Offset(offset)
        , MaxSize(maxSize)
      {}

      void AddPendingScan(Analysis::Result::Ptr result, LookaheadPlugins::Iterator iter)
      {
        PendingScans.emplace_back(std::move(result), std::move(iter));
      }

      std::size_t GetMatchedDataSize() const override
      {
        return 0;
      }

      std::size_t GetLookaheadOffset() const override
      {
        ApplyPendingScans();
        return Plugins.GetMinimalPluginLookahead();
      }

    private:
      void ApplyPendingScans() const
      {
        for (auto& [result, iter] : PendingScans)
        {
          const Time::Timer scanTimer;
          const auto lookahead = result->GetLookaheadOffset();
          iter.SetLookahead(lookahead);
          Dbg("Disabling check of {} for neareast {} bytes starting from {}", iter.GetPlugin().Id(), lookahead, Offset);
          if (lookahead == MaxSize)
          {
            Statistic::Self().AddAimed(iter.GetPlugin(), scanTimer);
          }
          else
          {
            Statistic::Self().AddScanned(iter.GetPlugin(), scanTimer);
          }
        }
        PendingScans.clear();
      }

    private:
      LookaheadPlugins& Plugins;
      const std::size_t Offset;
      const std::size_t MaxSize;
      mutable std::vector<std::pair<Analysis::Result::Ptr, LookaheadPlugins::Iterator>> PendingScans;
    };

  private:
    const Parameters::Accessor& Params;
    LookaheadPlugins Players;
    LookaheadPlugins Archives;
    std::size_t Offset = 0;
  };

  class Scaner : public ArchivePlugin
  {
  public:
    Scaner() = default;

    PluginId Id() const override
    {
      return ID;
    }

    StringView Description() const override
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
        Dbg("Size is too small ({})", size);
        return Analysis::CreateUnmatchedResult(size);
      }

      const PluginParameters scanParams(params);
      const std::size_t minRawSize = scanParams.GetMinimalSize();

      const String currentPath = input->GetPath()->AsString();
      Dbg("Detecting modules in raw data at '{}'", currentPath);
      ScanProgress progress(callback.GetProgress(), size, currentPath);

      RawDetectionPlugins usedPlugins(params, Plugins);

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
      const auto& pathComp = inPath.Elements().front();
      const auto pathIndex = Strings::PrefixedIndex::Parse(PLUGIN_PREFIX, pathComp);
      if (pathIndex.IsValid())
      {
        const auto offset = pathIndex.GetIndex();
        const auto inData = location->GetData();
        auto subData = inData->GetSubcontainer(offset, inData->Size() - offset);
        return CreateNestedLocation(std::move(location), std::move(subData), ID, pathComp);
      }
      return {};
    }

  private:
    Detectors Plugins;
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
