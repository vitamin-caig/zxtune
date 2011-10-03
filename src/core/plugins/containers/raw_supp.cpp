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
  using namespace ZXTune;

  const std::string THIS_MODULE("Core::RawScaner");

  const uint_t MIN_SCAN_STEP = 1;
  const uint_t MAX_SCAN_STEP = 256;
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

    ScanDataLocation(DataLocation::Ptr parent, Plugin::Ptr subPlugin, std::size_t offset)
      : Parent(parent)
      , Subdata(boost::make_shared<ScanDataContainer>(Parent->GetData(), offset))
      , Subplugin(subPlugin)
    {
    }

    virtual Binary::Container::Ptr GetData() const
    {
      return Subdata;
    }

    virtual DataPath::Ptr GetPath() const
    {
      const DataPath::Ptr parentPath = Parent->GetPath();
      if (std::size_t offset = Subdata->GetOffset())
      {
        const String subPath = RawPath.Build(offset);
        return CreateMergedDataPath(parentPath, subPath);
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

  class RawDetectionPlugins
  {
  public:
    RawDetectionPlugins(PluginsEnumerator::Ptr parent, const String& denied)
      : Players(parent->EnumeratePlayers())
      , Archives(parent->EnumerateArchives())
    {
      Archives.SetPluginLookahead(denied, ~std::size_t(0));
    }

    std::size_t Detect(DataLocation::Ptr input, const Module::DetectCallback& callback)
    {
      const DetectionResult::Ptr detectedArchives = DetectIn(Archives, input, callback);
      if (std::size_t matched = detectedArchives->GetMatchedDataSize())
      {
        return matched;
      }
      const DetectionResult::Ptr detectedModules = DetectIn(Players, input, callback);
      if (std::size_t matched = detectedModules->GetMatchedDataSize())
      {
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
    DetectionResult::Ptr DetectIn(LookaheadPluginsStorage<T>& container, DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
      for (typename T::Iterator::Ptr iter = container.Enumerate(); iter->IsValid(); iter->Next())
      {
        const typename T::Ptr plugin = iter->Get();
        const DetectionResult::Ptr result = plugin->Detect(input, callback);
        const String id = plugin->GetDescription()->Id();
        if (std::size_t usedSize = result->GetMatchedDataSize())
        {
          Log::Debug(THIS_MODULE, "Detected %1% in %2% bytes at %3%.", id, usedSize, input->GetPath()->AsString());
          return result;
        }
        const std::size_t lookahead = result->GetLookaheadOffset();
        container.SetPluginLookahead(id, std::max<std::size_t>(lookahead, MIN_SCAN_STEP));
      }
      const std::size_t minLookahead = container.GetMinimalPluginLookahead();
      return DetectionResult::CreateUnmatched(minLookahead);
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

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
      const Binary::Container::Ptr rawData = input->GetData();
      const std::size_t size = rawData->Size();
      if (size < MIN_MINIMAL_RAW_SIZE)
      {
        Log::Debug(THIS_MODULE, "Size is too small (%1%)", size);
        return DetectionResult::CreateUnmatched(size);
      }

      const Parameters::Accessor::Ptr pluginParams = callback.GetPluginsParameters();
      const RawPluginParameters scanParams(*pluginParams);
      const std::size_t minRawSize = scanParams.GetMinimalSize();
      const std::size_t scanStep = scanParams.GetScanStep();
      if (size < minRawSize + scanStep)
      {
        Log::Debug(THIS_MODULE, "Size is too small (%1%)", size);
        return DetectionResult::CreateUnmatched(size);
      }

      const String currentPath = input->GetPath()->AsString();
      Log::Debug(THIS_MODULE, "Detecting modules in raw data at '%1%'", currentPath);
      const Log::ProgressCallback::Ptr progress(new RawProgressCallback(callback, static_cast<uint_t>(size), currentPath));
      const Module::DetectCallback& noProgressCallback = Module::CustomProgressDetectCallbackAdapter(callback);

      const PluginsEnumerator::Ptr availablePlugins = PluginsEnumerator::Create();
      RawDetectionPlugins usedPlugins(availablePlugins, Description->Id());

      ScanDataLocation::Ptr subLocation = boost::make_shared<ScanDataLocation>(input, Description, 0);

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
          subLocation = boost::make_shared<ScanDataLocation>(input, Description, offset);
        }
        subLocation->Move(std::max(bytesToSkip, scanStep));
      }
      return DetectionResult::CreateMatched(size);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& /*commonParams*/, DataLocation::Ptr location, const DataPath& inPath) const
    {
      const String& pathComp = inPath.GetFirstComponent();
      std::size_t offset = 0;
      if (RawPath.GetIndex(pathComp, offset))
      {
        const Binary::Container::Ptr inData = location->GetData();
        const Binary::Container::Ptr subData = inData->GetSubcontainer(offset, inData->Size() - offset);
        return CreateNestedLocation(location, subData, Description, pathComp); 
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
