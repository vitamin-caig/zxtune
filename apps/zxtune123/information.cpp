#include "information.h"

#include <tools.h>
#include <formatter.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <io/provider.h>
#include <sound/backend.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>

#include <iostream>

#include "cmdline.h"
#include "messages.h"

namespace
{
  String PluginCaps(uint32_t caps)
  {
    typedef std::pair<uint32_t, String> CapsPair;
    
    static const CapsPair KNOWN_CAPS[] = {
      //device caps
      CapsPair(ZXTune::CAP_DEV_AYM, TEXT_INFO_CAP_AYM),
      CapsPair(ZXTune::CAP_DEV_TS, TEXT_INFO_CAP_TS),
      CapsPair(ZXTune::CAP_DEV_BEEPER, TEXT_INFO_CAP_BEEPER),
      CapsPair(ZXTune::CAP_DEV_FM, TEXT_INFO_CAP_FM),
      CapsPair(ZXTune::CAP_DEV_1DAC, TEXT_INFO_CAP_DAC1),
      CapsPair(ZXTune::CAP_DEV_2DAC, TEXT_INFO_CAP_DAC2),
      CapsPair(ZXTune::CAP_DEV_4DAC, TEXT_INFO_CAP_DAC4),
      //storage caps
      CapsPair(ZXTune::CAP_STOR_MODULE, TEXT_INFO_CAP_MODULE),
      CapsPair(ZXTune::CAP_STOR_CONTAINER, TEXT_INFO_CAP_CONTAINER),
      CapsPair(ZXTune::CAP_STOR_MULTITRACK, TEXT_INFO_CAP_MULTITRACK),
      CapsPair(ZXTune::CAP_STOR_SCANER, TEXT_INFO_CAP_SCANER),
      CapsPair(ZXTune::CAP_STOR_PLAIN, TEXT_INFO_CAP_PLAIN),
      //conversion caps
      CapsPair(ZXTune::CAP_CONV_RAW, TEXT_INFO_CAP_RAW)
    };
    
    String result;
    for (const CapsPair* cap = KNOWN_CAPS; cap != ArrayEnd(KNOWN_CAPS); ++cap)
    {
      if (cap->first & caps)
      {
        result += " ";
        result += cap->second;
      }
    }
    return result.substr(1);
  }
  
  void ShowPlugin(const ZXTune::PluginInformation& info)
  {
    std::cout << (Formatter(TEXT_INFO_PLUGIN_INFO)
       % info.Id % info.Description % info.Version % PluginCaps(info.Capabilities)).str();
  }

  void ShowPlugins()
  {
    ZXTune::PluginInformationArray plugins;
    ZXTune::EnumeratePlugins(plugins);
    std::for_each(plugins.begin(), plugins.end(), ShowPlugin);
  }
  
  void ShowBackend(const ZXTune::Sound::BackendInfo& info)
  {
    std::cout << (Formatter(TEXT_INFO_BACKEND_INFO)
      % info.Id % info.Description % info.Version).str();
  }
  
  void ShowBackends()
  {
    ZXTune::Sound::BackendInfoArray backends;
    ZXTune::Sound::EnumerateBackends(backends);
    std::for_each(backends.begin(), backends.end(), ShowBackend);
  }
  
  void ShowProvider(const ZXTune::IO::ProviderInfo& info)
  {
    std::cout << (Formatter(TEXT_INFO_PROVIDER_INFO)
      % info.Name % info.Description % info.Version).str();
  }
  
  void ShowProviders()
  {
    ZXTune::IO::ProviderInfoArray providers;
    ZXTune::IO::EnumerateProviders(providers);
    std::for_each(providers.begin(), providers.end(), ShowProvider);
  }

  class Information : public InformationComponent
  {
  public:
    Information()
      : OptionsDescription(TEXT_INFORMATIONAL_SECTION)
      , EnumPlugins(), EnumBackends(), EnumProviders()
    {
      OptionsDescription.add_options()
        (TEXT_INFO_PLUGINS_KEY, boost::program_options::bool_switch(&EnumPlugins), TEXT_INFO_PLUGINS_DESC)
        (TEXT_INFO_BACKENDS_KEY, boost::program_options::bool_switch(&EnumBackends), TEXT_INFO_BACKENDS_DESC)
        (TEXT_INFO_PROVIDERS_KEY, boost::program_options::bool_switch(&EnumProviders), TEXT_INFO_PROVIDERS_DESC)
      ;
    }
    
    virtual const boost::program_options::options_description& GetOptionsDescription() const
    {
      return OptionsDescription;
    }
    
    virtual bool Process() const
    {
      if (EnumPlugins)
      {
        ShowPlugins();
      }
      if (EnumBackends)
      {
        ShowBackends();
      }
      if (EnumProviders)
      {
        ShowProviders();
      }
      
      return EnumPlugins || EnumBackends || EnumProviders;
    }
  private:
    boost::program_options::options_description OptionsDescription;
    bool EnumPlugins;
    bool EnumBackends;
    bool EnumProviders;
  };
}

std::auto_ptr<InformationComponent> InformationComponent::Create()
{
  return std::auto_ptr<InformationComponent>(new Information);
}
