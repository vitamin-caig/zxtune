#include "error_codes.h"
#include "source.h"

#include <error_tools.h>
#include <string_helpers.h>
#include <tools.h>
#include <io/provider.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>

#include <boost/bind.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>

#include "text.h"

#define FILE_TAG 9EDFE3AF

namespace
{
  const Char DELIMITERS[] = {',', ';', ':', '\0'};
  
  void Parse(const StringSet& allplugs, const String& str, StringSet& plugs, uint32_t& caps)
  {
    typedef std::pair<uint32_t, String> CapsPair;
    static const CapsPair CAPABILITIES[] = 
    {
      CapsPair(ZXTune::CAP_DEV_AYM, TEXT_INFO_CAP_AYM),
      CapsPair(ZXTune::CAP_DEV_TS, TEXT_INFO_CAP_TS),
      CapsPair(ZXTune::CAP_DEV_BEEPER, TEXT_INFO_CAP_BEEPER),
      CapsPair(ZXTune::CAP_DEV_FM, TEXT_INFO_CAP_FM),
      CapsPair(ZXTune::CAP_DEV_1DAC | ZXTune::CAP_DEV_2DAC | ZXTune::CAP_DEV_4DAC, TEXT_INFO_CAP_DAC),

      //remove module capability- it cannot be enabled or disabled
      CapsPair(ZXTune::CAP_STOR_CONTAINER, TEXT_INFO_CAP_CONTAINER),
      CapsPair(ZXTune::CAP_STOR_MULTITRACK, TEXT_INFO_CAP_MULTITRACK),
      CapsPair(ZXTune::CAP_STOR_SCANER, TEXT_INFO_CAP_SCANER),
      CapsPair(ZXTune::CAP_STOR_PLAIN, TEXT_INFO_CAP_PLAIN),

      CapsPair(ZXTune::CAP_CONV_RAW, CONVERSION_MODE_RAW),
      CapsPair(ZXTune::CAP_CONV_PSG, CONVERSION_MODE_PSG)
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
          throw MakeFormattedError(THIS_LINE, INVALID_PARAMETER, TEXT_ERROR_INVALID_PARAMETER, *it, str);
        }
      }
    }
    plugs.swap(tmpPlugs);
    caps = tmpCaps;
  }
  
  Error FormModule(const String& path, const String& subpath, const ZXTune::Module::Holder::Ptr& module,
    const OnItemCallback& callback)
  {
    String uri;
    if (const Error& e = ZXTune::IO::CombineUri(path, subpath, uri))
    {
      return e;
    }
    ModuleItem item;
    item.Id = uri;
    item.Module = module;
    callback(item);
    return Error();
  }
  
  class Source : public SourceComponent
  {
  public:
    explicit Source(Parameters::Map& globalParams)
      : GlobalParams(globalParams)
      , OptionsDescription(TEXT_INPUT_SECTION)
      , EnabledCaps(0), DisabledCaps(0)
    {
      OptionsDescription.add_options()
        (TEXT_INPUT_FILE_KEY, boost::program_options::value<StringArray>(&Files), TEXT_INPUT_FILE_DESC)
        (TEXT_INPUT_ALLOW_PLUGIN_KEY, boost::program_options::value<String>(&Allowed), TEXT_INPUT_ALLOW_PLUGIN_DESC)
        (TEXT_INPUT_DENY_PLUGIN_KEY, boost::program_options::value<String>(&Denied), TEXT_INPUT_DENY_PLUGIN_DESC)
      ;
    }

    virtual const boost::program_options::options_description& GetOptionsDescription() const
    {
      return OptionsDescription;
    }
    
    // throw
    virtual void Initialize()
    {
      StringSet allplugs;
      {
        ZXTune::PluginInformationArray plugs;
        ZXTune::EnumeratePlugins(plugs);
        std::transform(plugs.begin(), plugs.end(), std::inserter(allplugs, allplugs.end()),\
          boost::mem_fn(&ZXTune::PluginInformation::Id));
      }
      Parse(allplugs, Allowed, EnabledPlugins, EnabledCaps);
      Parse(allplugs, Denied, DisabledPlugins, DisabledCaps);
      if (Files.empty())
      {
        throw Error(THIS_LINE, NO_INPUT_FILES, TEXT_INPUT_ERROR_NO_FILES);
      }
    }
    
    virtual void ProcessItems(const OnItemCallback& callback)
    {
      assert(callback);
      const bool hasFilter(!EnabledPlugins.empty() || !DisabledPlugins.empty() || 0 != EnabledCaps || 0 != DisabledCaps);
      ThrowIfError(ProcessModuleItems(Files, GlobalParams, 
        hasFilter ? boost::bind(&Source::DoFilter, this, _1) : ZXTune::DetectParameters::FilterFunc(), callback));
    }

    bool DoFilter(const ZXTune::PluginInformation& info)
    {
      if (EnabledPlugins.count(info.Id) || 0 != (info.Capabilities & EnabledCaps))
      {
        //enabled explicitly
        return false;
      }
      else if (DisabledPlugins.count(info.Id) || 0 != (info.Capabilities & DisabledCaps))
      {
        //disabled explicitly
        return true;
      }
      //enable all if there're no explicit enables
      return !(EnabledPlugins.empty() && !EnabledCaps);
    }
  private:
    Parameters::Map& GlobalParams;
    boost::program_options::options_description OptionsDescription;
    StringArray Files;
    String Allowed;
    String Denied;
    StringSet EnabledPlugins;
    StringSet DisabledPlugins;
    uint32_t EnabledCaps;
    uint32_t DisabledCaps;
  };
}

Error ProcessModuleItems(const StringArray& files, const Parameters::Map& params, const ZXTune::DetectParameters::FilterFunc& filter,
                    const boost::function<void(const ModuleItem&)>& callback)
{
  //TODO: optimize
  for (StringArray::const_iterator it = files.begin(), lim = files.end(); it != lim; ++it)
  {
    ZXTune::IO::DataContainer::Ptr data;
    String subpath;
    if (const Error& e = ZXTune::IO::OpenData(*it, params, 0, data, subpath))
    {
      return e;
    }
    ZXTune::DetectParameters detectParams;
    detectParams.Filter = filter;
    detectParams.Callback = boost::bind(&FormModule, *it, _1, _2, callback);
    if (const Error& e = ZXTune::DetectModules(params, detectParams, data, subpath))
    {
      return e;
    }
  }
  return Error();
}

std::auto_ptr<SourceComponent> SourceComponent::Create(Parameters::Map& globalParams)
{
  return std::auto_ptr<SourceComponent>(new Source(globalParams));
}
