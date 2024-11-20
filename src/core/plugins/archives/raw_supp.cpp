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

#include "core/plugins/archive_plugin.h"
#include "core/plugins/archive_plugins_registrator.h"
#include "core/plugins/archives/archived.h"
#include "core/plugins/archives/l10n.h"
#include "core/plugins/player_plugin.h"
#include "core/src/location.h"

#include "analysis/path.h"
#include "analysis/result.h"
#include "binary/container.h"
#include "binary/format.h"
#include "core/data_location.h"
#include "core/plugin_attrs.h"
#include "core/plugins_parameters.h"
#include "debug/log.h"
#include "l10n/api.h"
#include "math/scale.h"
#include "parameters/accessor.h"
#include "parameters/identifier.h"
#include "parameters/types.h"
#include "strings/conversion.h"
#include "strings/prefixed_index.h"
#include "time/serialize.h"
#include "time/timer.h"
#include "tools/progress_callback.h"

#include "error_tools.h"
#include "make_ptr.h"
#include "types.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <map>
#include <memory>
#include <span>
#include <utility>
#include <vector>

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

    bool GetDoubleAnalysis() const
    {
      return 0 != Parameters::GetInteger(Accessor, Parameters::ZXTune::Core::Plugins::Raw::PLAIN_DOUBLE_ANALYSIS);
    }

  private:
    const Parameters::Accessor& Accessor;
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

  template<class P>
  class LookaheadPluginsStorage
  {
    struct PluginEntry
    {
      typename P::Ptr Plugin;
      std::size_t Offset = 0;

      explicit PluginEntry(typename P::Ptr plugin)
        : Plugin(std::move(plugin))
      {}

      PluginEntry() = default;
    };
    using PluginsList = typename std::vector<PluginEntry>;

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
    {
      for (const auto& plugin : plugins)
      {
        if (plugin->Capabilities() != CAPS)
        {
          Plugins.emplace_back(PluginEntry(plugin));
        }
        else
        {
          Dbg("Ignore '{}'", plugin->Description());
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
    std::size_t Offset = 0;
    PluginsList Plugins;
  };

  class DoubleAnalyzedArchivePlugin : public ArchivePlugin
  {
  public:
    explicit DoubleAnalyzedArchivePlugin(ArchivePlugin::Ptr delegate)
      : Delegate(std::move(delegate))
    {}

    PluginId Id() const override
    {
      return Delegate->Id();
    }

    StringView Description() const override
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
        auto result = plugin.Detect(Params, input, callback);
        const auto id = plugin.Id();
        if (const auto usedSize = result->GetMatchedDataSize())
        {
          Statistic::Self().AddAimed(plugin, detectTimer);
          Dbg("Detected {} in {} bytes at {}", id, usedSize, input->GetPath()->AsString());
          return result;
        }
        else if (!initialScan)
        {
          Statistic::Self().AddMissed(plugin, detectTimer);
          const Time::Timer scanTimer;
          const std::size_t lookahead = result->GetLookaheadOffset();
          iter.SetLookahead(lookahead);
          Dbg("Disabling check of {} for neareast {} bytes starting from {}", id, lookahead, Offset);
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
