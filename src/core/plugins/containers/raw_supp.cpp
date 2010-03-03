/*
Abstract:
  RAW containers support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "../enumerator.h"

#include <error_tools.h>
#include <logging.h>
#include <tools.h>

#include <core/error_codes.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <io/container.h>
#include <io/fs_tools.h>

#include <boost/bind.hpp>

#include <text/core.h>
#include <text/plugins.h>

#define FILE_TAG 7E0CBD98

namespace
{
  using namespace ZXTune;

  const Char RAW_PLUGIN_ID[] = {'R', 'A', 'W', 0};
  
  const String TEXT_RAW_VERSION(FromStdString("$Rev$"));
  
  const uint_t MIN_SCAN_STEP = 1;
  const uint_t MAX_SCAN_STEP = 256;
  const std::size_t MINIMAL_RAW_SIZE = 128;
  
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
    stream << offset << RAW_REST_PART;
    return stream.str();
  }
  
  Error ProcessRawContainer(const Parameters::Map& commonParams, const DetectParameters& detectParams,
    const MetaContainer& data, ModuleRegion& region)
  {
    //do not search right after previous raw plugin
    if (!data.PluginsChain.empty() && data.PluginsChain.back() == RAW_PLUGIN_ID)
    {
      return Error(THIS_LINE, Module::ERROR_FIND_CONTAINER_PLUGIN);
    }
    const PluginsEnumerator& enumerator(PluginsEnumerator::Instance());
    
    //process without offset
    ModuleRegion curRegion;
    {
      DetectParameters filteredParams;
      filteredParams.Filter = boost::bind(ChainedFilter, _1, detectParams.Filter);
      filteredParams.Callback = detectParams.Callback;
      filteredParams.Logger = detectParams.Logger;
      
      if (const Error& err = enumerator.DetectModules(commonParams, filteredParams, data, curRegion))
      {
        return err;
      }
    }
    std::size_t scanStep = static_cast<std::size_t>(Parameters::ZXTune::Core::Plugins::Raw::SCAN_STEP_DEFAULT);
    if (const Parameters::IntType* const stepParam =
      Parameters::FindByName<Parameters::IntType>(commonParams, Parameters::ZXTune::Core::Plugins::Raw::SCAN_STEP))
    {
      if (*stepParam < Parameters::IntType(MIN_SCAN_STEP) ||
          *stepParam > Parameters::IntType(MAX_SCAN_STEP))
      {
        throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
          TEXT_RAW_ERROR_INVALID_STEP, *stepParam, MIN_SCAN_STEP, MAX_SCAN_STEP);
      }
      scanStep = static_cast<std::size_t>(*stepParam);
    }

    // progress-related
    const bool showMessage(detectParams.Logger);
    Log::MessageData message;
    if (showMessage)
    {
      message.Level = data.PluginsChain.size();
      message.Text = data.Path.empty() ? String(TEXT_PLUGIN_RAW_PROGRESS_NOPATH) : (Formatter(TEXT_PLUGIN_RAW_PROGRESS) % data.Path).str();
      message.Progress = -1;
    }

    bool wasResult = curRegion.Size != 0;
    const std::size_t limit(data.Data->Size());

    MetaContainer subcontainer;
    subcontainer.PluginsChain = data.PluginsChain;
    subcontainer.PluginsChain.push_back(RAW_PLUGIN_ID);
    for (std::size_t offset = std::max(curRegion.Offset + curRegion.Size, std::size_t(1));
      offset + MINIMAL_RAW_SIZE < limit; offset += std::max(curRegion.Offset + curRegion.Size, std::size_t(1)))
    {
      const uint_t curProg = offset * 100 / limit;
      if (showMessage && curProg != *message.Progress)
      {
        message.Progress = curProg;
        detectParams.Logger(message);
      }
      subcontainer.Data = data.Data->GetSubcontainer(offset, limit - offset);
      subcontainer.Path = IO::AppendPath(data.Path, CreateRawPart(offset));
      if (const Error& err = enumerator.DetectModules(commonParams, detectParams, subcontainer, curRegion))
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
  
  bool OpenRawContainer(const Parameters::Map& /*commonParams*/, const MetaContainer& inData, const String& inPath,
    IO::DataContainer::Ptr& outData, String& restPath)
  {
    //do not open right after self
    if (!inData.PluginsChain.empty() && inData.PluginsChain.back() == RAW_PLUGIN_ID)
    {
      return false;
    }
    String restComp;
    const String& pathComp = IO::ExtractFirstPathComponent(inPath, restComp);
    std::size_t offset(0);
    if (CheckIfRawPart(pathComp, offset))
    {
      outData = inData.Data->GetSubcontainer(offset, inData.Data->Size() - offset);
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
