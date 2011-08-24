/*
Abstract:
  Source data provider implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune123 application based on zxtune library
*/

//local includes
#include "console.h"
#include "source.h"
#include <apps/base/app.h>
#include <apps/base/error_codes.h>
#include <apps/base/parsing.h>
#include <apps/base/playitem.h>
//common includes
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <core/core_parameters.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/module_detect.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <io/fs_tools.h>
#include <io/provider.h>
#include <io/providers_parameters.h>
//std includes
#include <iomanip>
#include <iostream>
//boost includes
#include <boost/bind.hpp>
#include <boost/unordered_set.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>
//text includes
#include "text/text.h"

#define FILE_TAG 9EDFE3AF

namespace
{
  const Char DELIMITERS[] = {',', ';', ':', '\0'};

  typedef std::set<String> StringSet;

  void OutputString(uint_t width, const String& text)
  {
    const String::size_type curSize = text.size();
    if (curSize < width)
    {
      StdOut << text << String(width - curSize - 1, ' ') << '\r';
    }
    else
    {
      StdOut << text << '\n';
    }
    StdOut << std::flush;
  }

  class ProgressCallbackImpl : public Log::ProgressCallback
  {
  public:
    ProgressCallbackImpl()
      : Cons(Console::Self())
    {
    }

    virtual void OnProgress(uint_t current)
    {
      static const Char EMPTY[] = {0};
      OnProgress(current, EMPTY);
    }
    virtual void OnProgress(uint_t current, const String& message)
    {
      CheckForExit();
      if (const uint_t currentWidth = GetCurrentWidth())
      {
        String text = message;
        text += Strings::Format(Text::PROGRESS_FORMAT, current);
        OutputString(currentWidth, text);
      }
    }
  private:
    void CheckForExit() const
    {
      if (Cons.GetPressedKey() == Console::INPUT_KEY_CANCEL)
      {
        Cons.WaitForKeyRelease();
        throw Error(THIS_LINE, ZXTune::Module::ERROR_DETECT_CANCELED);
      }
    }

    uint_t GetCurrentWidth() const
    {
      const int_t width = Cons.GetSize().first;
      return width >= 0 ? static_cast<uint_t>(width) : 0;
    }
  private:
    const Console& Cons;
  };

  void Parse(const StringSet& allplugs, const String& str, StringSet& plugs, uint_t& caps)
  {
    typedef std::pair<uint32_t, String> CapsPair;
    static const CapsPair CAPABILITIES[] =
    {
      CapsPair(ZXTune::CAP_DEV_AYM, Text::INFO_CAP_AYM),
      CapsPair(ZXTune::CAP_DEV_TS, Text::INFO_CAP_TS),
      CapsPair(ZXTune::CAP_DEV_BEEPER, Text::INFO_CAP_BEEPER),
      CapsPair(ZXTune::CAP_DEV_FM, Text::INFO_CAP_FM),
      CapsPair(ZXTune::CAP_DEV_1DAC | ZXTune::CAP_DEV_2DAC | ZXTune::CAP_DEV_4DAC, Text::INFO_CAP_DAC),

      //remove module capability- it cannot be enabled or disabled
      CapsPair(ZXTune::CAP_STOR_CONTAINER, Text::INFO_CAP_CONTAINER),
      CapsPair(ZXTune::CAP_STOR_MULTITRACK, Text::INFO_CAP_MULTITRACK),
      CapsPair(ZXTune::CAP_STOR_SCANER, Text::INFO_CAP_SCANER),
      CapsPair(ZXTune::CAP_STOR_PLAIN, Text::INFO_CAP_PLAIN),

      CapsPair(ZXTune::CAP_CONV_RAW, Text::INFO_CONV_RAW),
      CapsPair(ZXTune::CAP_CONV_PSG, Text::INFO_CONV_PSG)
    };

    StringSet tmpPlugs;
    uint32_t tmpCaps = 0;

    if (!str.empty())
    {
      StringArray splitted;
      boost::algorithm::split(splitted, str, boost::algorithm::is_any_of(DELIMITERS));
      for (StringArray::const_iterator it = splitted.begin(), lim = splitted.end(); it != lim; ++it)
      {
        //check if capability
        const CapsPair* const capIter = std::find_if(CAPABILITIES, ArrayEnd(CAPABILITIES),
          boost::bind(&CapsPair::second, _1) == *it);
        if (ArrayEnd(CAPABILITIES) != capIter)
        {
          tmpCaps |= capIter->first;
        }
        else if (allplugs.count(*it))
        {
          tmpPlugs.insert(*it);
        }
        else
        {
          throw MakeFormattedError(THIS_LINE, INVALID_PARAMETER, Text::ERROR_INVALID_PARAMETER, *it, str);
        }
      }
    }
    plugs.swap(tmpPlugs);
    caps = tmpCaps;
  }

