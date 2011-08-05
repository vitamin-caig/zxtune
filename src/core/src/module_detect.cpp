/*
Abstract:
  Module detection in container

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "callback.h"
#include "core.h"
#include "core/plugins/plugins_types.h"
//common includes
#include <error.h>
#include <logging.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <src/core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const std::string THIS_MODULE("Core::Detection");

  const String ARCHIVE_PLUGIN_PREFIX(Text::ARCHIVE_PLUGIN_PREFIX);

  String EncodeArchivePluginToPath(const String& pluginId)
  {
    return ARCHIVE_PLUGIN_PREFIX + pluginId;
  }

  class OpenModuleCallback : public Module::DetectCallback
  {
  public:
    OpenModuleCallback(Parameters::Accessor::Ptr moduleParams)
      : ModuleParams(moduleParams)
    {
    }

    virtual PluginsEnumerator::Ptr GetUsedPlugins() const
    {
      assert(!"Should not be called");
      return PluginsEnumerator::Ptr();
    }

    virtual Parameters::Accessor::Ptr GetPluginsParameters() const
    {
      return Parameters::Container::Create();
    }

    virtual Parameters::Accessor::Ptr CreateModuleParameters(const DataLocation& /*location*/) const
    {
      return ModuleParams;
    }

    virtual Error ProcessModule(const DataLocation& /*location*/, Module::Holder::Ptr holder) const
    {
      Result = holder;
      return Error();
    }

    virtual Log::ProgressCallback* GetProgress() const
    {
      return 0;
    }

    Module::Holder::Ptr GetResult() const
    {
      return Result;
    }
  private:
    const Parameters::Accessor::Ptr ModuleParams;
    mutable Module::Holder::Ptr Result;
  };
  
  template<class T>
  std::size_t DetectByPlugins(typename T::Iterator::Ptr plugins, DataLocation::Ptr location, const Module::DetectCallback& callback)
  {
    for (; plugins->IsValid(); plugins->Next())
    {
      const typename T::Ptr plugin = plugins->Get();
      const DetectionResult::Ptr result = plugin->Detect(location, callback);
      if (std::size_t usedSize = result->GetMatchedDataSize())
      {
        Log::Debug(THIS_MODULE, "Detected %1% in %2% bytes at %3%.", plugin->GetDescription()->Id(), usedSize, location->GetPath()->AsString());
        return usedSize;
      }
    }
    return 0;
  }

  class DetectionResultImpl : public DetectionResult
  {
  public:
    explicit DetectionResultImpl(std::size_t matchedSize, std::size_t unmatchedSize)
      : MatchedSize(matchedSize)
      , UnmatchedSize(unmatchedSize)
    {
    }

    virtual std::size_t GetMatchedDataSize() const
    {
      return MatchedSize;
    }

    virtual std::size_t GetLookaheadOffset() const
    {
      return UnmatchedSize;
    }
  private:
    const std::size_t MatchedSize;
    const std::size_t UnmatchedSize;
  };

  class UnmatchedDetectionResult : public DetectionResult
  {
  public:
    UnmatchedDetectionResult(DataFormat::Ptr format, IO::DataContainer::Ptr data)
      : Format(format)
      , RawData(data)
    {
    }

    virtual std::size_t GetMatchedDataSize() const
    {
      return 0;
    }

    virtual std::size_t GetLookaheadOffset() const
    {
      return Format->Search(RawData->Data(), RawData->Size());
    }
  private:
    const DataFormat::Ptr Format;
    const IO::DataContainer::Ptr RawData;
  };
}

namespace ZXTune
{
  DetectionResult::Ptr DetectionResult::CreateMatched(std::size_t matchedSize)
  {
    return boost::make_shared<DetectionResultImpl>(matchedSize, 0);
  }

  DetectionResult::Ptr DetectionResult::CreateUnmatched(DataFormat::Ptr format, IO::DataContainer::Ptr data)
  {
    return boost::make_shared<UnmatchedDetectionResult>(format, data);
  }

  DetectionResult::Ptr DetectionResult::CreateUnmatched(std::size_t unmatchedSize)
  {
    return boost::make_shared<DetectionResultImpl>(0, unmatchedSize);
  }

  namespace Module
  {
    Holder::Ptr Open(DataLocation::Ptr location, PluginsEnumerator::Ptr usedPlugins, Parameters::Accessor::Ptr moduleParams)
    {
      const OpenModuleCallback callback(moduleParams);
      DetectByPlugins<PlayerPlugin>(usedPlugins->EnumeratePlayers(), location, callback);
      return callback.GetResult();
    }

    std::size_t Detect(DataLocation::Ptr location, const DetectCallback& callback)
    {
      const PluginsEnumerator::Ptr usedPlugins = callback.GetUsedPlugins();
      if (std::size_t usedSize = DetectByPlugins<ArchivePlugin>(usedPlugins->EnumerateArchives(), location, callback))
      {
        return usedSize;
      }
      return DetectByPlugins<PlayerPlugin>(usedPlugins->EnumeratePlayers(), location, callback);
    }
  }
}
