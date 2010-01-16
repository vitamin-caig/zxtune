/*
Abstract:
  Informational component implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  
  This file is a part of zxtune123 application based on zxtune library
*/

#include "information.h"

#include <tools.h>
#include <formatter.h>
#include <core/core_parameters.h>
#include <core/freq_tables.h>
#include <core/module_attrs.h>
#include <core/plugins_parameters.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <io/io_parameters.h>
#include <io/provider.h>
#include <io/providers_parameters.h>
#include <sound/backend.h>
#include <sound/backends_parameters.h>
#include <sound/sound_parameters.h>

#include <boost/tuple/tuple.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>

#include <iostream>

#include "text.h"

namespace
{
  String PluginCaps(uint32_t caps)
  {
    typedef std::pair<uint32_t, String> CapsPair;
    
    static const CapsPair KNOWN_CAPS[] = {
      //device caps
      CapsPair(ZXTune::CAP_DEVICE_MASK, TEXT_INFO_CAP_DEVICES),
      CapsPair(ZXTune::CAP_DEV_AYM, TEXT_INFO_CAP_AYM),
      CapsPair(ZXTune::CAP_DEV_TS, TEXT_INFO_CAP_TS),
      CapsPair(ZXTune::CAP_DEV_BEEPER, TEXT_INFO_CAP_BEEPER),
      CapsPair(ZXTune::CAP_DEV_FM, TEXT_INFO_CAP_FM),
      CapsPair(ZXTune::CAP_DEV_1DAC, TEXT_INFO_CAP_DAC1),
      CapsPair(ZXTune::CAP_DEV_2DAC, TEXT_INFO_CAP_DAC2),
      CapsPair(ZXTune::CAP_DEV_4DAC, TEXT_INFO_CAP_DAC4),
      //storage caps
      CapsPair(ZXTune::CAP_STORAGE_MASK, TEXT_INFO_CAP_STORAGES),
      CapsPair(ZXTune::CAP_STOR_MODULE, TEXT_INFO_CAP_MODULE),
      CapsPair(ZXTune::CAP_STOR_CONTAINER, TEXT_INFO_CAP_CONTAINER),
      CapsPair(ZXTune::CAP_STOR_MULTITRACK, TEXT_INFO_CAP_MULTITRACK),
      CapsPair(ZXTune::CAP_STOR_SCANER, TEXT_INFO_CAP_SCANER),
      CapsPair(ZXTune::CAP_STOR_PLAIN, TEXT_INFO_CAP_PLAIN),
      //conversion caps
      CapsPair(ZXTune::CAP_CONVERSION_MASK, TEXT_INFO_CAP_CONVERSION),
      CapsPair(ZXTune::CAP_CONV_RAW, CONVERSION_MODE_RAW),
      CapsPair(ZXTune::CAP_CONV_PSG, CONVERSION_MODE_PSG)
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
    std::cout << TEXT_INFO_LIST_PLUGINS_TITLE << std::endl;
    std::for_each(plugins.begin(), plugins.end(), ShowPlugin);
  }
  
  void ShowBackend(const ZXTune::Sound::BackendInformation& info)
  {
    std::cout << (Formatter(TEXT_INFO_BACKEND_INFO)
      % info.Id % info.Description % info.Version).str();
  }
  
  void ShowBackends()
  {
    ZXTune::Sound::BackendInformationArray backends;
    ZXTune::Sound::EnumerateBackends(backends);
    std::cout << TEXT_INFO_LIST_BACKENDS_TITLE << std::endl;
    std::for_each(backends.begin(), backends.end(), ShowBackend);
  }
  
  void ShowProvider(const ZXTune::IO::ProviderInformation& info)
  {
    std::cout << (Formatter(TEXT_INFO_PROVIDER_INFO)
      % info.Name % info.Description % info.Version).str();
  }
  
  void ShowProviders()
  {
    ZXTune::IO::ProviderInformationArray providers;
    ZXTune::IO::EnumerateProviders(providers);
    std::cout << TEXT_INFO_LIST_PROVIDERS_TITLE << std::endl;
    std::for_each(providers.begin(), providers.end(), ShowProvider);
  }
  
  typedef boost::tuple<String, String, Parameters::ValueType> OptionDesc;
  
  void ShowOption(const OptionDesc& opt)
  {
    //section
    if (opt.get<1>().empty())
    {
      std::cout << opt.get<0>() << std::endl;
    }
    else
    {
      const String& defVal(Parameters::ConvertToString(opt.get<2>()));
      if (defVal.empty())
      {
        std::cout << (Formatter(TEXT_INFO_OPTION_INFO) % opt.get<0>() % opt.get<1>()).str();
      }
      else
      {
        std::cout << (Formatter(TEXT_INFO_OPTION_INFO_DEFAULTS)
          % opt.get<0>() % opt.get<1>() % defVal).str();
      }
    }
  }
  
  void ShowOptions()
  {
    static const String EMPTY;
    
    static const OptionDesc OPTIONS[] = 
    {
      OptionDesc(TEXT_INFO_OPTIONS_IO_PROVIDERS_TITLE, EMPTY, 0),
      OptionDesc(Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD, 
                 TEXT_INFO_OPTIONS_IO_PROVIDERS_FILE_MMAP_THRESHOLD,
                 Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD_DEFAULT),
      OptionDesc(TEXT_INFO_OPTIONS_SOUND_TITLE, EMPTY, 0),
      OptionDesc(Parameters::ZXTune::Sound::FREQUENCY,
                 TEXT_INFO_OPTIONS_SOUND_FREQUENCY,
                 Parameters::ZXTune::Sound::FREQUENCY_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::CLOCKRATE,
                 TEXT_INFO_OPTIONS_SOUND_CLOCKRATE,
                 Parameters::ZXTune::Sound::CLOCKRATE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::FRAMEDURATION,
                 TEXT_INFO_OPTIONS_SOUND_FRAMEDURATION,
                 Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::LOOPMODE,
                 TEXT_INFO_OPTIONS_SOUND_LOOPMODE,
                 Parameters::ZXTune::Sound::LOOPMODE_DEFAULT),
      OptionDesc(TEXT_INFO_OPTIONS_SOUND_BACKENDS_TITLE, EMPTY, 0),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Wav::FILENAME,
                 TEXT_INFO_OPTIONS_SOUND_BACKENDS_WAV_FILENAME,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Win32::DEVICE,
                 TEXT_INFO_OPTIONS_SOUND_BACKENDS_WIN32_DEVICE,
                 Parameters::ZXTune::Sound::Backends::Win32::DEVICE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::Backends::OSS::DEVICE,
                 TEXT_INFO_OPTIONS_SOUND_BACKENDS_OSS_DEVICE,
                 Parameters::ZXTune::Sound::Backends::OSS::DEVICE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::Backends::OSS::MIXER,
                 TEXT_INFO_OPTIONS_SOUND_BACKENDS_OSS_MIXER,
                 Parameters::ZXTune::Sound::Backends::OSS::MIXER_DEFAULT),
      OptionDesc(TEXT_INFO_OPTIONS_CORE_TITLE, EMPTY, 0),
      OptionDesc(Parameters::ZXTune::Core::AYM::TYPE,
                 TEXT_INFO_OPTIONS_CORE_AYM_TYPE,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Core::AYM::TABLE,
                 TEXT_INFO_OPTIONS_CORE_AYM_TABLE,
                 EMPTY),
      OptionDesc(TEXT_INFO_OPTIONS_CORE_PLUGINS_TITLE, EMPTY,0),
      OptionDesc(Parameters::ZXTune::Core::Plugins::Raw::SCAN_STEP,
                 TEXT_INFO_OPTIONS_CORE_PLUGINS_RAW_SCAN_STEP,
                 Parameters::ZXTune::Core::Plugins::Raw::SCAN_STEP_DEFAULT)
    };
    std::cout << TEXT_INFO_LIST_OPTIONS_TITLE << std::endl;
    std::for_each(OPTIONS, ArrayEnd(OPTIONS), ShowOption);
  }
  