  class PluginsFilter
  {
  public:
    PluginsFilter(const String& allowed, const String& denied)
    {
      std::map<String, uint_t> plug2Cap;
      StringSet allplugs;
      for (ZXTune::Plugin::Iterator::Ptr plugs = ZXTune::EnumeratePlugins(); plugs->IsValid(); plugs->Next())
      {
        const ZXTune::Plugin::Ptr plugin = plugs->Get();
        allplugs.insert(plugin->Id());
        plug2Cap[plugin->Id()] = plugin->Capabilities();
      }
      uint_t enabledCaps = 0;
      StringSet enabledPlugins;
      uint_t disabledCaps = 0;
      StringSet disabledPlugins;
      Parse(allplugs, allowed, enabledPlugins, enabledCaps);
      Parse(allplugs, denied, disabledPlugins, disabledCaps);
      for (std::map<String, uint_t>::const_iterator it = plug2Cap.begin(), lim = plug2Cap.end(); it != lim; ++it)
      {
        const String id = it->first;
        const uint_t caps = it->second;
        if (enabledPlugins.count(id) || 0 != (caps & enabledCaps))
        {
          //enabled explicitly
          continue;
        }
        else if (disabledPlugins.count(id) || 0 != (caps & disabledCaps))
        {
          //disabled explicitly
          FilteredPlugins.insert(id);
        }
        //enable all if there're no explicit enables
        if (!(enabledPlugins.empty() && !enabledCaps))
        {
          FilteredPlugins.insert(id);
        }
      }
    }

    bool IsPluginFiltered(const ZXTune::Plugin& plugin) const
    {
      return !FilteredPlugins.empty() && 
             0 != FilteredPlugins.count(plugin.Id());
    }
  private:
    boost::unordered_set<String> FilteredPlugins;
  };

  class DetectParametersImpl : public ZXTune::DetectParameters
  {
  public:
    DetectParametersImpl(Parameters::Accessor::Ptr params, const PluginsFilter& filter, const String& path, const OnItemCallback& callback, bool showLogs)
      : Params(params)
      , Filter(filter)
      , Path(path)
      , Callback(callback)
      , ProgressCallback(showLogs ? new ProgressCallbackImpl() : 0)
    {
    }

    virtual bool FilterPlugin(const ZXTune::Plugin& plugin) const
    {
      return Filter.IsPluginFiltered(plugin);
    }
    
    virtual Parameters::Accessor::Ptr CreateModuleParams(const String& subpath) const
    {
      return Parameters::CreateMergedAccessor(CreatePathProperties(Path, subpath), Params);
    }

    virtual void ProcessModule(const String& /*subpath*/, ZXTune::Module::Holder::Ptr holder) const
    {
      Callback(holder);
    }

    virtual Log::ProgressCallback* GetProgressCallback() const
    {
      return ProgressCallback.get();
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const PluginsFilter& Filter;
    const String Path;
    const OnItemCallback& Callback;
    const Log::ProgressCallback::Ptr ProgressCallback;
  };

