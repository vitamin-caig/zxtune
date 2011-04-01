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
#include <core/error_codes.h>
#include <core/module_detect.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <io/container.h>
#include <io/fs_tools.h>
//std includes
#include <list>
//boost includes
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>

#define FILE_TAG 7E0CBD98

namespace
{
  using namespace ZXTune;

  const std::string THIS_MODULE("Core::RawScaner");

  const Char RAW_PLUGIN_ID[] = {'R', 'A', 'W', 0};
  const String RAW_PLUGIN_VERSION(FromStdString("$Rev$"));

  const uint_t MIN_SCAN_STEP = 1;
  const uint_t MAX_SCAN_STEP = 256;
  const std::size_t MIN_MINIMAL_RAW_SIZE = 128;

  const Char RAW_PREFIX[] = {'+', 0};

  //\+\d+
  inline bool CheckIfRawPart(const String& str, std::size_t& offset)
  {
    static const String PREFIX(RAW_PREFIX);

    Parameters::IntType res = 0;
    if (!str.empty() && 
        0 == str.find(PREFIX) &&
        Parameters::ConvertFromString(str.substr(PREFIX.size()), res))
    {
      offset = res;
      return true;
    }
    return false;
  }

  inline String CreateRawPart(std::size_t offset)
  {
    String res(RAW_PREFIX);
    res += Parameters::ConvertToString(offset);
    return res;
  }

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
      if (Accessor.FindIntValue(Parameters::ZXTune::Core::Plugins::Raw::MIN_SIZE, minRawSize) &&
          minRawSize < Parameters::IntType(MIN_MINIMAL_RAW_SIZE))
      {
        throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
          Text::RAW_ERROR_INVALID_MIN_SIZE, minRawSize, MIN_MINIMAL_RAW_SIZE);
      }
      return static_cast<std::size_t>(minRawSize);
    }

    std::size_t GetScanStep() const
    {
      Parameters::IntType scanStep = Parameters::ZXTune::Core::Plugins::Raw::SCAN_STEP_DEFAULT;
      if (Accessor.FindIntValue(Parameters::ZXTune::Core::Plugins::Raw::SCAN_STEP, scanStep) &&
          (scanStep < Parameters::IntType(MIN_SCAN_STEP) ||
           scanStep > Parameters::IntType(MAX_SCAN_STEP)))
      {
        throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
          Text::RAW_ERROR_INVALID_STEP, scanStep, MIN_SCAN_STEP, MAX_SCAN_STEP);
      }
      return static_cast<std::size_t>(scanStep);
    }
  private:
    const Parameters::Accessor& Accessor;
  };

  class RawProgressCallback : public Log::ProgressCallback
  {
  public:
    RawProgressCallback(const Module::DetectCallback& callback, uint_t limit, const String& path)
      : Delegate(CreateProgressCallback(callback, limit))
      , Text(path.empty() ? String(Text::PLUGIN_RAW_PROGRESS_NOPATH) : (Formatter(Text::PLUGIN_RAW_PROGRESS) % path).str())
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

  class ScanDataContainer : public IO::DataContainer
  {
  public:
    typedef boost::shared_ptr<ScanDataContainer> Ptr;

    ScanDataContainer(IO::DataContainer::Ptr delegate, std::size_t offset)
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

    virtual IO::DataContainer::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
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
    const IO::DataContainer::Ptr Delegate;
    const std::size_t OriginalSize;
    const uint8_t* const OriginalData;
    std::size_t Offset;
  };

  class ScanDataLocation : public DataLocation
  {
  public:
    typedef boost::shared_ptr<ScanDataLocation> Ptr;

    ScanDataLocation(DataLocation::Ptr parent, Plugin::Ptr subPlugin, std::size_t offset)
      : Parent(parent)
      , Subdata(boost::make_shared<ScanDataContainer>(Parent->GetData(), offset))
      , Subplugin(subPlugin)
    {
    }

    virtual IO::DataContainer::Ptr GetData() const
    {
      return Subdata;
    }

    virtual String GetPath() const
    {
      const String parentPath = Parent->GetPath();
      if (std::size_t offset = Subdata->GetOffset())
      {
        const String subPath = CreateRawPart(offset);
        return IO::AppendPath(parentPath, subPath);
      }
      return parentPath;
    }

    virtual PluginsChain::Ptr GetPlugins() const
    {
      const PluginsChain::Ptr parentPlugins = Parent->GetPlugins();
      if (Subdata->GetOffset())
      {
        return PluginsChain::CreateMerged(parentPlugins, Subplugin);
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
    const Plugin::Ptr Subplugin;
  };

  template<class P>
  class LookaheadPluginsStorage
  {
    typedef typename std::pair<std::size_t, typename P::Ptr> OffsetAndPlugin;
    typedef typename std::list<OffsetAndPlugin> PluginsList;

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
        return Cur->second;
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
        while (Cur != Lim && Cur->first > Offset)
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
        Plugins.push_back(OffsetAndPlugin(Offset, plugin));
      }
    }

    typename P::Iterator::Ptr Enumerate() const
    {
      return typename P::Iterator::Ptr(new IteratorImpl(Plugins.begin(), Plugins.end(), Offset));
    }

    void SetOffset(std::size_t offset)
    {
      Offset = offset;
    }

    bool SetPluginLookahead(Plugin::Ptr plugin, std::size_t lookahead)
    {
      const typename PluginsList::iterator it = std::find_if(Plugins.begin(), Plugins.end(),
        boost::bind(&OffsetAndPlugin::second, _1) == plugin);
      if (it != Plugins.end())
      {
        Log::Debug(THIS_MODULE, "Disabling check of %1% for neareast %2% bytes starting from %3%", plugin->Id(), lookahead, Offset);
        it->first += lookahead;
        return true;
      }
      return false;
    }
  private:
    std::size_t Offset;
    PluginsList Plugins;
  };

  class RawDetectionPlugins : public PluginsEnumerator
  {
  public:
    typedef boost::shared_ptr<RawDetectionPlugins> Ptr;

    explicit RawDetectionPlugins(PluginsEnumerator::Ptr parent)
      : Parent(parent)
      , Players(parent->EnumeratePlayers())
      , Archives(parent->EnumerateArchives())
      , Containers(parent->EnumerateContainers())
    {
    }

    virtual Plugin::Iterator::Ptr Enumerate() const
    {
      assert(!"Should not be called");
      return Parent->Enumerate();
    }

    virtual PlayerPlugin::Iterator::Ptr EnumeratePlayers() const
    {
      return Players.Enumerate();
    }

    virtual ArchivePlugin::Iterator::Ptr EnumerateArchives() const
    {
      return Archives.Enumerate();
    }

    virtual ContainerPlugin::Iterator::Ptr EnumerateContainers() const
    {
      return Containers.Enumerate();
    }

    void SetOffset(std::size_t offset)
    {
      Containers.SetOffset(offset);
      Archives.SetOffset(offset);
      Players.SetOffset(offset);
    }

    void SetPluginLookahead(Plugin::Ptr plugin, std::size_t lookahead)
    {
      Players.SetPluginLookahead(plugin, lookahead) ||
      Archives.SetPluginLookahead(plugin, lookahead) ||
      Containers.SetPluginLookahead(plugin, lookahead);
    }
  private:
    const PluginsEnumerator::Ptr Parent;
    LookaheadPluginsStorage<PlayerPlugin> Players;
    LookaheadPluginsStorage<ArchivePlugin> Archives;
    LookaheadPluginsStorage<ContainerPlugin> Containers;
  };

  class RawDataDetector : public DataProcessor
  {
  public:
    RawDataDetector(RawDetectionPlugins& plugins, DataLocation::Ptr input, const Module::DetectCallback& callback)
      : Plugins(plugins)
      , Delegate(DataProcessor::Create(input, callback))
      , Location(input)
      , Callback(callback)
    {
    }

    virtual std::size_t ProcessContainers() const
    {
      for (ContainerPlugin::Iterator::Ptr iter = Plugins.EnumerateContainers(); iter->IsValid(); iter->Next())
      {
        const ContainerPlugin::Ptr plugin = iter->Get();
        const DetectionResult::Ptr result = plugin->Detect(Location, Callback);
        if (std::size_t usedSize = result->GetMatchedDataSize())
        {
          Log::Debug(THIS_MODULE, "Detected %1% in %2% bytes at %3%.", plugin->Id(), usedSize, Location->GetPath());
          return usedSize;
        }
        const std::size_t lookahead = result->GetLookaheadOffset();
        Plugins.SetPluginLookahead(plugin, std::max<std::size_t>(lookahead, MIN_SCAN_STEP));
      }
      return 0;
    }

    virtual std::size_t ProcessArchives() const
    {
      const Parameters::Accessor::Ptr params = GetPluginParams();
      const IO::DataContainer::Ptr data = GetData();
      for (ArchivePlugin::Iterator::Ptr iter = Plugins.EnumerateArchives(); iter->IsValid(); iter->Next())
      {
        const ArchivePlugin::Ptr plugin = iter->Get();
        const ArchiveExtractionResult::Ptr result = plugin->ExtractSubdata(*params, data);
        if (IO::DataContainer::Ptr subdata = result->GetExtractedData())
        {
          const std::size_t usedSize = result->GetMatchedDataSize();
          Log::Debug(THIS_MODULE, "Detected %1% in %2% bytes at %3%.", plugin->Id(), usedSize, Location->GetPath());
          const String subpath = Text::ARCHIVE_PLUGIN_PREFIX + plugin->Id();
          const DataLocation::Ptr subLocation = CreateNestedLocation(Location, plugin, subdata, subpath);
          Module::Detect(subLocation, Callback);
          return usedSize;
        }
        const std::size_t lookahead = result->GetLookaheadOffset();
        Plugins.SetPluginLookahead(plugin, std::max<std::size_t>(lookahead, MIN_SCAN_STEP));
        //TODO: dispatch heavy checks- return false if not enabled
      }
      return 0;
    }

    virtual std::size_t ProcessModules() const
    {
      for (PlayerPlugin::Iterator::Ptr iter = Plugins.EnumeratePlayers(); iter->IsValid(); iter->Next())
      {
        const PlayerPlugin::Ptr plugin = iter->Get();
        const Parameters::Accessor::Ptr moduleParams = GetModuleParams();
        const ModuleCreationResult::Ptr result = plugin->CreateModule(moduleParams, Location);
        if (Module::Holder::Ptr module = result->GetModule())
        {
          const std::size_t usedSize = result->GetMatchedDataSize();
          Log::Debug(THIS_MODULE, "Detected %1% in %2% bytes at %3%.", plugin->Id(), usedSize, Location->GetPath());
          ThrowIfError(Callback.ProcessModule(*Location, module));
          return usedSize;
        }
        const std::size_t lookahead = result->GetLookaheadOffset();
        Plugins.SetPluginLookahead(plugin, std::max<std::size_t>(lookahead, MIN_SCAN_STEP));
        //TODO: dispatch heavy checks- return false if not enabled
      }
      return 0;
    }
  private:
    Parameters::Accessor::Ptr GetPluginParams() const
    {
      if (!ModuleParams)
      {
        PluginsParams = Callback.GetPluginsParameters();
      }
      return PluginsParams;
    }

    IO::DataContainer::Ptr GetData() const
    {
      if (!RawData)
      {
        RawData = Location->GetData();
      }
      return RawData;
    }

    Parameters::Accessor::Ptr GetModuleParams() const
    {
      if (!ModuleParams)
      {
        ModuleParams = Callback.CreateModuleParameters(*Location);
      }
      return ModuleParams;
    }
  private:
    RawDetectionPlugins& Plugins;
    const DataProcessor::Ptr Delegate;
    const DataLocation::Ptr Location;
    const Module::DetectCallback& Callback;
    mutable Parameters::Accessor::Ptr PluginsParams;
    mutable Parameters::Accessor::Ptr ModuleParams;
    mutable IO::DataContainer::Ptr RawData;
  };

  class RawScaner : public ContainerPlugin
                  , public boost::enable_shared_from_this<RawScaner>
  {
  public:
    virtual String Id() const
    {
      return RAW_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::RAW_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return RAW_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_MULTITRACK | CAP_STOR_SCANER;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      //check only size restrictions
      return inputData.Size() >= MIN_MINIMAL_RAW_SIZE;
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
      const IO::DataContainer::Ptr rawData = input->GetData();
      const std::size_t size = rawData->Size();
      if (size < MIN_MINIMAL_RAW_SIZE)
      {
        Log::Debug(THIS_MODULE, "Size is too small (%1%)", size);
        return DetectionResult::Create(0, size);
      }

      const Parameters::Accessor::Ptr pluginParams = callback.GetPluginsParameters();
      const RawPluginParameters scanParams(*pluginParams);
      const std::size_t minRawSize = scanParams.GetMinimalSize();
      const std::size_t scanStep = scanParams.GetScanStep();
      if (size < minRawSize + scanStep)
      {
        Log::Debug(THIS_MODULE, "Size is too small (%1%)", size);
        return DetectionResult::Create(0, size);
      }

      const Log::ProgressCallback::Ptr progress(new RawProgressCallback(callback, size, input->GetPath()));
      const Module::NoProgressDetectCallbackAdapter noProgressCallback(callback);


      const PluginsEnumerator::Ptr availablePlugins = callback.GetUsedPlugins();
      const RawDetectionPlugins::Ptr usedPlugins = boost::make_shared<RawDetectionPlugins>(availablePlugins);

      const ContainerPlugin::Ptr thisPlugin = shared_from_this();
      usedPlugins->SetPluginLookahead(thisPlugin, size);
      ScanDataLocation::Ptr subLocation = boost::make_shared<ScanDataLocation>(input, thisPlugin, 0);

      DataProcessor::Ptr processor(new RawDataDetector(*usedPlugins, subLocation, noProgressCallback));
      int_t useCount = subLocation.use_count();
      while (subLocation->HasToScan(minRawSize))
      {
        const std::size_t offset = subLocation->GetOffset();
        progress->OnProgress(offset);
        usedPlugins->SetOffset(offset);
        const std::size_t usedSize = Module::Detect(*processor);
        if (subLocation.use_count() != useCount)
        {
          Log::Debug(THIS_MODULE, "Sublocation is captured. Duplicate.");
          subLocation = boost::make_shared<ScanDataLocation>(input, thisPlugin, offset);
          processor.reset(new RawDataDetector(*usedPlugins, subLocation, noProgressCallback));
          useCount = subLocation.use_count();
        }
        subLocation->Move(std::max(usedSize, scanStep));
      }
      return DetectionResult::Create(size, 0);
    }

    IO::DataContainer::Ptr Open(const Parameters::Accessor& /*commonParams*/,
      const IO::DataContainer& inData, const String& inPath, String& restPath) const
    {
      String restComp;
      const String& pathComp = IO::ExtractFirstPathComponent(inPath, restComp);
      std::size_t offset = 0;
      if (CheckIfRawPart(pathComp, offset))
      {
        restPath = restComp;
        return inData.GetSubcontainer(offset, inData.Size() - offset);
      }
      return IO::DataContainer::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterRawContainer(PluginsRegistrator& registrator)
  {
    const ContainerPlugin::Ptr plugin(new RawScaner());
    registrator.RegisterPlugin(plugin);
  }
}