  typedef std::pair<String, String> AttrType;
  void ShowAttribute(const AttrType& arg)
  {
    std::cout << (Formatter(TEXT_INFO_ATTRIBUTE_INFO) % arg.first % arg.second).str();
  }
  
  void ShowAttributes()
  {
    static const AttrType ATTRIBUTES[] =
    {
      AttrType(ZXTune::Module::ATTR_TYPE, TEXT_INFO_ATTRIBUTES_TYPE),
      AttrType(ZXTune::Module::ATTR_CONTAINER, TEXT_INFO_ATTRIBUTES_CONTAINER),
      AttrType(ZXTune::Module::ATTR_SUBPATH, TEXT_INFO_ATTRIBUTES_SUBPATH),
      AttrType(ZXTune::Module::ATTR_AUTHOR, TEXT_INFO_ATTRIBUTES_AUTHOR),
      AttrType(ZXTune::Module::ATTR_TITLE, TEXT_INFO_ATTRIBUTES_TITLE),
      AttrType(ZXTune::Module::ATTR_PROGRAM, TEXT_INFO_ATTRIBUTES_PROGRAM),
      AttrType(ZXTune::Module::ATTR_COMPUTER, TEXT_INFO_ATTRIBUTES_COMPUTER),
      AttrType(ZXTune::Module::ATTR_DATE, TEXT_INFO_ATTRIBUTES_DATE),
      AttrType(ZXTune::Module::ATTR_COMMENT, TEXT_INFO_ATTRIBUTES_COMMENT),
      AttrType(ZXTune::Module::ATTR_WARNINGS, TEXT_INFO_ATTRIBUTES_WARNINGS),
      AttrType(ZXTune::Module::ATTR_WARNINGS_COUNT, TEXT_INFO_ATTRIBUTES_WARNINGS_COUNT),
      AttrType(ZXTune::Module::ATTR_CRC, TEXT_INFO_ATTRIBUTES_CRC),
      AttrType(ZXTune::Module::ATTR_SIZE, TEXT_INFO_ATTRIBUTES_SIZE)
    };
    std::cout << TEXT_INFO_LIST_ATTRIBUTES_TITLE << std::endl;
    std::for_each(ATTRIBUTES, ArrayEnd(ATTRIBUTES), ShowAttribute);
  }
  
