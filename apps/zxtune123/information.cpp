/*
Abstract:
  Informational component implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  
  This file is a part of zxtune123 application based on zxtune library
*/

//local includes
#include "information.h"
#include <apps/base/app.h>
//common includes
#include <tools.h>
#include <format.h>
//library includes
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
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/sound_parameters.h>
//std includes
#include <iostream>
//boost includes
#include <boost/tuple/tuple.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>
//text includes
#include "text/text.h"

namespace
{
  typedef std::pair<uint_t, String> CapsPair;
  String SerializeCaps(uint_t caps, const CapsPair* from)
  {
    String result;
    for (const CapsPair* cap = from; cap->first; ++cap)
    {
      if (cap->first & caps)
      {
        result += Char(' ');
        result += cap->second;
      }
    }
    return result.substr(1);
  }

  String PluginCaps(uint_t caps)
  {
    static const CapsPair PLUGINS_CAPS[] =
    {
      //device caps
      CapsPair(ZXTune::CAP_DEV_AYM, Text::INFO_CAP_AYM),
      CapsPair(ZXTune::CAP_DEV_TS, Text::INFO_CAP_TS),
      CapsPair(ZXTune::CAP_DEV_BEEPER, Text::INFO_CAP_BEEPER),
      CapsPair(ZXTune::CAP_DEV_FM, Text::INFO_CAP_FM),
      CapsPair(ZXTune::CAP_DEV_1DAC, Text::INFO_CAP_DAC1),
      CapsPair(ZXTune::CAP_DEV_2DAC, Text::INFO_CAP_DAC2),
      CapsPair(ZXTune::CAP_DEV_4DAC, Text::INFO_CAP_DAC4),
      //storage caps
      CapsPair(ZXTune::CAP_STOR_MODULE, Text::INFO_CAP_MODULE),
      CapsPair(ZXTune::CAP_STOR_CONTAINER, Text::INFO_CAP_CONTAINER),
      CapsPair(ZXTune::CAP_STOR_MULTITRACK, Text::INFO_CAP_MULTITRACK),
      CapsPair(ZXTune::CAP_STOR_SCANER, Text::INFO_CAP_SCANER),
      CapsPair(ZXTune::CAP_STOR_PLAIN, Text::INFO_CAP_PLAIN),
      CapsPair(ZXTune::CAP_STOR_DIRS, Text::INFO_CAP_DIRS),
      //conversion caps
      CapsPair(ZXTune::CAP_CONV_RAW, Text::INFO_CONV_RAW),
      CapsPair(ZXTune::CAP_CONV_PSG, Text::INFO_CONV_PSG),
      CapsPair(ZXTune::CAP_CONV_ZX50, Text::INFO_CONV_ZX50),
      CapsPair(ZXTune::CAP_CONV_TXT, Text::INFO_CONV_TXT),
      CapsPair(ZXTune::CAP_CONV_AYDUMP, Text::INFO_CONV_AYDUMP),
      CapsPair(ZXTune::CAP_CONV_FYM, Text::INFO_CONV_FYM),
      //limiter
      CapsPair()
    };
    return SerializeCaps(caps, PLUGINS_CAPS);
  }

  String BackendCaps(uint_t caps)
  {
    static const CapsPair BACKENDS_CAPS[] =
    {
      // Type-related capabilities
      CapsPair(ZXTune::Sound::CAP_TYPE_STUB, Text::INFO_CAP_STUB),
      CapsPair(ZXTune::Sound::CAP_TYPE_SYSTEM, Text::INFO_CAP_SYSTEM),
      CapsPair(ZXTune::Sound::CAP_TYPE_FILE, Text::INFO_CAP_FILE),
      CapsPair(ZXTune::Sound::CAP_TYPE_HARDWARE, Text::INFO_CAP_HARDWARE),
      // Features-related capabilities
      CapsPair(ZXTune::Sound::CAP_FEAT_HWVOLUME, Text::INFO_CAP_HWVOLUME),
      //limiter
      CapsPair()
    };
    return SerializeCaps(caps, BACKENDS_CAPS);
  }
  
  inline void ShowPlugin(const ZXTune::Plugin& plugin)
  {
    StdOut << Strings::Format(Text::INFO_PLUGIN_INFO,
      plugin.Id(), plugin.Description(), PluginCaps(plugin.Capabilities()));
  }