  class Source : public SourceComponent
  {
  public:
    Source(Parameters::Container::Ptr configParams)
      : Params(configParams)
      , OptionsDescription(Text::INPUT_SECTION)
      , ShowProgress(false)
      , YM(false)
    {
      OptionsDescription.add_options()
        (Text::INPUT_FILE_KEY, boost::program_options::value<StringArray>(&Files), Text::INPUT_FILE_DESC)
        (Text::IO_PROVIDERS_OPTS_KEY, boost::program_options::value<String>(&ProvidersOptions), Text::IO_PROVIDERS_OPTS_DESC)
        (Text::INPUT_ALLOW_PLUGIN_KEY, boost::program_options::value<String>(&Allowed), Text::INPUT_ALLOW_PLUGIN_DESC)
        (Text::INPUT_DENY_PLUGIN_KEY, boost::program_options::value<String>(&Denied), Text::INPUT_DENY_PLUGIN_DESC)
        (Text::INPUT_PROGRESS_KEY, boost::program_options::bool_switch(&ShowProgress), Text::INPUT_PROGRESS_DESC)
        (Text::CORE_OPTS_KEY, boost::program_options::value<String>(&CoreOptions), Text::CORE_OPTS_DESC)
        (Text::YM_KEY, boost::program_options::bool_switch(&YM), Text::YM_DESC)
      ;
    }

    virtual const boost::program_options::options_description& GetOptionsDescription() const
    {
      return OptionsDescription;
    }

    virtual void ParseParameters()
    {
      if (!ProvidersOptions.empty())
      {
        const Parameters::Container::Ptr ioParams = Parameters::Container::Create();
        ThrowIfError(ParseParametersString(Parameters::ZXTune::IO::Providers::PREFIX,
          ProvidersOptions, *ioParams));
        ioParams->Process(*Params);
      }

      if (!CoreOptions.empty() || YM)
      {
        const Parameters::Container::Ptr coreParams = Parameters::Container::Create();
        if (!CoreOptions.empty())
        {
          ThrowIfError(ParseParametersString(Parameters::ZXTune::Core::PREFIX,
            CoreOptions, *coreParams));
        }
        if (YM)
        {
          coreParams->SetIntValue(Parameters::ZXTune::Core::AYM::TYPE, 1);
        }
        coreParams->Process(*Params);
      }
    }

    // throw
    virtual void Initialize()
    {
      if (Files.empty())
      {
        throw Error(THIS_LINE, NO_INPUT_FILES, Text::INPUT_ERROR_NO_FILES);
      }
      Filter.reset(new PluginsFilter(Allowed, Denied));
    }

    virtual void ProcessItems(const OnItemCallback& callback)
    {
      assert(callback);

      for (StringArray::const_iterator it = Files.begin(), lim = Files.end(); it != lim; ++it)
      {
        ZXTune::IO::DataContainer::Ptr data;
        String path, subpath;
        ThrowIfError(ZXTune::IO::SplitUri(*it, path, subpath));
        ThrowIfError(ZXTune::IO::OpenData(path, *Params, 0, data));

        const DetectParametersImpl params(Params, *Filter, path, callback, ShowProgress);
        ThrowIfError(ZXTune::DetectModules(Params, params, data, subpath));
      }
    }
  private:
    const Parameters::Container::Ptr Params;
    boost::program_options::options_description OptionsDescription;
    StringArray Files;
    String ProvidersOptions;
    String Allowed;
    String Denied;
    std::auto_ptr<PluginsFilter> Filter;
    bool ShowProgress;
    String CoreOptions;
    bool YM;
  };
}

std::auto_ptr<SourceComponent> SourceComponent::Create(Parameters::Container::Ptr configParams)
{
  return std::auto_ptr<SourceComponent>(new Source(configParams));
}