  void ShowFreqtables()
  {
    static const String FREQTABLES[] =
    {
      ZXTune::Module::TABLE_SOUNDTRACKER,
      ZXTune::Module::TABLE_ASM
    };
    std::cout << TEXT_INFO_LIST_FREQTABLES_TITLE;
    std::copy(FREQTABLES, ArrayEnd(FREQTABLES), std::ostream_iterator<String>(std::cout, " "));
    std::cout << std::endl;
  }

  class Information : public InformationComponent
  {
  public:
    Information()
      : OptionsDescription(TEXT_INFORMATIONAL_SECTION)
      , EnumPlugins(), EnumBackends(), EnumProviders(), EnumOptions(), EnumAttributes(), EnumFreqtables()
    {
      OptionsDescription.add_options()
        (TEXT_INFO_LIST_PLUGINS_KEY, boost::program_options::bool_switch(&EnumPlugins), TEXT_INFO_LIST_PLUGINS_DESC)
        (TEXT_INFO_LIST_BACKENDS_KEY, boost::program_options::bool_switch(&EnumBackends), TEXT_INFO_LIST_BACKENDS_DESC)
        (TEXT_INFO_LIST_PROVIDERS_KEY, boost::program_options::bool_switch(&EnumProviders), TEXT_INFO_LIST_PROVIDERS_DESC)
        (TEXT_INFO_LIST_OPTIONS_KEY, boost::program_options::bool_switch(&EnumOptions), TEXT_INFO_LIST_OPTIONS_DESC)
        (TEXT_INFO_LIST_ATTRIBUTES_KEY, boost::program_options::bool_switch(&EnumAttributes), TEXT_INFO_LIST_ATTRIBUTES_DESC)
        (TEXT_INFO_LIST_FREQTABLES_KEY, boost::program_options::bool_switch(&EnumFreqtables), TEXT_INFO_LIST_FREQTABLES_DESC)
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
      if (EnumOptions)
      {
        ShowOptions();
      }
      if (EnumAttributes)
      {
        ShowAttributes();
      }
      if (EnumFreqtables)
      {
        ShowFreqtables();
      }
      return EnumPlugins || EnumBackends || EnumProviders || EnumOptions || EnumAttributes || EnumFreqtables;
    }
  private:
    boost::program_options::options_description OptionsDescription;
    bool EnumPlugins;
    bool EnumBackends;
    bool EnumProviders;
    bool EnumOptions;
    bool EnumAttributes;
    bool EnumFreqtables;
  };
}

std::auto_ptr<InformationComponent> InformationComponent::Create()
{
  return std::auto_ptr<InformationComponent>(new Information);
}
