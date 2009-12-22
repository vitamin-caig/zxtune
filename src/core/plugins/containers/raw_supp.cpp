/*
Abstract:
  RAW containers support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "../enumerator.h"

#include <tools.h>

#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <core/error_codes.h>

#include <io/container.h>
#include <io/fs_tools.h>

#include <boost/bind.hpp>

#include <text/core.h>
#include <text/plugins.h>

#define FILE_TAG 7E0CBD98

namespace
{
  using namespace ZXTune;

  const Char RAW_PLUGIN_ID[] = {'R', 'a', 'w', 0};
  
  const String TEXT_RAW_VERSION(FromChar("Revision: $Rev$"));
  
  const std::size_t MINIMAL_RAW_SIZE = 16;
  
  const Char RAW_REST_PART[] = {'.', 'r', 'a', 'w', 0};
  
  bool ChainedFilter(const PluginInformation& info, const DetectParameters::FilterFunc& nextFilter)
  {
    return info.Id == RAW_PLUGIN_ID || (nextFilter && nextFilter(info));
  }
  
  //\d+\.raw
  inline bool CheckIfRawPart(const String& str, std::size_t& offset)
  {
    std::size_t res;
    String rest;
    InStringStream stream(str);
    return (stream >> res) && (stream >> rest) && (rest == RAW_REST_PART) && (offset = res, true);
  }
  
  inline String CreateRawPart(std::size_t offset)
  {
    OutStringStream stream;
    stream << static_cast<unsigned>(offset) << RAW_REST_PART;
    return stream.str();
  }
  
  Error ProcessRawContainer(const MetaContainer& data, const DetectParameters& params, ModuleRegion& region)
  {
    //do not search right after previous raw plugin
    if (!data.PluginsChain.empty() && *data.PluginsChain.rbegin() == RAW_PLUGIN_ID)
    {
      return Error(THIS_LINE, Module::ERROR_FIND_CONTAINER_PLUGIN);
    }
    const PluginsEnumerator& enumerator(PluginsEnumerator::Instance());
    DetectParameters filteredParams;
    filteredParams.Filter = boost::bind(ChainedFilter, _1, params.Filter);
    filteredParams.Callback = params.Callback;
    filteredParams.Logger = params.Logger;
    
    //process without offset
    ModuleRegion curRegion;
    if (const Error& err = enumerator.DetectModules(filteredParams, data, curRegion))
    {
      return err;
    }
    bool wasResult = curRegion.Size != 0;
    const std::size_t limit(data.Data->Size());
    for (std::size_t offset = curRegion.Offset + curRegion.Size;
      offset < limit - MINIMAL_RAW_SIZE; offset += std::max(curRegion.Offset + curRegion.Size, std::size_t(1)))
    {
      MetaContainer subcontainer;
      subcontainer.Data = data.Data->GetSubcontainer(offset, limit - offset);
      subcontainer.Path = IO::AppendPath(data.Path, CreateRawPart(offset));
      subcontainer.PluginsChain = data.PluginsChain;
      subcontainer.PluginsChain.push_back(RAW_PLUGIN_ID);
      if (const Error& err = enumerator.DetectModules(filteredParams, subcontainer, curRegion))
      {
        return err;
      }
      wasResult = wasResult || curRegion.Size != 0;
    }
    if (wasResult)
    {
      region.Offset = 0;
      region.Size = limit;
      return Error();
    }
    else
    {
      return Error(THIS_LINE, Module::ERROR_FIND_CONTAINER_PLUGIN);
    }
  }
  
  bool OpenRawContainer(const IO::DataContainer& inData, const String& inPath, IO::DataContainer::Ptr& outData, String& restPath)
  {
    String restComp;
    const String& pathComp = IO::ExtractFirstPathComponent(inPath, restComp);
    std::size_t offset(0);
    if (CheckIfRawPart(pathComp, offset))
    {
      outData = inData.GetSubcontainer(offset, inData.Size() - offset);
      restPath = restComp;
      return true;
    }
    return false;
  }
}

namespace ZXTune
{
  void RegisterRawContainer(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    info.Id = RAW_PLUGIN_ID;
    info.Description = TEXT_RAW_INFO;
    info.Version = TEXT_RAW_VERSION;
    info.Capabilities = CAP_STOR_MULTITRACK | CAP_STOR_SCANER;
    enumerator.RegisterContainerPlugin(info, OpenRawContainer, ProcessRawContainer);
  }
}
