/**
* 
* @file
*
* @brief Information component implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "information.h"
#include "sound.h"
//library includes
#include <core/core_parameters.h>
#include <core/freq_tables.h>
#include <core/plugins_parameters.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <io/io_parameters.h>
#include <io/provider.h>
#include <io/providers_parameters.h>
#include <module/attributes.h>
#include <platform/application.h>
#include <sound/backend.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/mixer_parameters.h>
#include <sound/service.h>
#include <sound/sound_parameters.h>
#include <strings/format.h>
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
  String SerializeBitmask(uint_t caps, const CapsPair* from)
  {
    String result;
    for (const CapsPair* cap = from; cap->first; ++cap)
    {
      if (cap->first & caps)
      {
        result += Char(' ');
        result += cap->second;
        caps ^= cap->first;
      }
    }
    if (caps)
    {
      result += Strings::Format(" 0x%08x", caps);
    }
    return result;
  }
  
  String SerializeEnum(uint_t caps, const CapsPair* from)
  {
    for (const CapsPair* cap = from; !cap->second.empty(); ++cap)
    {
      if (cap->first == caps)
      {
        return Char(' ') + cap->second;
      }
    }
    return FromStdString(" unknown");
  }

  String PluginCaps(uint_t caps)
  {
    using namespace ZXTune;
    static const CapsPair PLUGIN_TYPES[] =
    {
      CapsPair(Capabilities::Category::MODULE, Text::INFO_CAT_MODULE),
      CapsPair(Capabilities::Category::CONTAINER, Text::INFO_CAT_CONTAINER),
      CapsPair()
    };

    static const CapsPair MODULE_TYPES[] =
    {
      CapsPair(Capabilities::Module::Type::TRACK, Text::INFO_MOD_TRACK),
      CapsPair(Capabilities::Module::Type::STREAM, Text::INFO_MOD_STREAM),
      CapsPair(Capabilities::Module::Type::MEMORYDUMP, Text::INFO_MOD_MEMORYDUMP),
      CapsPair(Capabilities::Module::Type::MULTI, Text::INFO_MOD_MULTI),
      CapsPair()
    };
    
    static const CapsPair MODULE_CAPS[] =
    {
      //device caps
      CapsPair(Capabilities::Module::Device::AY38910, Text::INFO_DEV_AY38910),
      CapsPair(Capabilities::Module::Device::TURBOSOUND, Text::INFO_DEV_TURBOSOUND),
      CapsPair(Capabilities::Module::Device::BEEPER, Text::INFO_DEV_BEEPER),
      CapsPair(Capabilities::Module::Device::YM2203, Text::INFO_DEV_YM2203),
      CapsPair(Capabilities::Module::Device::TURBOFM, Text::INFO_DEV_TURBOFM),
      CapsPair(Capabilities::Module::Device::DAC, Text::INFO_DEV_DAC),
      CapsPair(Capabilities::Module::Device::SAA1099, Text::INFO_DEV_SAA1099),
      CapsPair(Capabilities::Module::Device::MOS6581, Text::INFO_DEV_MOS6581),
      CapsPair(Capabilities::Module::Device::SPC700, Text::INFO_DEV_SPC700),
      CapsPair(Capabilities::Module::Device::MULTI, Text::INFO_DEV_MULTI),
      CapsPair(Capabilities::Module::Device::RP2A0X, Text::INFO_DEV_RP2A0X),
      CapsPair(Capabilities::Module::Device::LR35902, Text::INFO_DEV_LR35902),
      CapsPair(Capabilities::Module::Device::CO12294, Text::INFO_DEV_CO12294),
      CapsPair(Capabilities::Module::Device::HUC6270, Text::INFO_DEV_HUC6270),
      //conversion caps
      CapsPair(Capabilities::Module::Conversion::PSG, Text::INFO_CONV_PSG),
      CapsPair(Capabilities::Module::Conversion::ZX50, Text::INFO_CONV_ZX50),
      CapsPair(Capabilities::Module::Conversion::TXT, Text::INFO_CONV_TXT),
      CapsPair(Capabilities::Module::Conversion::AYDUMP, Text::INFO_CONV_AYDUMP),
      CapsPair(Capabilities::Module::Conversion::FYM, Text::INFO_CONV_FYM),
      //traits caps
      CapsPair(Capabilities::Module::Traits::MULTIFILE, Text::INFO_CAP_MULTIFILE),
      //limiter
      CapsPair()
    };
    
    static const CapsPair CONTAINER_TYPES[] =
    {
      CapsPair(Capabilities::Container::Type::ARCHIVE, Text::INFO_CONT_ARCHIVE),
      CapsPair(Capabilities::Container::Type::COMPRESSOR, Text::INFO_CONT_COMPRESSOR),
      CapsPair(Capabilities::Container::Type::SNAPSHOT, Text::INFO_CONT_SNAPSHOT),
      CapsPair(Capabilities::Container::Type::DISKIMAGE, Text::INFO_CONT_DISKIMAGE),
      CapsPair(Capabilities::Container::Type::DECOMPILER, Text::INFO_CONT_DECOMPILER),
      CapsPair(Capabilities::Container::Type::MULTITRACK, Text::INFO_CONT_MULTITRACK),
      CapsPair(Capabilities::Container::Type::SCANER, Text::INFO_CONT_SCANER),
      //limiter
      CapsPair()
    };

    static const CapsPair CONTAINER_CAPS[] =
    {
      CapsPair(Capabilities::Container::Traits::DIRECTORIES, Text::INFO_CAP_DIRS),
      CapsPair(Capabilities::Container::Traits::PLAIN, Text::INFO_CAP_PLAIN),
      CapsPair(Capabilities::Container::Traits::ONCEAPPLIED, Text::INFO_CAP_ONCEAPPLIED),
      //limiter
      CapsPair()
    };
    String result = SerializeEnum(caps & Capabilities::Category::MASK, PLUGIN_TYPES);
    switch (caps & Capabilities::Category::MASK)
    {
    case Capabilities::Category::MODULE:
      result += SerializeEnum(caps & Capabilities::Module::Type::MASK, MODULE_TYPES);
      result += SerializeBitmask(caps & ~(Capabilities::Category::MASK | Capabilities::Module::Type::MASK), MODULE_CAPS);
      break;
    case Capabilities::Category::CONTAINER:
      result += SerializeEnum(caps & Capabilities::Container::Type::MASK, CONTAINER_TYPES);
      result += SerializeBitmask(caps & ~(Capabilities::Category::MASK | Capabilities::Container::Type::MASK), CONTAINER_CAPS);
      break;
    }
    return result.substr(1);
  }

  String BackendCaps(uint_t caps)
  {
    static const CapsPair BACKENDS_CAPS[] =
    {
      // Type-related capabilities
      CapsPair(Sound::CAP_TYPE_STUB, Text::INFO_CAP_STUB),
      CapsPair(Sound::CAP_TYPE_SYSTEM, Text::INFO_CAP_SYSTEM),
      CapsPair(Sound::CAP_TYPE_FILE, Text::INFO_CAP_FILE),
      CapsPair(Sound::CAP_TYPE_HARDWARE, Text::INFO_CAP_HARDWARE),
      // Features-related capabilities
      CapsPair(Sound::CAP_FEAT_HWVOLUME, Text::INFO_CAP_HWVOLUME),
      //limiter
      CapsPair()
    };
    return SerializeBitmask(caps, BACKENDS_CAPS);
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
  
  inline void ShowBackend(const Sound::BackendInformation& info)
  {
    const Error& status = info.Status();
    StdOut << Strings::Format(Text::INFO_BACKEND_INFO,
      info.Id(), info.Description(), BackendCaps(info.Capabilities()), status ? status.GetText() : Text::INFO_STATUS_OK);
  }
  
  inline void ShowBackends(Sound::BackendInformation::Iterator::Ptr backends)
  {
    for (; backends->IsValid(); backends->Next())
    {
      ShowBackend(*backends->Get());
    }
  }
  
  inline void ShowProvider(const IO::Provider& provider)
  {
    const Error& status = provider.Status();
    StdOut << Strings::Format(Text::INFO_PROVIDER_INFO,
      provider.Id(), provider.Description(), status ? status.GetText() : Text::INFO_STATUS_OK);
  }
  
  inline void ShowProviders()
  {
    StdOut << Text::INFO_LIST_PROVIDERS_TITLE << std::endl;
    for (IO::Provider::Iterator::Ptr providers = IO::EnumerateProviders(); 
      providers->IsValid(); providers->Next())
    {
      ShowProvider(*providers->Get());
    }
  }
  
  typedef boost::variant<Parameters::IntType, Parameters::StringType> ValueType;

  struct OptionDesc
  {
    OptionDesc(const Parameters::NameType& name, String descr, ValueType def)
      : Name(FromStdString(name.FullPath()))
      , Desc(std::move(descr))
      , Default(std::move(def))
    {
    }

    OptionDesc(String name, String descr, ValueType def)
      : Name(std::move(name))
      , Desc(std::move(descr))
      , Default(std::move(def))
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
      OptionDesc(Parameters::ZXTune::Sound::LOOPED,
                 Text::INFO_OPTIONS_SOUND_LOOPED,
                 EMPTY),
      //Mixer parameters
      OptionDesc(Text::INFO_OPTIONS_SOUND_MIXER_TITLE, EMPTY, 0),
      OptionDesc(Parameters::ZXTune::Sound::Mixer::PREFIX + Text::INFO_OPTIONS_SOUND_MIXER_TEMPLATE,
                 Text::INFO_OPTIONS_SOUND_MIXER,
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
      OptionDesc(Parameters::ZXTune::Sound::Backends::Alsa::LATENCY,
                 Text::INFO_OPTIONS_SOUND_BACKENDS_ALSA_LATENCY,
                 Parameters::ZXTune::Sound::Backends::Alsa::LATENCY_DEFAULT),
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
                 Parameters::ZXTune::Core::AYM::TYPE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Core::AYM::INTERPOLATION,
                 Text::INFO_OPTIONS_CORE_AYM_INTERPOLATION,
                 Parameters::ZXTune::Core::AYM::INTERPOLATION_DEFAULT),
      OptionDesc(Parameters::ZXTune::Core::AYM::TABLE,
                 Text::INFO_OPTIONS_CORE_AYM_TABLE,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Core::AYM::DUTY_CYCLE,
                 Text::INFO_OPTIONS_CORE_AYM_DUTY_CYCLE,
                 Parameters::ZXTune::Core::AYM::DUTY_CYCLE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Core::AYM::DUTY_CYCLE_MASK,
                 Text::INFO_OPTIONS_CORE_AYM_DUTY_CYCLE_MASK,
                 Parameters::ZXTune::Core::AYM::DUTY_CYCLE_MASK_DEFAULT),
      OptionDesc(Parameters::ZXTune::Core::AYM::LAYOUT,
                 Text::INFO_OPTIONS_CORE_AYM_LAYOUT,
                 EMPTY),
      OptionDesc(Parameters::ZXTune::Core::DAC::INTERPOLATION,
                 Text::INFO_OPTIONS_CORE_DAC_INTERPOLATION,
                 Parameters::ZXTune::Core::DAC::INTERPOLATION_DEFAULT),
      OptionDesc(Parameters::ZXTune::Core::Z80::INT_TICKS,
                 Text::INFO_OPTIONS_CORE_Z80_INT_TICKS,
                 Parameters::ZXTune::Core::Z80::INT_TICKS_DEFAULT),
      OptionDesc(Parameters::ZXTune::Core::Z80::CLOCKRATE,
                 Text::INFO_OPTIONS_CORE_Z80_CLOCKRATE,
                 Parameters::ZXTune::Core::Z80::CLOCKRATE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Core::FM::CLOCKRATE,
                 Text::INFO_OPTIONS_CORE_FM_CLOCKRATE,
                 Parameters::ZXTune::Core::FM::CLOCKRATE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Core::SAA::CLOCKRATE,
                 Text::INFO_OPTIONS_CORE_SAA_CLOCKRATE,
                 Parameters::ZXTune::Core::SAA::CLOCKRATE_DEFAULT),
      OptionDesc(Parameters::ZXTune::Core::SAA::INTERPOLATION,
                 Text::INFO_OPTIONS_CORE_SAA_INTERPOLATION,
                 Parameters::ZXTune::Core::SAA::INTERPOLATION_DEFAULT),
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
      OptionDesc(Parameters::ZXTune::Core::Plugins::Zip::MAX_DEPACKED_FILE_SIZE_MB,
                 Text::INFO_OPTIONS_CORE_PLUGINS_ZIP_MAX_DEPACKED_FILE_SIZE_MB,
                 Parameters::ZXTune::Core::Plugins::Zip::MAX_DEPACKED_FILE_SIZE_MB_DEFAULT),
    };
    StdOut << Text::INFO_LIST_OPTIONS_TITLE << std::endl;
    std::for_each(OPTIONS, std::end(OPTIONS), ShowOption);
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
      AttrType(Module::ATTR_EXTENSION, Text::INFO_ATTRIBUTES_EXTENSION),
      AttrType(Module::ATTR_FILENAME, Text::INFO_ATTRIBUTES_FILENAME),
      AttrType(Module::ATTR_PATH, Text::INFO_ATTRIBUTES_PATH),
      AttrType(Module::ATTR_FULLPATH, Text::INFO_ATTRIBUTES_FULLPATH),
      //internal
      AttrType(Module::ATTR_TYPE, Text::INFO_ATTRIBUTES_TYPE),
      AttrType(Module::ATTR_CONTAINER, Text::INFO_ATTRIBUTES_CONTAINER),
      AttrType(Module::ATTR_SUBPATH, Text::INFO_ATTRIBUTES_SUBPATH),
      AttrType(Module::ATTR_AUTHOR, Text::INFO_ATTRIBUTES_AUTHOR),
      AttrType(Module::ATTR_TITLE, Text::INFO_ATTRIBUTES_TITLE),
      AttrType(Module::ATTR_PROGRAM, Text::INFO_ATTRIBUTES_PROGRAM),
      AttrType(Module::ATTR_COMPUTER, Text::INFO_ATTRIBUTES_COMPUTER),
      AttrType(Module::ATTR_DATE, Text::INFO_ATTRIBUTES_DATE),
      AttrType(Module::ATTR_COMMENT, Text::INFO_ATTRIBUTES_COMMENT),
      AttrType(Module::ATTR_CRC, Text::INFO_ATTRIBUTES_CRC),
      AttrType(Module::ATTR_SIZE, Text::INFO_ATTRIBUTES_SIZE),
      //runtime
      AttrType(Module::ATTR_CURRENT_POSITION, Text::INFO_ATTRIBUTES_CURRENT_POSITION),
      AttrType(Module::ATTR_CURRENT_PATTERN, Text::INFO_ATTRIBUTES_CURRENT_PATTERN),
      AttrType(Module::ATTR_CURRENT_LINE, Text::INFO_ATTRIBUTES_CURRENT_LINE)
    };
    StdOut << Text::INFO_LIST_ATTRIBUTES_TITLE << std::endl;
    std::for_each(ATTRIBUTES, std::end(ATTRIBUTES), ShowAttribute);
  }
  
  void ShowFreqtables()
  {
    static const String FREQTABLES[] =
    {
      Module::TABLE_SOUNDTRACKER,
      Module::TABLE_PROTRACKER2,
      Module::TABLE_PROTRACKER3_3,
      Module::TABLE_PROTRACKER3_4,
      Module::TABLE_PROTRACKER3_3_ASM,
      Module::TABLE_PROTRACKER3_4_ASM,
      Module::TABLE_PROTRACKER3_3_REAL,
      Module::TABLE_PROTRACKER3_4_REAL,
      Module::TABLE_ASM,
      Module::TABLE_SOUNDTRACKER_PRO,
      Module::TABLE_NATURAL_SCALED
    };
    StdOut << Text::INFO_LIST_FREQTABLES_TITLE;
    std::copy(FREQTABLES, std::end(FREQTABLES), std::ostream_iterator<String>(StdOut, " "));
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
    
    const boost::program_options::options_description& GetOptionsDescription() const override
    {
      return OptionsDescription;
    }
    
    bool Process(SoundComponent& sound) const override
    {
      if (EnumPlugins)
      {
        ShowPlugins();
      }
      if (EnumBackends)
      {
        ShowBackends(sound.EnumerateBackends());
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

std::unique_ptr<InformationComponent> InformationComponent::Create()
{
  return std::unique_ptr<InformationComponent>(new Information);
}
