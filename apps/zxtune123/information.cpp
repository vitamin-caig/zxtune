/**
 *
 * @file
 *
 * @brief Information component implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune123/information.h"

#include "apps/zxtune123/sound.h"

#include "core/plugins/parameters.h"
#include "io/providers/parameters.h"
#include "module/players/aym/freq_tables.h"
#include "sound/backends/parameters.h"

#include "core/parameters.h"
#include "core/plugin.h"
#include "core/plugin_attrs.h"
#include "io/io_parameters.h"
#include "io/provider.h"
#include "module/attributes.h"
#include "platform/application.h"
#include "sound/backend.h"
#include "sound/backend_attrs.h"
#include "sound/mixer_parameters.h"
#include "sound/parameters.h"
#include "sound/service.h"
#include "strings/format.h"

#include "static_string.h"
#include "string_view.h"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>

#include <iostream>
#include <variant>

namespace
{
  using CapsPair = std::pair<uint_t, StringView>;
  String SerializeBitmask(uint_t caps, const CapsPair* from)
  {
    String result;
    for (const CapsPair* cap = from; cap->first; ++cap)
    {
      if (cap->first & caps)
      {
        result += ' ';
        result += cap->second;
        caps ^= cap->first;
      }
    }
    if (caps)
    {
      result += Strings::Format(" 0x{:08x}", caps);
    }
    return result;
  }

  String SerializeEnum(uint_t caps, const CapsPair* from)
  {
    for (const CapsPair* cap = from; !cap->second.empty(); ++cap)
    {
      if (cap->first == caps)
      {
        return " "s + cap->second;
      }
    }
    return " unknown";
  }

  template<class C, C... Chars>
  constexpr auto operator"" _cat()
  {
    return "cat_"_ss + basic_static_string<C, Chars...>{};
  }

  template<class C, C... Chars>
  constexpr auto operator"" _mod()
  {
    return "mod_"_ss + basic_static_string<C, Chars...>{};
  }

  template<class C, C... Chars>
  constexpr auto operator"" _dev()
  {
    return "dev_"_ss + basic_static_string<C, Chars...>{};
  }

  template<class C, C... Chars>
  constexpr auto operator"" _conv()
  {
    return "conv_"_ss + basic_static_string<C, Chars...>{};
  }

  template<class C, C... Chars>
  constexpr auto operator"" _cap()
  {
    return "cap_"_ss + basic_static_string<C, Chars...>{};
  }

  template<class C, C... Chars>
  constexpr auto operator"" _cont()
  {
    return "cont_"_ss + basic_static_string<C, Chars...>{};
  }

  template<class C, C... Chars>
  constexpr auto operator"" _type()
  {
    return "type_"_ss + basic_static_string<C, Chars...>{};
  }

  String PluginCaps(uint_t caps)
  {
    using namespace ZXTune;
    static const CapsPair PLUGIN_TYPES[] = {
        {Capabilities::Category::MODULE, "module"_cat}, {Capabilities::Category::CONTAINER, "container"_cat}, {}};

    static const CapsPair MODULE_TYPES[] = {{Capabilities::Module::Type::TRACK, "track"_mod},
                                            {Capabilities::Module::Type::STREAM, "stream"_mod},
                                            {Capabilities::Module::Type::MEMORYDUMP, "memorydump"_mod},
                                            {Capabilities::Module::Type::MULTI, "multi"_mod},
                                            CapsPair()};

    static const CapsPair MODULE_CAPS[] = {// device caps
                                           {Capabilities::Module::Device::AY38910, "ay38910"_dev},
                                           {Capabilities::Module::Device::TURBOSOUND, "turbosound"_dev},
                                           {Capabilities::Module::Device::BEEPER, "beeper"_dev},
                                           {Capabilities::Module::Device::YM2203, "ym2203"_dev},
                                           {Capabilities::Module::Device::TURBOFM, "turbofm"_dev},
                                           {Capabilities::Module::Device::DAC, "dac"_dev},
                                           {Capabilities::Module::Device::SAA1099, "saa1099"_dev},
                                           {Capabilities::Module::Device::MOS6581, "mos6581"_dev},
                                           {Capabilities::Module::Device::SPC700, "spc700"_dev},
                                           {Capabilities::Module::Device::MULTI, "multi"_dev},
                                           {Capabilities::Module::Device::RP2A0X, "rp2a0x"_dev},
                                           {Capabilities::Module::Device::LR35902, "lr35902"_dev},
                                           {Capabilities::Module::Device::CO12294, "co12294"_dev},
                                           {Capabilities::Module::Device::HUC6270, "huc6270"_dev},
                                           // conversion caps
                                           {Capabilities::Module::Conversion::PSG, "psg"_conv},
                                           {Capabilities::Module::Conversion::ZX50, "zx50"_conv},
                                           {Capabilities::Module::Conversion::TXT, "txt"_conv},
                                           {Capabilities::Module::Conversion::AYDUMP, "aydump"_conv},
                                           {Capabilities::Module::Conversion::FYM, "fym"_conv},
                                           // traits caps
                                           {Capabilities::Module::Traits::MULTIFILE, "multifile"_cap},
                                           // limiter
                                           {}};

    static const CapsPair CONTAINER_TYPES[] = {{Capabilities::Container::Type::ARCHIVE, "archive"_cont},
                                               {Capabilities::Container::Type::COMPRESSOR, "compressor"_cont},
                                               {Capabilities::Container::Type::SNAPSHOT, "snapshot"_cont},
                                               {Capabilities::Container::Type::DISKIMAGE, "diskimage"_cont},
                                               {Capabilities::Container::Type::DECOMPILER, "decompiler"_cont},
                                               {Capabilities::Container::Type::MULTITRACK, "multitrack"_cont},
                                               {Capabilities::Container::Type::SCANER, "scaner"_cont},
                                               // limiter
                                               {}};

    static const CapsPair CONTAINER_CAPS[] = {{Capabilities::Container::Traits::DIRECTORIES, "dirs"_cap},
                                              {Capabilities::Container::Traits::PLAIN, "plain"_cap},
                                              // limiter
                                              {}};
    String result = SerializeEnum(caps & Capabilities::Category::MASK, PLUGIN_TYPES);
    switch (caps & Capabilities::Category::MASK)
    {
    case Capabilities::Category::MODULE:
      result += SerializeEnum(caps & Capabilities::Module::Type::MASK, MODULE_TYPES);
      result +=
          SerializeBitmask(caps & ~(Capabilities::Category::MASK | Capabilities::Module::Type::MASK), MODULE_CAPS);
      break;
    case Capabilities::Category::CONTAINER:
      result += SerializeEnum(caps & Capabilities::Container::Type::MASK, CONTAINER_TYPES);
      result += SerializeBitmask(caps & ~(Capabilities::Category::MASK | Capabilities::Container::Type::MASK),
                                 CONTAINER_CAPS);
      break;
    }
    return result.substr(1);
  }

  String BackendCaps(uint_t caps)
  {
    static const CapsPair BACKENDS_CAPS[] = {// Type-related capabilities
                                             {Sound::CAP_TYPE_STUB, "stub"_type},
                                             {Sound::CAP_TYPE_SYSTEM, "system"_type},
                                             {Sound::CAP_TYPE_FILE, "file"_type},
                                             {Sound::CAP_TYPE_HARDWARE, "hardware"_type},
                                             // Features-related capabilities
                                             {Sound::CAP_FEAT_HWVOLUME, "feat_hwvolume"},
                                             // limiter
                                             {}};
    return SerializeBitmask(caps, BACKENDS_CAPS);
  }

  inline String DescribePlugin(const ZXTune::Plugin& plugin)
  {
    return Strings::Format(
        "Plugin:       {0}\n"
        "Description:  {1}\n"
        "Capabilities: {2}\n"
        "\n",
        plugin.Id(), plugin.Description(), PluginCaps(plugin.Capabilities()));
  }

  inline void ShowPlugins()
  {
    class PluginsPrinter : public ZXTune::PluginVisitor
    {
    public:
      void Visit(const ZXTune::Plugin& plugin) override
      {
        StdOut << DescribePlugin(plugin);
      }
    };
    StdOut << "Supported plugins:" << std::endl;
    PluginsPrinter print;
    ZXTune::EnumeratePlugins(print);
  }

  inline String DescribeBackend(const Sound::BackendInformation& info)
  {
    const Error& status = info.Status();
    return Strings::Format(
        "Backend:      {0}\n"
        "Description:  {1}\n"
        "Capabilities: {2}\n"
        "Status:       {3}\n"
        "\n",
        info.Id(), info.Description(), BackendCaps(info.Capabilities()), status ? status.GetText() : "Available");
  }

  inline void ShowBackends(std::span<const Sound::BackendInformation::Ptr> backends)
  {
    StdOut << "Supported backends:" << std::endl;
    for (const auto& backend : backends)
    {
      StdOut << DescribeBackend(*backend);
    }
  }

  inline String DescribeProvider(const IO::Provider& provider)
  {
    const Error& status = provider.Status();
    return Strings::Format(
        "Provider:    {0}\n"
        "Description: {1}\n"
        "Status:      {2}\n"
        "\n",
        provider.Id(), provider.Description(), status ? status.GetText() : "Available");
  }

  inline void ShowProviders()
  {
    StdOut << "Supported IO providers:" << std::endl;
    for (const auto& provider : IO::EnumerateProviders())
    {
      StdOut << DescribeProvider(*provider);
    }
  }

  using ValueType = std::variant<Parameters::IntType, Parameters::StringType>;

  struct OptionDesc
  {
    OptionDesc(StringView name)
      : Name(name)
    {}

    OptionDesc(Parameters::Identifier name, const char* descr, ValueType def)
      : Name(name)
      , Desc(descr)
      , Default(std::move(def))
    {}

    StringView Name;
    const char* const Desc = nullptr;
    ValueType Default;

    String Describe() const
    {
      // section
      if (!Desc)
      {
        return String{Name};
      }
      else
      {
        if (const auto* defValString = std::get_if<Parameters::StringType>(&Default))
        {
          if (defValString->empty())
          {
            return Strings::Format("  {0}\n  - {1}.", Name, Desc);
          }
          else
          {
            return Strings::Format("  {0}\n  - {1} (default value is '{2}').", Name, Desc, *defValString);
          }
        }
        else
        {
          return Strings::Format("  {0}\n  - {1} (default value is {2}).", Name, Desc,
                                 std::get<Parameters::IntType>(Default));
        }
      }
    }
  };

  void ShowOptions()
  {
    static const String EMPTY;

    using Parameters::operator""_id;

    static const OptionDesc OPTIONS[] = {
        // IO parameters
        {" IO providers options:"},
        {Parameters::ZXTune::IO::Providers::File::MmapThreshold::Key, "minimal size for use memory mapping",
         Parameters::IntType(Parameters::ZXTune::IO::Providers::File::MmapThreshold::Default)},
        {Parameters::ZXTune::IO::Providers::File::CreateDirectories::Key,
         "create all intermediate directories (applicable for for file-based backends)",
         Parameters::IntType(Parameters::ZXTune::IO::Providers::File::CreateDirectories::Default)},
        {Parameters::ZXTune::IO::Providers::File::OverwriteExisting::Key,
         "overwrite target file if already exists (applicable for for file-based backends)",
         Parameters::IntType(Parameters::ZXTune::IO::Providers::File::OverwriteExisting::Default)},
        // Sound parameters
        {" Sound options:"},
        {Parameters::ZXTune::Sound::Frequency::Key, "sound frequency in Hz",
         Parameters::IntType(Parameters::ZXTune::Sound::Frequency::Default)},
        {Parameters::ZXTune::Sound::Looped::Key, "loop playback", EMPTY},
        // Mixer parameters
        {" Mixer options:"},
        {Parameters::ZXTune::Sound::Mixer::Prefix + "A.B_C"_id,
         "specify level in percent (A- total channels count, B- input channel, C- output channel). E.g. when A=3, "
         "B=0, C=1 will specify A channel level of AY/YM-like chiptunes to right stereochannel",
         EMPTY},
        // Sound backend parameters
        {" Sound backends options:"},
        {Parameters::ZXTune::Sound::Backends::File::Filename::Key,
         "filename template for file-based backends (see --list-attributes command). Also duplicated in "
         "backend-specific namespace",
         EMPTY},
        {Parameters::ZXTune::Sound::Backends::File::Buffers::Key,
         "use asynchronous conversing with specified queue size for file-based backends. Also duplicated in "
         "backend-specific namespace",
         Parameters::ZXTune::Sound::Backends::File::Buffers::Default},
        {Parameters::ZXTune::Sound::Backends::Win32::Device::Key, "device index for win32 backend",
         Parameters::ZXTune::Sound::Backends::Win32::Device::Default},
        {Parameters::ZXTune::Sound::Backends::Win32::Buffers::Key, "playback buffers count",
         Parameters::ZXTune::Sound::Backends::Win32::Buffers::Default},
        {Parameters::ZXTune::Sound::Backends::Oss::Device::Key, "playback device for OSS backend",
         Parameters::ZXTune::Sound::Backends::Oss::Device::Default},
        {Parameters::ZXTune::Sound::Backends::Oss::Mixer::Key, "mixer device for OSS backend",
         Parameters::ZXTune::Sound::Backends::Oss::Mixer::Default},
        {Parameters::ZXTune::Sound::Backends::Alsa::Device::Key, "playback device for ALSA backend",
         Parameters::ZXTune::Sound::Backends::Alsa::Device::Default},
        {Parameters::ZXTune::Sound::Backends::Alsa::Mixer::Key,
         "mixer for ALSA backend (taking the first one if not specified)", EMPTY},
        {Parameters::ZXTune::Sound::Backends::Alsa::Latency::Key, "latency in ms for ALSA backend",
         Parameters::ZXTune::Sound::Backends::Alsa::Latency::Default},
        {Parameters::ZXTune::Sound::Backends::Sdl::Buffers::Key, "buffers count for SDL backend",
         Parameters::ZXTune::Sound::Backends::Sdl::Buffers::Default},
        {Parameters::ZXTune::Sound::Backends::DirectSound::Latency::Key, "latency in ms for DirectSound backend",
         Parameters::ZXTune::Sound::Backends::DirectSound::Latency::Default},
        // Mp3
        {Parameters::ZXTune::Sound::Backends::Mp3::Mode::Key, "specify mode for Mp3 backend (cbr/abr/vbr)",
         Parameters::ZXTune::Sound::Backends::Mp3::Mode::Default},
        {Parameters::ZXTune::Sound::Backends::Mp3::Bitrate::Key,
         "specify bitrate in kbps for Mp3 backend in mode=cbr or mode=abr",
         Parameters::ZXTune::Sound::Backends::Mp3::Bitrate::Default},
        {Parameters::ZXTune::Sound::Backends::Mp3::Quality::Key,
         "specify quality for Mp3 backend (0- highest, 9- lowest) in mode=vbr",
         Parameters::ZXTune::Sound::Backends::Mp3::Quality::Default},
        {Parameters::ZXTune::Sound::Backends::Mp3::Channels::Key,
         "specify channels encoding mode (default/stereo/jointstereo/mode)",
         Parameters::ZXTune::Sound::Backends::Mp3::Channels::Default},
        // Ogg
        {Parameters::ZXTune::Sound::Backends::Ogg::Mode::Key, "specify mode for Ogg backend (quality/abr)",
         Parameters::ZXTune::Sound::Backends::Ogg::Mode::Default},
        {Parameters::ZXTune::Sound::Backends::Ogg::Quality::Key,
         "specify quality for Ogg backend (0- lowest, 10- highest) in mode=quality",
         Parameters::ZXTune::Sound::Backends::Ogg::Quality::Default},
        {Parameters::ZXTune::Sound::Backends::Ogg::Bitrate::Key,
         "specify averate bitrate in kbps for Ogg backend in mode=abr",
         Parameters::ZXTune::Sound::Backends::Ogg::Bitrate::Default},
        // flac
        {Parameters::ZXTune::Sound::Backends::Flac::Compression::Key,
         "specify compression level for Flac backend (0- lowest, 8- highest)", EMPTY},
        {Parameters::ZXTune::Sound::Backends::Flac::Blocksize::Key, "specyfy blocksize in samples for Flac backend",
         EMPTY},
        // Core options
        {" Core options:"},
        {Parameters::ZXTune::Core::AYM::Clockrate::Key, "clock rate for AYM in Hz",
         Parameters::IntType(Parameters::ZXTune::Core::AYM::Clockrate::Default)},
        {Parameters::ZXTune::Core::AYM::Chip::Key, "use YM chip type for AYM rendering",
         Parameters::IntType(Parameters::ZXTune::Core::AYM::Chip::Default)},
        {Parameters::ZXTune::Core::AYM::Interpolation::Key, "use interpolation for AYM rendering",
         Parameters::IntType(Parameters::ZXTune::Core::AYM::Interpolation::Default)},
        {Parameters::ZXTune::Core::AYM::Table::Key,
         "frequency table name to use in AY-based players. Can be name (--list-freqtables) or dump (#..). Use '~' to "
         "revert",
         EMPTY},
        {Parameters::ZXTune::Core::AYM::DutyCycle::Key, "chip duty cycle value in percent. Should be in range 1..99",
         Parameters::IntType(Parameters::ZXTune::Core::AYM::DutyCycle::Default)},
        {Parameters::ZXTune::Core::AYM::DutyCycleMask::Key,
         "chip channels mask where duty cycle will be applied. Can be set of letters (A,B,C) or numeric mask",
         Parameters::IntType(Parameters::ZXTune::Core::AYM::DutyCycleMask::Default)},
        {Parameters::ZXTune::Core::AYM::Layout::Key,
         "chip channels layout. Set of letters or numeric (0-ABC, 1-ACB, 2-BAC, 3-BCA, 4-CBA, 5-CAB)",
         Parameters::IntType(Parameters::ZXTune::Core::AYM::Layout::Default)},
        {Parameters::ZXTune::Core::DAC::Interpolation::Key, "use interpolation for DAC rendering",
         Parameters::IntType(Parameters::ZXTune::Core::DAC::Interpolation::Default)},
        {Parameters::ZXTune::Core::Z80::IntTicks::Key, "Z80 processor INT signal duration in ticks",
         Parameters::IntType(Parameters::ZXTune::Core::Z80::IntTicks::Default)},
        {Parameters::ZXTune::Core::Z80::Clockrate::Key, "Z80 processor clockrate",
         Parameters::IntType(Parameters::ZXTune::Core::Z80::Clockrate::Default)},
        {Parameters::ZXTune::Core::FM::Clockrate::Key, "clock rate for FM in Hz",
         Parameters::IntType(Parameters::ZXTune::Core::FM::Clockrate::Default)},
        {Parameters::ZXTune::Core::SAA::Clockrate::Key, "clock rate for SAA in Hz",
         Parameters::IntType(Parameters::ZXTune::Core::SAA::Clockrate::Default)},
        {Parameters::ZXTune::Core::SAA::Interpolation::Key, "use interpolation for SAA rendering",
         Parameters::IntType(Parameters::ZXTune::Core::SAA::Interpolation::Default)},
        // Core plugins options
        {" Core plugins options:"},
        {Parameters::ZXTune::Core::Plugins::Raw::PlainDoubleAnalysis::Key, "analyze cap_plain plugins twice", EMPTY},
        {Parameters::ZXTune::Core::Plugins::Raw::MinSize::Key, "minimum data size to use raw scaner",
         Parameters::IntType(Parameters::ZXTune::Core::Plugins::Raw::MinSize::Default)},
        {Parameters::ZXTune::Core::Plugins::Hrip::IgnoreCorrupted::Key, "ignore corrupted blocks in HRiP archive",
         EMPTY},
        {Parameters::ZXTune::Core::Plugins::Zip::MaxDepackedFileSizeMb::Key,
         "maximal file size to be depacked from .zip archive",
         Parameters::IntType(Parameters::ZXTune::Core::Plugins::Zip::MaxDepackedFileSizeMb::Default)}};
    StdOut << "Supported zxtune options:" << std::endl;
    for (const auto& opt : OPTIONS)
    {
      StdOut << opt.Describe() << std::endl;
    }
  }

  using AttrType = std::pair<StringView, const char*>;
  void ShowAttribute(const AttrType& arg)
  {
    StdOut << Strings::Format(" {0:<20}- {1}", arg.first, arg.second) << std::endl;
  }

  void ShowAttributes()
  {
    static const AttrType ATTRIBUTES[] = {
        // external
        {Module::ATTR_EXTENSION, "source file extension"},
        {Module::ATTR_FILENAME, "source file name"},
        {Module::ATTR_PATH, "full source file path"},
        {Module::ATTR_FULLPATH, "full source data identifier including full filename and subpath"},
        // internal
        {Module::ATTR_TYPE, "module type (plugin id)"},
        {Module::ATTR_CONTAINER, "nested containers chain"},
        {Module::ATTR_SUBPATH, "module subpath in top-level data container"},
        {Module::ATTR_AUTHOR, "module author"},
        {Module::ATTR_TITLE, "module title"},
        {Module::ATTR_PROGRAM, "program module was created in"},
        {Module::ATTR_COMPUTER, "computer-specific information about module"},
        {Module::ATTR_DATE, "module date information"},
        {Module::ATTR_COMMENT, "embedded module comment"},
        {Module::ATTR_CRC, "original module data crc32 checksum"},
        {Module::ATTR_SIZE, "original module data size"},
        // runtime
        {Module::ATTR_CURRENT_POSITION, "currently played position"},
        {Module::ATTR_CURRENT_PATTERN, "currently played pattern number"},
        {Module::ATTR_CURRENT_LINE, "currently played pattern line"}};
    StdOut << "Supported module attributes:" << std::endl;
    std::for_each(ATTRIBUTES, std::end(ATTRIBUTES), ShowAttribute);
  }

  void ShowFreqtables()
  {
    StdOut << "Supported frequency tables:";
    for (const auto& table : Module::AYM::GetAvailableFreqTables())
    {
      StdOut << ' ' << table;
    }
    StdOut << std::endl;
  }

  class Information : public InformationComponent
  {
  public:
    Information()
      : OptionsDescription("Information keys")
    {
      using namespace boost::program_options;
      auto opt = OptionsDescription.add_options();
      opt("list-plugins", bool_switch(&EnumPlugins), "show list of supported plugins");
      opt("list-backends", bool_switch(&EnumBackends), "show list of supported backends");
      opt("list-providers", bool_switch(&EnumProviders), "show list of supported IO providers");
      opt("list-options", bool_switch(&EnumOptions), "show list of supported options");
      opt("list-attributes", bool_switch(&EnumAttributes), "show list of supported attributes");
      opt("list-freqtables", bool_switch(&EnumFreqtables), "show list of supported frequency tables");
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
    bool EnumPlugins = false;
    bool EnumBackends = false;
    bool EnumProviders = false;
    bool EnumOptions = false;
    bool EnumAttributes = false;
    bool EnumFreqtables = false;
  };
}  // namespace

std::unique_ptr<InformationComponent> InformationComponent::Create()
{
  return std::unique_ptr<InformationComponent>(new Information);
}
