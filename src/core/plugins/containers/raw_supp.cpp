/*
Abstract:
  RAW containers support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <core/plugins/enumerator.h>
//common includes
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <core/error_codes.h>
#include <core/plugin_attrs.h>
#include <core/plugins_parameters.h>
#include <io/container.h>
#include <io/fs_tools.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>

#define FILE_TAG 7E0CBD98

namespace
{
  using namespace ZXTune;

  const Char RAW_PLUGIN_ID[] = {'R', 'A', 'W', 0};
  const String RAW_PLUGIN_VERSION(FromStdString("$Rev$"));

  const uint_t MIN_SCAN_STEP = 1;
  const uint_t MAX_SCAN_STEP = 256;
  const std::size_t MIN_MINIMAL_RAW_SIZE = 128;

  //fake parameter name used to prevent plugin recursive call while first pass processing
  const Char RAW_PLUGIN_RECURSIVE_DEPTH[] =
  {
    'r','a','w','_','s','c','a','n','e','r','_','r','e','c','u','r','s','i','o','n','_','d','e','p','t','h','\0'
  };

  const Char RAW_REST_PART[] = {'.', 'r', 'a', 'w', 0};

  //\d+\.raw
  inline bool CheckIfRawPart(const String& str, std::size_t& offset)
  {
    std::size_t res = 0;
    String rest;
    InStringStream stream(str);
    return (stream >> res) && (stream >> rest) && (rest == RAW_REST_PART) &&
    // path '0.raw' is enabled now
      (offset = res, true);
  }

  inline String CreateRawPart(std::size_t offset)
  {
    OutStringStream stream;
    stream << offset << RAW_REST_PART;
    return stream.str();
  }

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

    virtual bool Process(const Parameters::Map& commonParams, 
      const DetectParameters& detectParams,
      const MetaContainer& data, ModuleRegion& region) const
    {
      {
        Parameters::IntType depth = 0;
        //do not search right after previous raw plugin
        if ((!data.PluginsChain.empty() && data.PluginsChain.back()->Id() == RAW_PLUGIN_ID) ||
            //special mark to determine if plugin is called due to recursive scan
            (Parameters::FindByName(commonParams, RAW_PLUGIN_RECURSIVE_DEPTH, depth) &&
             depth == Parameters::IntType(data.PluginsChain.size())))
        {
          return false;
        }
      }

      const PluginsEnumerator& enumerator = PluginsEnumerator::Instance();

      const std::size_t limit = data.Data->Size();
      //process without offset
      ModuleRegion curRegion;
      {
        Parameters::Map newParams(commonParams);
        newParams[RAW_PLUGIN_RECURSIVE_DEPTH] = data.PluginsChain.size();
        enumerator.DetectModules(newParams, detectParams, data, curRegion);
      }
      Parameters::Helper parameters(commonParams);
      const std::size_t minRawSize = static_cast<std::size_t>(
      parameters.GetValue(
        Parameters::ZXTune::Core::Plugins::Raw::MIN_SIZE,
        Parameters::ZXTune::Core::Plugins::Raw::MIN_SIZE_DEFAULT));
      if (minRawSize < Parameters::IntType(MIN_MINIMAL_RAW_SIZE))
      {
        throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
          Text::RAW_ERROR_INVALID_MIN_SIZE, minRawSize, MIN_MINIMAL_RAW_SIZE);
      }

      //check for further scanning possibility
      if (curRegion.Size != 0 &&
          curRegion.Offset + curRegion.Size + minRawSize >= limit)
      {
        region.Offset = 0;
        region.Size = limit;
        return true;
      }

      const std::size_t scanStep = static_cast<std::size_t>(
        parameters.GetValue(
          Parameters::ZXTune::Core::Plugins::Raw::SCAN_STEP,
          Parameters::ZXTune::Core::Plugins::Raw::SCAN_STEP_DEFAULT));
      if (scanStep < Parameters::IntType(MIN_SCAN_STEP) ||
          scanStep > Parameters::IntType(MAX_SCAN_STEP))
      {
        throw MakeFormattedError(THIS_LINE, Module::ERROR_INVALID_PARAMETERS,
          Text::RAW_ERROR_INVALID_STEP, scanStep, MIN_SCAN_STEP, MAX_SCAN_STEP);
      }

      // progress-related
      const bool showMessage = detectParams.Logger != 0;
      Log::MessageData message;
      if (showMessage)
      {
        message.Level = CalculateContainersNesting(data.PluginsChain);
        message.Text = data.Path.empty() ? String(Text::PLUGIN_RAW_PROGRESS_NOPATH) : (Formatter(Text::PLUGIN_RAW_PROGRESS) % data.Path).str();
        message.Progress = -1;
      }

      //to determine was scaner really affected
      bool wasResult = curRegion.Size != 0;

      MetaContainer subcontainer;
      subcontainer.PluginsChain = data.PluginsChain;
      subcontainer.PluginsChain.push_back(shared_from_this());
      for (std::size_t offset = std::max(curRegion.Offset + curRegion.Size, std::size_t(1));
        offset + minRawSize < limit;
        offset += std::max(curRegion.Offset + curRegion.Size, std::size_t(scanStep)))
      {
        const uint_t curProg = offset * 100 / limit;
        if (showMessage && curProg != *message.Progress)
        {
          message.Progress = curProg;
          detectParams.Logger(message);
        }
        subcontainer.Data = data.Data->GetSubcontainer(offset, limit - offset);
        subcontainer.Path = IO::AppendPath(data.Path, CreateRawPart(offset));
        enumerator.DetectModules(commonParams, detectParams, subcontainer, curRegion);
        wasResult = wasResult || curRegion.Size != 0;
      }
      if (wasResult)
      {
        region.Offset = 0;
        region.Size = limit;
      }
      return wasResult;
    }

    IO::DataContainer::Ptr Open(const Parameters::Map& /*commonParams*/, 
      const MetaContainer& inData, const String& inPath, String& restPath) const
    {
      //do not open right after self
      if (!inData.PluginsChain.empty() && 
          inData.PluginsChain.back()->Id() == RAW_PLUGIN_ID)
      {
        return IO::DataContainer::Ptr();
      }
      String restComp;
      const String& pathComp = IO::ExtractFirstPathComponent(inPath, restComp);
      std::size_t offset = 0;
      if (CheckIfRawPart(pathComp, offset))
      {
        restPath = restComp;
        return inData.Data->GetSubcontainer(offset, inData.Data->Size() - offset);
      }
      return IO::DataContainer::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterRawContainer(PluginsEnumerator& enumerator)
  {
    const ContainerPlugin::Ptr plugin(new RawScaner());
    enumerator.RegisterPlugin(plugin);
  }
}
