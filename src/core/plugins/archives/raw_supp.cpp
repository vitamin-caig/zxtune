/**
* 
* @file
*
* @brief  Scanner plugin implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "archived.h"
#include <core/src/callback.h>
#include <core/plugins/archive_plugins_enumerator.h>
#include <core/plugins/archive_plugins_registrator.h>
#include <core/plugins/player_plugins_enumerator.h>
#include <core/plugins/utils.h>
#include <core/plugins/plugins_types.h>
//common includes
#include <error_tools.h>
#include <make_ptr.h>
//library includes
#include <binary/container.h>
#include <core/module_detect.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <debug/log.h>
#include <l10n/api.h>
#include <time/duration.h>
#include <time/timer.h>
//std includes
#include <list>
#include <map>
//boost includes
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
//text includes
#include <core/text/plugins.h>

#define FILE_TAG 7E0CBD98

namespace ZXTune
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("core");

  template<std::size_t Fields>
  class StatisticBuilder
  {
  public:
    typedef boost::array<std::string, Fields> Line;

    StatisticBuilder()
      : Lines()
      , Widths()
    {
    }

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

    std::string Get() const
    {
      std::string res;
      for (typename std::vector<Line>::const_iterator it = Lines.begin(), lim = Lines.end(); it != lim; ++it)
      {
        res += ToString(*it);
      }
      return res;
    }
  private:
    std::string ToString(const Line& line) const
    {
      std::string res;
      for (uint_t idx = 0; idx != Fields; ++idx)
      {
        res += Widen(line[idx], Widths[idx] + 1);
      }
      *res.rbegin() = '\n';
      return res;
    }

    static std::string Widen(const std::string& str, std::size_t wid)
    {
      std::string res(wid, ' ');
      std::copy(str.begin(), str.end(), res.begin());
      return res;
    }
  private:
    std::vector<Line> Lines;
    boost::array<std::size_t, Fields> Widths;
  };

  class Statistic
  {
    typedef Time::Timer::NativeStamp Stamp;
  public:
    Statistic()
      : TotalData(0)
      , ArchivedData(0)
      , ModulesData(0)
    {
    }

    ~Statistic()
    {
      const Debug::Stream Dbg("Core::RawScaner::Statistic");
      const Stamp spent = Timer.Elapsed();
      Dbg("Total processed: %1%", TotalData);
      Dbg("Time spent: %1%", Time::Duration<Stamp::ValueType, Stamp>(1, spent).ToString());
      const uint64_t useful = ArchivedData + ModulesData;
      Dbg("Useful detected: %1% (%2% archived + %3% modules)", useful, ArchivedData, ModulesData);
      Dbg("Coverage: %1%%%", useful * 100 / TotalData);
      Dbg("Speed: %1% b/s", spent.Get() ? (TotalData * Stamp::PER_SECOND / spent.Get()) : TotalData);
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
      item.ScanTime = Stamp(0);
    }

    template<class PluginType>
    void AddMissed(const PluginType& plug, const Time::Timer& scanTimer)
    {
      StatItem& item = GetStat(plug);
      ++item.Missed;
      item.MissedTime += scanTimer.Elapsed() + item.ScanTime;
      item.ScanTime = Stamp(0);
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
      std::string Name;
      std::size_t Index;
      std::size_t Aimed;
      std::size_t Missed;
      Stamp AimedTime;
      Stamp MissedTime;
      Stamp ScanTime;

      StatItem()
        : Index()
        , Aimed()
        , Missed()
        , AimedTime()
        , MissedTime()
        , ScanTime()
      {
      }

      StatItem& operator += (const StatItem& rh)
      {
        Aimed += rh.Aimed;
        Missed += rh.Missed;
        AimedTime += rh.AimedTime;
        MissedTime += rh.MissedTime;
        return *this;
      }
    };

    static boost::array<std::string, 7> MakeStatLine()
    {
      boost::array<std::string, 7> res;
      res[0] = "\nDetector";
      res[1] = "Missed detect";
      res[2] = "Total detect";
      res[3] = "DEff,%";
      res[4] = "Missed time,ms";
      res[5] = "Total time,ms";
      res[6] = "TEff,%";
      return res;
    }

    static boost::array<std::string, 7> MakeStatLine(const StatItem& item)
    {
      boost::array<std::string, 7> res;
      res[0] = item.Name;
      res[1] = boost::lexical_cast<std::string>(item.Missed);
      res[2] = boost::lexical_cast<std::string>(item.Aimed + item.Missed);
      res[3] = boost::lexical_cast<std::string>(Percent(item.Aimed, item.Missed));
      res[4] = boost::lexical_cast<std::string>(Time::Milliseconds(item.MissedTime).Get());
      res[5] = boost::lexical_cast<std::string>(Time::Milliseconds(item.MissedTime + item.AimedTime).Get());
      res[6] = boost::lexical_cast<std::string>(Percent(item.AimedTime.Get(), item.MissedTime.Get()));
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
        res.Name = ToStdString(plug.GetDescription()->Description());
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
}

namespace ZXTune
{
  const Debug::Stream Dbg("Core::RawScaner");

  const Char ID[] = {'R', 'A', 'W', 0};
  const Char* const INFO = Text::RAW_PLUGIN_INFO;
  const uint_t CAPS = Capabilities::Category::CONTAINER | Capabilities::Container::Type::SCANER;

  const std::size_t SCAN_STEP = 1;
  const std::size_t MIN_MINIMAL_RAW_SIZE = 128;

  const IndexPathComponent RawPath(Text::RAW_PLUGIN_PREFIX);

  class RawPluginParameters
  {
  public:
    explicit RawPluginParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    std::size_t GetMinimalSize() const
    {
      Parameters::IntType minRawSize = Parameters::ZXTune::Core::Plugins::Raw::MIN_SIZE_DEFAULT;
      if (Accessor.FindValue(Parameters::ZXTune::Core::Plugins::Raw::MIN_SIZE, minRawSize) &&
          minRawSize < Parameters::IntType(MIN_MINIMAL_RAW_SIZE))
      {
        throw MakeFormattedError(THIS_LINE,
          translate("Specified minimal scan size (%1%). Should be more than %2%."), minRawSize, MIN_MINIMAL_RAW_SIZE);
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

  class RawProgressCallback : public Log::ProgressCallback
  {
  public:
    RawProgressCallback(const Module::DetectCallback& callback, uint_t limit, const String& path)
      : Delegate(CreateProgressCallback(callback, limit))
      , Text(ProgressMessage(ID, path))
    {
    }

    virtual void OnProgress(uint_t current)
    {
      OnProgress(current, Text);
    }

    virtual void OnProgress(uint_t current, const String& message)
    {
      if (Delegate.get())
      {
        Delegate->OnProgress(current, message);
      }
    }
  private:
    const Log::ProgressCallback::Ptr Delegate;
    const String Text;
  };

  class ScanDataContainer : public Binary::Container
  {
  public:
    typedef boost::shared_ptr<ScanDataContainer> Ptr;

    ScanDataContainer(Binary::Container::Ptr delegate, std::size_t offset)
      : Delegate(delegate)
      , OriginalSize(delegate->Size())
      , OriginalData(static_cast<const uint8_t*>(delegate->Start()))
      , Offset(offset)
    {
    }

    virtual const void* Start() const
    {
      return OriginalData + Offset;
    }

    virtual std::size_t Size() const
    {
      return OriginalSize - Offset;
    }

    virtual Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
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
    typedef boost::shared_ptr<ScanDataLocation> Ptr;

    ScanDataLocation(DataLocation::Ptr parent, const String& subPlugin, std::size_t offset)
      : Parent(parent)
      , Subdata(MakePtr<ScanDataContainer>(Parent->GetData(), offset))
      , Subplugin(subPlugin)
    {
    }

    virtual Binary::Container::Ptr GetData() const
    {
      return Subdata;
    }

    virtual Analysis::Path::Ptr GetPath() const
    {
      const Analysis::Path::Ptr parentPath = Parent->GetPath();
      if (std::size_t offset = Subdata->GetOffset())
      {
        const String subPath = RawPath.Build(offset);
        return parentPath->Append(subPath);
      }
      return parentPath;
    }

    virtual Analysis::Path::Ptr GetPluginsChain() const
    {
      const Analysis::Path::Ptr parentPlugins = Parent->GetPluginsChain();
      if (Subdata->GetOffset())
      {
        return parentPlugins->Append(Subplugin);
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
      return Subdata->Move(step);
    }
  private:
    const DataLocation::Ptr Parent;
    const ScanDataContainer::Ptr Subdata;
    const String Subplugin;
  };

  template<class P>
  class LookaheadPluginsStorage
  {
    struct PluginEntry
    {
      typename P::Ptr Plugin;
      std::size_t Offset;

      explicit PluginEntry(typename P::Ptr plugin)
        : Plugin(plugin)
        , Offset()
      {
      }

      PluginEntry()
        : Offset()
      {
      }
    };
    typedef typename std::vector<PluginEntry> PluginsList;

    class IteratorImpl : public P::Iterator
    {
    public:
      IteratorImpl(typename PluginsList::const_iterator it, typename PluginsList::const_iterator lim, std::size_t offset)
        : Cur(it)
        , Lim(lim)
        , Offset(offset)
      {
        SkipUnaffected();
      }

      virtual bool IsValid() const
      {
        return Cur != Lim;
      }

      virtual typename P::Ptr Get() const
      {
        assert(Cur != Lim);
        return Cur->Plugin;
      }

      virtual void Next()
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
      typename PluginsList::const_iterator Cur;
      const typename PluginsList::const_iterator Lim;
      const std::size_t Offset;
    };
  public:
    explicit LookaheadPluginsStorage(typename P::Iterator::Ptr iterator)
      : Offset()
    {
      for (; iterator->IsValid(); iterator->Next())
      {
        const typename P::Ptr plugin = iterator->Get();
        Plugins.push_back(PluginEntry(plugin));
      }
    }

    typename P::Iterator::Ptr Enumerate() const
    {
      return MakePtr<IteratorImpl>(Plugins.begin(), Plugins.end(), Offset);
    }

    std::size_t GetMinimalPluginLookahead() const
    {
      const typename PluginsList::const_iterator it = std::min_element(Plugins.begin(), Plugins.end(), 
        boost::bind(&PluginEntry::Offset, _1) < boost::bind(&PluginEntry::Offset, _2));
      return it->Offset >= Offset ? it->Offset - Offset : 0;
    }
    
    void SetOffset(std::size_t offset)
    {
      Offset = offset;
    }

    void SetPluginLookahead(const P& plug, const String& id, std::size_t lookahead)
    {
      const typename PluginsList::iterator it = std::find_if(Plugins.begin(), Plugins.end(),
        boost::bind(&P::Ptr::get, boost::bind(&PluginEntry::Plugin, _1)) == &plug);
      if (it != Plugins.end())
      {
        Dbg("Disabling check of %1% for neareast %2% bytes starting from %3%", id, lookahead, Offset);
        it->Offset += lookahead;
      }
    }
  private:
    std::size_t Offset;
    PluginsList Plugins;
  };

  class DoubleAnalyzedArchivePlugin : public ArchivePlugin
  {
  public:
    explicit DoubleAnalyzedArchivePlugin(ArchivePlugin::Ptr delegate)
      : Delegate(delegate)
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Delegate->GetDescription();
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Delegate->GetFormat();
    }

    virtual Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      const Analysis::Result::Ptr result = Delegate->Detect(params, inputData, callback);
      if (const std::size_t matched = result->GetMatchedDataSize())
      {
        return Analysis::CreateUnmatchedResult(matched);
      }
      return result;
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& params,
                                   DataLocation::Ptr inputData,
                                   const Analysis::Path& pathToOpen) const
    {
      return Delegate->Open(params, inputData, pathToOpen);
    }
  private:
    const ArchivePlugin::Ptr Delegate;
  };

  class DoubleAnalysisArchivePlugins : public ArchivePlugin::Iterator
  {
  public:
    explicit DoubleAnalysisArchivePlugins(ArchivePlugin::Iterator::Ptr delegate)
      : Delegate(delegate)
    {
    }

    virtual bool IsValid() const
    {
      return Delegate->IsValid();
    }

    virtual ArchivePlugin::Ptr Get() const
    {
      if (const ArchivePlugin::Ptr res = Delegate->Get())
      {
        const Plugin::Ptr plug = res->GetDescription();
        return 0 != (plug->Capabilities() & Capabilities::Container::Traits::PLAIN)
          ? MakePtr<DoubleAnalyzedArchivePlugin>(res)
          : res;
      }
      else
      {
        return ArchivePlugin::Ptr();
      }
    }

    virtual void Next()
    {
      return Delegate->Next();
    }
  private:
    const ArchivePlugin::Iterator::Ptr Delegate;
  };

  class RawDetectionPlugins
  {
  public:
    RawDetectionPlugins(const Parameters::Accessor& params, PlayerPlugin::Iterator::Ptr players, ArchivePlugin::Iterator::Ptr archives, const ArchivePlugin& denied)
      : Params(params)
      , Players(players)
      , Archives(archives)
      , Offset()
    {
      Archives.SetPluginLookahead(denied, denied.GetDescription()->Id(), ~std::size_t(0));
    }

    std::size_t Detect(DataLocation::Ptr input, const Module::DetectCallback& callback)
    {
      const Analysis::Result::Ptr detectedModules = DetectIn(Players, input, callback);
      if (const std::size_t matched = detectedModules->GetMatchedDataSize())
      {
        Statistic::Self().AddModule(matched);
        return matched;
      }
      const Analysis::Result::Ptr detectedArchives = DetectIn(Archives, input, callback);
      if (const std::size_t matched = detectedArchives->GetMatchedDataSize())
      {
        Statistic::Self().AddArchived(matched);
        return matched;
      }
      const std::ptrdiff_t archiveLookahead = detectedArchives->GetLookaheadOffset();
      const std::ptrdiff_t moduleLookahead = detectedModules->GetLookaheadOffset();
      Dbg("No archives for nearest %1% bytes, modules for %2% bytes",
        archiveLookahead, moduleLookahead);
      return static_cast<std::size_t>(std::min(archiveLookahead, moduleLookahead));
    }

    void SetOffset(std::size_t offset)
    {
      Offset = offset;
      Archives.SetOffset(offset);
      Players.SetOffset(offset);
    }
  private:
    template<class T>
    Analysis::Result::Ptr DetectIn(LookaheadPluginsStorage<T>& container, DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
      const bool firstScan = 0 == Offset;
      const std::size_t maxSize = input->GetData()->Size();
      for (typename T::Iterator::Ptr iter = container.Enumerate(); iter->IsValid(); iter->Next())
      {
        Time::Timer timer;
        const typename T::Ptr plugin = iter->Get();
        const Analysis::Result::Ptr result = plugin->Detect(Params, input, callback);
        const String id = plugin->GetDescription()->Id();
        if (const std::size_t usedSize = result->GetMatchedDataSize())
        {
          Statistic::Self().AddAimed(*plugin, timer);
          Dbg("Detected %1% in %2% bytes at %3%.", id, usedSize, input->GetPath()->AsString());
          return result;
        }
        else
        {
          if (!firstScan)
          {
            Statistic::Self().AddMissed(*plugin, timer);
            timer = Time::Timer();
          }
          const std::size_t lookahead = result->GetLookaheadOffset();
          container.SetPluginLookahead(*plugin, id, lookahead);
          if (lookahead == maxSize)
          {
            Statistic::Self().AddAimed(*plugin, timer);
          }
          else
          {
            Statistic::Self().AddScanned(*plugin, timer);
          }
        }
      }
      const std::size_t minLookahead = container.GetMinimalPluginLookahead();
      return Analysis::CreateUnmatchedResult(minLookahead);
    }
  private:
    const Parameters::Accessor& Params;
    LookaheadPluginsStorage<PlayerPlugin> Players;
    LookaheadPluginsStorage<ArchivePlugin> Archives;
    std::size_t Offset;
  };
}

namespace ZXTune
{
  class RawScaner : public ArchivePlugin
  {
  public:
    RawScaner()
      : Description(CreatePluginDescription(ID, INFO, CAPS))
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Description;
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Binary::Format::Ptr();
    }

    virtual Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
      const Binary::Container::Ptr rawData = input->GetData();
      const std::size_t size = rawData->Size();
      Statistic::Self().Enqueue(size);
      if (size < MIN_MINIMAL_RAW_SIZE)
      {
        Dbg("Size is too small (%1%)", size);
        return Analysis::CreateUnmatchedResult(size);
      }

      const RawPluginParameters scanParams(params);
      const std::size_t minRawSize = scanParams.GetMinimalSize();

      const String currentPath = input->GetPath()->AsString();
      Dbg("Detecting modules in raw data at '%1%'", currentPath);
      const Log::ProgressCallback::Ptr progress = MakePtr<RawProgressCallback>(callback, static_cast<uint_t>(size), currentPath);
      const Module::DetectCallback& noProgressCallback = Module::CustomProgressDetectCallbackAdapter(callback);

      const ArchivePlugin::Iterator::Ptr availableArchives = ArchivePluginsEnumerator::Create()->Enumerate();
      const ArchivePlugin::Iterator::Ptr usedArchives = scanParams.GetDoubleAnalysis()
        ? MakePtr<DoubleAnalysisArchivePlugins>(availableArchives)
        : availableArchives;
      RawDetectionPlugins usedPlugins(params, PlayerPluginsEnumerator::Create()->Enumerate(), usedArchives, *this);

      ScanDataLocation::Ptr subLocation = MakePtr<ScanDataLocation>(input, Description->Id(), 0);

      while (subLocation->HasToScan(minRawSize))
      {
        const std::size_t offset = subLocation->GetOffset();
        progress->OnProgress(static_cast<uint_t>(offset));
        usedPlugins.SetOffset(offset);
        const Module::DetectCallback& curCallback = offset ? noProgressCallback : callback;
        const std::size_t bytesToSkip = usedPlugins.Detect(subLocation, curCallback);
        if (!subLocation.unique())
        {
          Dbg("Sublocation is captured. Duplicate.");
          subLocation = MakePtr<ScanDataLocation>(input, Description->Id(), offset);
        }
        subLocation->Move(std::max(bytesToSkip, SCAN_STEP));
      }
      return Analysis::CreateMatchedResult(size);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& /*params*/, DataLocation::Ptr location, const Analysis::Path& inPath) const
    {
      const String& pathComp = inPath.GetIterator()->Get();
      std::size_t offset = 0;
      if (RawPath.GetIndex(pathComp, offset))
      {
        const Binary::Container::Ptr inData = location->GetData();
        const Binary::Container::Ptr subData = inData->GetSubcontainer(offset, inData->Size() - offset);
        return CreateNestedLocation(location, subData, Description->Id(), pathComp); 
      }
      return DataLocation::Ptr();
    }
  private:
    const Plugin::Ptr Description;
  };
}

namespace ZXTune
{
  void RegisterRawContainer(ArchivePluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin = MakePtr<RawScaner>();
    registrator.RegisterPlugin(plugin);
  }
}