  inline void ShowPlugins()
  {
    StdOut << Text::INFO_LIST_PLUGINS_TITLE << std::endl;
    for (ZXTune::Plugin::Iterator::Ptr plugins = ZXTune::EnumeratePlugins(); plugins->IsValid(); plugins->Next())
    {
      ShowPlugin(*plugins->Get());
    }
  }
  
  inline void ShowBackend(const ZXTune::Sound::BackendInformation& info)
  {
    const Error& status = info.Status();
    StdOut << Strings::Format(Text::INFO_BACKEND_INFO,
      info.Id(), info.Description(), BackendCaps(info.Capabilities()), status ? status.GetText() : Text::INFO_STATUS_OK);
  }
  
  inline void ShowBackends()
  {
    using namespace ZXTune::Sound;
    for (BackendCreator::Iterator::Ptr backends = EnumerateBackends();
      backends->IsValid(); backends->Next())
    {
      ShowBackend(*backends->Get());
    }
  }
  
  inline void ShowProvider(const ZXTune::IO::Provider& provider)
  {
    const Error& status = provider.Status();
    StdOut << Strings::Format(Text::INFO_PROVIDER_INFO,
      provider.Id(), provider.Description(), status ? status.GetText() : Text::INFO_STATUS_OK);
  }
  
  inline void ShowProviders()
  {
    using namespace ZXTune::IO;
    StdOut << Text::INFO_LIST_PROVIDERS_TITLE << std::endl;
    for (Provider::Iterator::Ptr providers = EnumerateProviders(); 
      providers->IsValid(); providers->Next())
    {
      ShowProvider(*providers->Get());
    }
  }
  
  typedef boost::variant<Parameters::IntType, Parameters::StringType> ValueType;

  struct OptionDesc
  {
    OptionDesc(const Parameters::NameType& name, const String& descr, const ValueType& def)
      : Name(FromStdString(name.FullPath()))
      , Desc(descr)
      , Default(def)
    {
    }

    OptionDesc(const String& name, const String& descr, const ValueType& def)
      : Name(name)
      , Desc(descr)
      , Default(def)
    {
    }

    String Name;
    String Desc;
    ValueType Default;
  };
  
  void ShowOption(const OptionDesc& opt)
  {
    //section
    if (opt.Desc.empty())
    {
      StdOut << opt.Name << std::endl;
    }
    else
    {
      const Parameters::StringType* defValString = boost::get<const Parameters::StringType>(&opt.Default);
      if (defValString && defValString->empty())
      {
        StdOut << Strings::Format(Text::INFO_OPTION_INFO, opt.Name, opt.Desc);
      }
      else
      {
        StdOut << Strings::Format(Text::INFO_OPTION_INFO_DEFAULTS,
          opt.Name, opt.Desc, opt.Default);
      }
    }
  }
  
