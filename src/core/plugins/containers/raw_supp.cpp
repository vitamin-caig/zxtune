/*
Abstract:
  RAW containers support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <core/src/callback.h>
#include <core/src/core.h>
#include <core/plugins/registrator.h>
#include <core/plugins/utils.h>
//common includes
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <binary/container.h>
#include <core/error_codes.h>
#include <core/module_detect.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
//std includes
#include <list>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>

#define FILE_TAG 7E0CBD98

namespace
{
  class Statistic
  {
  public:
    Statistic()
      : Start(std::time(0))
      , TotalData(0)
      , ArchivedData(0)
      , ModulesData(0)
    {
    }

    ~Statistic()
    {
      const std::string THIS_MODULE("Core::RawScaner::Statistic");
      const std::time_t spent = std::time(0) - Start;
      Log::Debug(THIS_MODULE, "Total processed: %1%", TotalData);
      const uint64_t useful = ArchivedData + ModulesData;
      Log::Debug(THIS_MODULE, "Useful detected: %1% (%2% archived + %3% modules)", useful, ArchivedData, ModulesData);
      Log::Debug(THIS_MODULE, "Coverage: %1%%%", useful * 100 / TotalData);
      Log::Debug(THIS_MODULE, "Speed is %1% b/s", spent ? (TotalData / spent) : TotalData);
      uint64_t totalUsed = 0, totalMissed = 0;
      for (DetectMap::const_iterator it = Detection.begin(), lim = Detection.end(); it != lim; ++it)
      {
        const std::size_t used = it->second.first;
        const std::size_t missed = it->second.second;
        Log::Debug(THIS_MODULE, "Detector %1%: %2%/%3% missed/used (%4%%% effeciency)", it->first, missed, used, uint64_t(100) * (used - missed) / used);
        totalUsed += used;
        totalMissed += missed;
      }
      Log::Debug(THIS_MODULE, "Total: %1%/%2% missed/used (%3%%% effeciency)", totalMissed, totalUsed, uint64_t(100) * (totalUsed - totalMissed) / totalUsed);
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

    void AddUsed(const String& type)
    {
      ++Detection[type].first;
    }

    void AddMissed(const String& type)
    {
      ++Detection[type].second;
    }

    static Statistic& Self()
    {
      static Statistic self;
      return self;
    }
  private:
    const std::time_t Start;
    uint64_t TotalData;
    uint64_t ArchivedData;
    uint64_t ModulesData;
    typedef std::map<String, std::pair<std::size_t, std::size_t> > DetectMap;
    DetectMap Detection;
  };
}

namespace
{
  using namespace ZXTune;

  const std::string THIS_MODULE("Core::RawScaner");

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
        throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
          Text::RAW_ERROR_INVALID_MIN_SIZE, minRawSize, MIN_MINIMAL_RAW_SIZE);
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
      , Text(path.empty() ? String(Text::PLUGIN_RAW_PROGRESS_NOPATH) : Strings::Format(Text::PLUGIN_RAW_PROGRESS, path))
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
      , OriginalData(static_cast<const uint8_t*>(delegate->Data()))
      , Offset(offset)
    {
    }

    virtual std::size_t Size() const
    {
      return OriginalSize - Offset;
    }

    virtual const void* Data() const
    {
      return OriginalData + Offset;
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
      , Subdata(boost::make_shared<ScanDataContainer>(Parent->GetData(), offset))
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
      String Id;
      std::size_t Offset;

      explicit PluginEntry(typename P::Ptr plugin)
        : Plugin(plugin)
        , Id(Plugin->GetDescription()->Id())
        , Offset()
      {
      }

      PluginEntry()
        : Offset()
      {
      }
    };
    typedef typename std::list<PluginEntry> PluginsList;

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
      return typename P::Iterator::Ptr(new IteratorImpl(Plugins.begin(), Plugins.end(), Offset));
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

    void SetPluginLookahead(const String& id, std::size_t lookahead)
    {
      const typename PluginsList::iterator it = std::find_if(Plugins.begin(), Plugins.end(),
        boost::bind(&PluginEntry::Id, _1) == id);
      if (it != Plugins.end())
      {
        Log::Debug(THIS_MODULE, "Disabling check of %1% for neareast %2% bytes starting from %3%", id, lookahead, Offset);
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

    virtual Analysis::Result::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      const Analysis::Result::Ptr result = Delegate->Detect(inputData, callback);
      if (const std::size_t matched = result->GetMatchedDataSize())
      {
        return Analysis::CreateUnmatchedResult(matched);
      }
      return result;
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& parameters,
                                   DataLocation::Ptr inputData,
                                   const Analysis::Path& pathToOpen) const
    {
      return Delegate->Open(parameters, inputData, pathToOpen);
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
        return 0 != (plug->Capabilities() & CAP_STOR_PLAIN)
          ? boost::make_shared<DoubleAnalyzedArchivePlugin>(res)
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
    RawDetectionPlugins(PlayerPlugin::Iterator::Ptr players, ArchivePlugin::Iterator::Ptr archives, const String& denied)
      : Players(players)
      , Archives(archives)
    {
      Archives.SetPluginLookahead(denied, ~std::size_t(0));
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
      const std::size_t archiveLookahead = detectedArchives->GetLookaheadOffset();
      const std::size_t moduleLookahead = detectedModules->GetLookaheadOffset();
      Log::Debug(THIS_MODULE, "No archives for nearest %1% bytes, modules for %2% bytes",
        archiveLookahead, moduleLookahead);
      return std::min(archiveLookahead, moduleLookahead);
    }

    void SetOffset(std::size_t offset)
    {
      Archives.SetOffset(offset);
      Players.SetOffset(offset);
    }
  private:
    template<class T>
    Analysis::Result::Ptr DetectIn(LookaheadPluginsStorage<T>& container, DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
      for (typename T::Iterator::Ptr iter = container.Enumerate(); iter->IsValid(); iter->Next())
      {
        const typename T::Ptr plugin = iter->Get();
        const Analysis::Result::Ptr result = plugin->Detect(input, callback);
        const String id = plugin->GetDescription()->Id();
        Statistic::Self().AddUsed(id);
        if (std::size_t usedSize = result->GetMatchedDataSize())
        {
          Log::Debug(THIS_MODULE, "Detected %1% in %2% bytes at %3%.", id, usedSize, input->GetPath()->AsString());
          return result;
        }
        const std::size_t lookahead = result->GetLookaheadOffset();
        if (0 == lookahead)
        {
          Statistic::Self().AddMissed(id);
        }
        container.SetPluginLookahead(id, std::max<std::size_t>(lookahead, SCAN_STEP));
      }
      const std::size_t minLookahead = container.GetMinimalPluginLookahead();
      return Analysis::CreateUnmatchedResult(minLookahead);
    }
  private:
    LookaheadPluginsStorage<PlayerPlugin> Players;
    LookaheadPluginsStorage<ArchivePlugin> Archives;
  };
}

namespace
{
  using namespace ZXTune;

  const Char ID[] = {'R', 'A', 'W', 0};
  const Char* const INFO = Text::RAW_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_MULTITRACK | CAP_STOR_SCANER;

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

    virtual Analysis::Result::Ptr Detect(DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
      const Binary::Container::Ptr rawData = input->GetData();
      const std::size_t size = rawData->Size();
      Statistic::Self().Enqueue(size);
      if (size < MIN_MINIMAL_RAW_SIZE)
      {
        Log::Debug(THIS_MODULE, "Size is too small (%1%)", size);
        return Analysis::CreateUnmatchedResult(size);
      }

      const Parameters::Accessor::Ptr pluginParams = callback.GetPluginsParameters();
      const RawPluginParameters scanParams(*pluginParams);
      const std::size_t minRawSize = scanParams.GetMinimalSize();

      const String currentPath = input->GetPath()->AsString();
      Log::Debug(THIS_MODULE, "Detecting modules in raw data at '%1%'", currentPath);
      const Log::ProgressCallback::Ptr progress(new RawProgressCallback(callback, static_cast<uint_t>(size), currentPath));
      const Module::DetectCallback& noProgressCallback = Module::CustomProgressDetectCallbackAdapter(callback);

      const PluginsEnumerator::Ptr availablePlugins = PluginsEnumerator::Create();
      ArchivePlugin::Iterator::Ptr availableArchives = availablePlugins->EnumerateArchives();
      ArchivePlugin::Iterator::Ptr usedArchives = scanParams.GetDoubleAnalysis()
        ? ArchivePlugin::Iterator::Ptr(new DoubleAnalysisArchivePlugins(availableArchives))
        : availableArchives;
      RawDetectionPlugins usedPlugins(availablePlugins->EnumeratePlayers(), usedArchives, Description->Id());

      ScanDataLocation::Ptr subLocation = boost::make_shared<ScanDataLocation>(input, Description->Id(), 0);

      while (subLocation->HasToScan(minRawSize))
      {
        const std::size_t offset = subLocation->GetOffset();
        progress->OnProgress(static_cast<uint_t>(offset));
        usedPlugins.SetOffset(offset);
        const Module::DetectCallback& curCallback = offset ? noProgressCallback : callback;
        const std::size_t bytesToSkip = usedPlugins.Detect(subLocation, curCallback);
        if (!subLocation.unique())
        {
          Log::Debug(THIS_MODULE, "Sublocation is captured. Duplicate.");
          subLocation = boost::make_shared<ScanDataLocation>(input, Description->Id(), offset);
        }
        subLocation->Move(std::max(bytesToSkip, SCAN_STEP));
      }
      return Analysis::CreateMatchedResult(size);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& /*commonParams*/, DataLocation::Ptr location, const Analysis::Path& inPath) const
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
  void RegisterRawContainer(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new RawScaner());
    registrator.RegisterPlugin(plugin);
  }
}
