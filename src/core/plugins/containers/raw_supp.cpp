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
#include <boost/make_shared.hpp>
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

  class RawPluginParameters
  {
  public:
    explicit RawPluginParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    int_t GetRecursiveDepth() const
    {
      Parameters::IntType depth = -1;
      Accessor.FindIntValue(RAW_PLUGIN_RECURSIVE_DEPTH, depth);
      return static_cast<int_t>(depth);
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

  class DepthLimitedParameters : public Parameters::Accessor
  {
  public:
    DepthLimitedParameters(Parameters::Accessor::Ptr delegate, std::size_t depth)
      : Delegate(delegate)
      , Depth(depth)
    {
    }

    virtual bool FindIntValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      if (name == RAW_PLUGIN_RECURSIVE_DEPTH)
      {
        val = Depth;
        return true;
      }
      else
      {
        return Delegate->FindIntValue(name, val);
      }
    }

    virtual bool FindStringValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      return Delegate->FindStringValue(name, val);
    }

    virtual bool FindDataValue(const Parameters::NameType& name, Parameters::DataType& val) const
    {
      return Delegate->FindDataValue(name, val);
    }

    virtual void Process(Parameters::Visitor& visitor) const
    {
      return Delegate->Process(visitor);
    }
  private:
    const Parameters::Accessor::Ptr Delegate;
    const std::size_t Depth;
  };

  class LoggerHelper
  {
  public:
    LoggerHelper(const DetectParameters& params, const MetaContainer& data, uint_t limit)
      : Params(params)
      , Total(limit)
    {
      Message.Level = data.Plugins->CalculateContainersNesting();
      Message.Text = data.Path.empty() ? String(Text::PLUGIN_RAW_PROGRESS_NOPATH) : (Formatter(Text::PLUGIN_RAW_PROGRESS) % data.Path).str();
      Message.Progress = -1;
    }

    void operator()(uint_t offset)
    {
      const uint_t curProg = offset * 100 / Total;
      if (curProg != *Message.Progress)
      {
        Message.Progress = curProg;
        Params.ReportMessage(Message);
      }
    }
  private:
    const DetectParameters& Params;
    const uint_t Total;
    Log::MessageData Message;
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

    virtual bool Process(Parameters::Accessor::Ptr params,
      const DetectParameters& detectParams,
      const MetaContainer& data, ModuleRegion& region) const
    {
      RawPluginParameters pluginParams(*params);
      {
        //do not search right after previous raw plugin
        if (data.Plugins->Count() && data.Plugins->GetLast()->Id() == RAW_PLUGIN_ID)
        {
          return false;
        }

        //special mark to determine if plugin is called due to recursive scan
        if (pluginParams.GetRecursiveDepth() == int_t(data.Plugins->Count()))
        {
          return false;
        }
      }

      const PluginsEnumerator& enumerator = PluginsEnumerator::Instance();

      const std::size_t limit = data.Data->Size();
      //process without offset
      ModuleRegion curRegion;
      {
        const Parameters::Accessor::Ptr newParams =
          boost::make_shared<DepthLimitedParameters>(params, data.Plugins->Count());
        enumerator.DetectModules(newParams, detectParams, data, curRegion);
      }
      const std::size_t minRawSize = pluginParams.GetMinimalSize();

      //check for further scanning possibility
      if (curRegion.Size != 0 &&
          curRegion.Offset + curRegion.Size + minRawSize >= limit)
      {
        region.Offset = 0;
        region.Size = limit;
        return true;
      }

      const std::size_t scanStep = pluginParams.GetScanStep();

      //to determine was scaner really affected
      bool wasResult = curRegion.Size != 0;

      MetaContainer subcontainer;
      subcontainer.Plugins = data.Plugins->Clone();
      subcontainer.Plugins->Add(shared_from_this());

      LoggerHelper logger(detectParams, data, limit);
      for (std::size_t offset = std::max(curRegion.Offset + curRegion.Size, std::size_t(1));
        offset + minRawSize < limit;
        offset += std::max(curRegion.Offset + curRegion.Size, std::size_t(scanStep)))
      {
        logger(offset);
        subcontainer.Data = data.Data->GetSubcontainer(offset, limit - offset);
        subcontainer.Path = IO::AppendPath(data.Path, CreateRawPart(offset));
        enumerator.DetectModules(params, detectParams, subcontainer, curRegion);
        wasResult = wasResult || curRegion.Size != 0;
      }
      if (wasResult)
      {
        region.Offset = 0;
        region.Size = limit;
      }
      return wasResult;
    }

    IO::DataContainer::Ptr Open(const Parameters::Accessor& /*commonParams*/,
      const MetaContainer& inData, const String& inPath, String& restPath) const
    {
      //do not open right after self
      if (inData.Plugins->Count() &&
          inData.Plugins->GetLast()->Id() == RAW_PLUGIN_ID)
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