  void ShowOptions()
  {
    static const String EMPTY;
    
    static const OptionDesc OPTIONS[] =
    {
      //IO parameters
      OptionDesc(Text::INFO_OPTIONS_IO_PROVIDERS_TITLE, EMPTY, 0),
      OptionDesc(Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD,
                 Text::INFO_OPTIONS_IO_PROVIDERS_FILE_MMAP_THRESHOLD,
                 Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD_DEFAULT),
      OptionDesc(Parameters::ZXTune::IO::Providers::File::CREATE_DIRECTORIES,
                 Text::INFO_OPTIONS_IO_PROVIDERS_FILE_CREATE_DIRECTORIES,
                 Parameters::ZXTune::IO::Providers::File::CREATE_DIRECTORIES_DEFAULT),
      OptionDesc(Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING,
                 Text::INFO_OPTIONS_IO_PROVIDERS_FILE_OVERWRITE_EXISTING,
                 Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING_DEFAULT),
      //Sound parameters
      OptionDesc(Text::INFO_OPTIONS_SOUND_TITLE, EMPTY, 0),
      OptionDesc(Parameters::ZXTune::Sound::FREQUENCY,
                 Text::INFO_OPTIONS_SOUND_FREQUENCY,
                 Parameters::ZXTune::Sound::FREQUENCY_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::FRAMEDURATION,
                 Text::INFO_OPTIONS_SOUND_FRAMEDURATION,
                 Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::LOOPED,
                 Text::INFO_OPTIONS_SOUND_LOOPED,
                 EMPTY),
      //Sound backend parameters
      OptionDesc(Text::INFO_OPTIONS_SOUND_BACKENDS_TITLE, EMPTY, 0),
      OptionDesc(Parameters::ZXTune::Sound::Backends::File::FILENAME,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_FILE_FILENAME,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Sound::Backends::File::BUFFERS,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_FILE_BUFFERS,
                 0),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Win32::DEVICE,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_WIN32_DEVICE,
                 Parameters::ZXTune::Sound::Backends::Win32::DEVICE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Win32::BUFFERS,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_WIN32_BUFFERS,
                 Parameters::ZXTune::Sound::Backends::Win32::BUFFERS_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Oss::DEVICE,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_OSS_DEVICE,
                 Parameters::ZXTune::Sound::Backends::Oss::DEVICE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Oss::MIXER,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_OSS_MIXER,
                 Parameters::ZXTune::Sound::Backends::Oss::MIXER_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Alsa::DEVICE,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_ALSA_DEVICE,
                 Parameters::ZXTune::Sound::Backends::Alsa::DEVICE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Alsa::MIXER,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_ALSA_MIXER,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Alsa::BUFFERS,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_ALSA_BUFFERS,
                 Parameters::ZXTune::Sound::Backends::Alsa::BUFFERS_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Sdl::BUFFERS,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_SDL_BUFFERS,
                 Parameters::ZXTune::Sound::Backends::Sdl::BUFFERS_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::Backends::DirectSound::LATENCY,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_DIRECTSOUND_LATENCY,
                 Parameters::ZXTune::Sound::Backends::DirectSound::LATENCY_DEFAULT),
      //Mp3
      OptionDesc(Parameters::ZXTune::Sound::Backends::Mp3::MODE,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_MP3_MODE,
                 Parameters::ZXTune::Sound::Backends::Mp3::MODE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Mp3::BITRATE,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_MP3_BITRATE,
                 Parameters::ZXTune::Sound::Backends::Mp3::BITRATE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Mp3::QUALITY,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_MP3_QUALITY,
                 Parameters::ZXTune::Sound::Backends::Mp3::QUALITY_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_MP3_CHANNELS,
                 Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS_DEFAULT),
      //Ogg
      OptionDesc(Parameters::ZXTune::Sound::Backends::Ogg::MODE,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_OGG_MODE,
                 Parameters::ZXTune::Sound::Backends::Ogg::MODE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Ogg::QUALITY,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_OGG_QUALITY,
                 Parameters::ZXTune::Sound::Backends::Ogg::QUALITY_DEFAULT),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Ogg::BITRATE,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_MP3_BITRATE,
                 Parameters::ZXTune::Sound::Backends::Ogg::BITRATE_DEFAULT),
      //flac
      OptionDesc(Parameters::ZXTune::Sound::Backends::Flac::COMPRESSION,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_FLAC_COMPRESSION,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Sound::Backends::Flac::BLOCKSIZE,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_FLAC_BLOCKSIZE,
                 EMPTY),
      //Core options
      OptionDesc(Text::INFO_OPTIONS_CORE_TITLE, EMPTY, 0),
      OptionDesc(Parameters::ZXTune::Core::AYM::CLOCKRATE,
                 Text::INFO_OPTIONS_CORE_AYM_CLOCKRATE,
                 Parameters::ZXTune::Core::AYM::CLOCKRATE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Core::AYM::TYPE,
                 Text::INFO_OPTIONS_CORE_AYM_TYPE,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Core::AYM::INTERPOLATION,
                 Text::INFO_OPTIONS_CORE_AYM_INTERPOLATION,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Core::AYM::TABLE,
                 Text::INFO_OPTIONS_CORE_AYM_TABLE,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Core::AYM::DUTY_CYCLE,
                 Text::INFO_OPTIONS_CORE_AYM_DUTY_CYCLE,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Core::AYM::DUTY_CYCLE_MASK,
                 Text::INFO_OPTIONS_CORE_AYM_DUTY_CYCLE_MASK,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Core::AYM::LAYOUT,
                 Text::INFO_OPTIONS_CORE_AYM_LAYOUT,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Core::DAC::INTERPOLATION,
                 Text::INFO_OPTIONS_CORE_DAC_INTERPOLATION,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Core::Z80::INT_TICKS,
                 Text::INFO_OPTIONS_CORE_Z80_INT_TICKS,
                 Parameters::ZXTune::Core::Z80::INT_TICKS_DEFAULT),
      OptionDesc(Parameters::ZXTune::Core::Z80::CLOCKRATE,
                 Text::INFO_OPTIONS_CORE_Z80_CLOCKRATE,
                 Parameters::ZXTune::Core::Z80::CLOCKRATE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Core::FM::CLOCKRATE,
                 Text::INFO_OPTIONS_CORE_FM_CLOCKRATE,
                 Parameters::ZXTune::Core::AYM::CLOCKRATE_DEFAULT),
      //Core plugins options
      OptionDesc(Text::INFO_OPTIONS_CORE_PLUGINS_TITLE, EMPTY,0),
      OptionDesc(Parameters::ZXTune::Core::Plugins::Raw::PLAIN_DOUBLE_ANALYSIS,
                 Text::INFO_OPTIONS_CORE_PLUGINS_RAW_PLAIN_DOUBLE_ANALYSIS,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Core::Plugins::Raw::MIN_SIZE,
                 Text::INFO_OPTIONS_CORE_PLUGINS_RAW_MIN_SIZE,
                 Parameters::ZXTune::Core::Plugins::Raw::MIN_SIZE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Core::Plugins::Hrip::IGNORE_CORRUPTED,
                 Text::INFO_OPTIONS_CORE_PLUGINS_HRIP_IGNORE_CORRUPTED,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Core::Plugins::AY::DEFAULT_DURATION_FRAMES,
                 Text::INFO_OPTIONS_CORE_PLUGINS_AY_DEFAULT_DURATION_FRAMES,
                 Parameters::ZXTune::Core::Plugins::AY::DEFAULT_DURATION_FRAMES_DEFAULT),
      OptionDesc(Parameters::ZXTune::Core::Plugins::Zip::MAX_DEPACKED_FILE_SIZE_MB,
                 Text::INFO_OPTIONS_CORE_PLUGINS_ZIP_MAX_DEPACKED_FILE_SIZE_MB,
                 Parameters::ZXTune::Core::Plugins::Zip::MAX_DEPACKED_FILE_SIZE_MB_DEFAULT),
    };
    StdOut << Text::INFO_LIST_OPTIONS_TITLE << std::endl;
    std::for_each(OPTIONS, ArrayEnd(OPTIONS), ShowOption);
  }
  
  typedef std::pair<String, String> AttrType;
  void ShowAttribute(const AttrType& arg)
  {
    StdOut << Strings::Format(Text::INFO_ATTRIBUTE_INFO, arg.first, arg.second);
  }
  
  void ShowAttributes()
  {
    static const AttrType ATTRIBUTES[] =
    {
      //external
      AttrType(ZXTune::Module::ATTR_EXTENSION, Text::INFO_ATTRIBUTES_EXTENSION),
      AttrType(ZXTune::Module::ATTR_FILENAME, Text::INFO_ATTRIBUTES_FILENAME),
      AttrType(ZXTune::Module::ATTR_PATH, Text::INFO_ATTRIBUTES_PATH),
      AttrType(ZXTune::Module::ATTR_FULLPATH, Text::INFO_ATTRIBUTES_FULLPATH),
      //internal
      AttrType(ZXTune::Module::ATTR_TYPE, Text::INFO_ATTRIBUTES_TYPE),
      AttrType(ZXTune::Module::ATTR_CONTAINER, Text::INFO_ATTRIBUTES_CONTAINER),
      AttrType(ZXTune::Module::ATTR_SUBPATH, Text::INFO_ATTRIBUTES_SUBPATH),
      AttrType(ZXTune::Module::ATTR_AUTHOR, Text::INFO_ATTRIBUTES_AUTHOR),
      AttrType(ZXTune::Module::ATTR_TITLE, Text::INFO_ATTRIBUTES_TITLE),
      AttrType(ZXTune::Module::ATTR_PROGRAM, Text::INFO_ATTRIBUTES_PROGRAM),
      AttrType(ZXTune::Module::ATTR_COMPUTER, Text::INFO_ATTRIBUTES_COMPUTER),
      AttrType(ZXTune::Module::ATTR_DATE, Text::INFO_ATTRIBUTES_DATE),
      AttrType(ZXTune::Module::ATTR_COMMENT, Text::INFO_ATTRIBUTES_COMMENT),
      AttrType(ZXTune::Module::ATTR_WARNINGS, Text::INFO_ATTRIBUTES_WARNINGS),
      AttrType(ZXTune::Module::ATTR_WARNINGS_COUNT, Text::INFO_ATTRIBUTES_WARNINGS_COUNT),
      AttrType(ZXTune::Module::ATTR_CRC, Text::INFO_ATTRIBUTES_CRC),
      AttrType(ZXTune::Module::ATTR_SIZE, Text::INFO_ATTRIBUTES_SIZE),
      //runtime
      AttrType(ZXTune::Module::ATTR_CURRENT_POSITION, Text::INFO_ATTRIBUTES_CURRENT_POSITION),
      AttrType(ZXTune::Module::ATTR_CURRENT_PATTERN, Text::INFO_ATTRIBUTES_CURRENT_PATTERN),
      AttrType(ZXTune::Module::ATTR_CURRENT_LINE, Text::INFO_ATTRIBUTES_CURRENT_LINE)
    };
    StdOut << Text::INFO_LIST_ATTRIBUTES_TITLE << std::endl;
    std::for_each(ATTRIBUTES, ArrayEnd(ATTRIBUTES), ShowAttribute);
  }
  
  void ShowFreqtables()
  {
    static const String FREQTABLES[] =
    {
      ZXTune::Module::TABLE_SOUNDTRACKER,
      ZXTune::Module::TABLE_PROTRACKER2,
      ZXTune::Module::TABLE_PROTRACKER3_3,
      ZXTune::Module::TABLE_PROTRACKER3_4,
      ZXTune::Module::TABLE_PROTRACKER3_3_ASM,
      ZXTune::Module::TABLE_PROTRACKER3_4_ASM,
      ZXTune::Module::TABLE_PROTRACKER3_3_REAL,
      ZXTune::Module::TABLE_PROTRACKER3_4_REAL,
      ZXTune::Module::TABLE_ASM,
      ZXTune::Module::TABLE_SOUNDTRACKER_PRO,
      ZXTune::Module::TABLE_NATURAL_SCALED
    };
    StdOut << Text::INFO_LIST_FREQTABLES_TITLE;
    std::copy(FREQTABLES, ArrayEnd(FREQTABLES), std::ostream_iterator<String>(StdOut, " "));
    StdOut << std::endl;
  }

  class Information : public InformationComponent
  {
  public:
    Information()
      : OptionsDescription(Text::INFORMATIONAL_SECTION)
      , EnumPlugins(), EnumBackends(), EnumProviders(), EnumOptions(), EnumAttributes(), EnumFreqtables()
    {
      OptionsDescription.add_options()
        (Text::INFO_LIST_PLUGINS_KEY, boost::program_options::bool_switch(&EnumPlugins), Text::INFO_LIST_PLUGINS_DESC)
        (Text::INFO_LIST_BACKENDS_KEY, boost::program_options::bool_switch(&EnumBackends), Text::INFO_LIST_BACKENDS_DESC)
        (Text::INFO_LIST_PROVIDERS_KEY, boost::program_options::bool_switch(&EnumProviders), Text::INFO_LIST_PROVIDERS_DESC)
        (Text::INFO_LIST_OPTIONS_KEY, boost::program_options::bool_switch(&EnumOptions), Text::INFO_LIST_OPTIONS_DESC)
        (Text::INFO_LIST_ATTRIBUTES_KEY, boost::program_options::bool_switch(&EnumAttributes), Text::INFO_LIST_ATTRIBUTES_DESC)
        (Text::INFO_LIST_FREQTABLES_KEY, boost::program_options::bool_switch(&EnumFreqtables), Text::INFO_LIST_FREQTABLES_DESC)
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
