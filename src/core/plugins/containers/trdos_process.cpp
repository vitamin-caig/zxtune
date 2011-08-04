/*
Abstract:
  TrDOS entries processing implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "trdos_process.h"
#include "core/plugins/utils.h"
#include "core/src/callback.h"
//common includes
#include <format.h>
#include <logging.h>
//library includes
#include <core/module_detect.h>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  class LoggerHelper
  {
  public:
    LoggerHelper(Log::ProgressCallback* callback, const Plugin& plugin, const String& path)
      : Callback(callback)
      , Id(plugin.Id())
      , Path(path)
      , Current()
    {
    }

    void operator()(const TRDos::File& cur)
    {
      if (Callback)
      {
        const String text = Path.empty()
          ? Strings::Format(Text::CONTAINER_PLUGIN_PROGRESS_NOPATH, Id, cur.GetName())
          : Strings::Format(Text::CONTAINER_PLUGIN_PROGRESS, Id, cur.GetName(), Path);
        Callback->OnProgress(Current++, text);
      }
    }
  private:
    Log::ProgressCallback* const Callback;
    const String Id;
    const String Path;
    uint_t Current;
  };
}

namespace TRDos
{
  void ProcessEntries(ZXTune::DataLocation::Ptr location, const ZXTune::Module::DetectCallback& callback, ZXTune::Plugin::Ptr plugin, const Catalogue& files)
  {
    const ZXTune::IO::DataContainer::Ptr data = location->GetData();
    const Log::ProgressCallback::Ptr progress = CreateProgressCallback(callback, files.GetFilesCount());
    LoggerHelper logger(progress.get(), *plugin, location->GetPath()->AsString());
    const ZXTune::Module::NoProgressDetectCallbackAdapter noProgressCallback(callback);
    for (Catalogue::Iterator::Ptr it = files.GetFiles(); it->IsValid(); it->Next())
    {
      const TRDos::File::Ptr file = it->Get();
      const IO::DataContainer::Ptr subData = file->GetData();
      const String subPath = file->GetName();
      const ZXTune::DataLocation::Ptr subLocation = CreateNestedLocation(location, subData, plugin, subPath);
      logger(*file);
      ZXTune::Module::Detect(subLocation, noProgressCallback);
    }
  }
}
