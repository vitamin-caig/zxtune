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
#include <core/plugins/enumerator.h>
#include <core/plugins/utils.h>
//common includes
#include <formatter.h>
#include <logging.h>
//library includes
#include <core/module_detect.h>
#include <io/fs_tools.h>
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
      , Path(path)
      , Format(new Formatter(Path.empty() ? Text::CONTAINER_PLUGIN_PROGRESS_NOPATH : Text::CONTAINER_PLUGIN_PROGRESS))
      , Current()
    {
      (*Format) % plugin.Id();
    }

    void operator()(const TRDos::FileEntry& cur)
    {
      if (Callback)
      {
        const String text = Path.empty()
          ? (Formatter(*Format) % cur.Name).str()
          : (Formatter(*Format) % cur.Name % Path).str();
        Callback->OnProgress(Current++, text);
      }
    }
  private:
    Log::ProgressCallback* const Callback;
    const String Path;
    const std::auto_ptr<Formatter> Format;
    uint_t Current;
  };
}

namespace TRDos
{
  void ProcessEntries(Parameters::Accessor::Ptr params, const DetectParameters& detectParams, const MetaContainer& data, Plugin::Ptr plugin, const FilesSet& files)
  {
    MetaContainer subcontainer;
    subcontainer.Plugins = data.Plugins->Clone();
    subcontainer.Plugins->Add(plugin);

    const Log::ProgressCallback::Ptr progress = CreateProgressCallback(detectParams, files.GetEntriesCount());
    LoggerHelper logger(progress.get(), *plugin, data.Path);
    const ZXTune::PluginsEnumeratorOld& oldEnumerator = ZXTune::PluginsEnumeratorOld::Instance();
    for (TRDos::FilesSet::Iterator::Ptr it = files.GetEntries(); it->IsValid(); it->Next())
    {
      const TRDos::FileEntry& entry = it->Get();
      subcontainer.Data = data.Data->GetSubcontainer(entry.Offset, entry.Size);
      subcontainer.Path = IO::AppendPath(data.Path, entry.Name);
      logger(entry);
      oldEnumerator.DetectModules(params, detectParams, subcontainer);
    }
  }
}
