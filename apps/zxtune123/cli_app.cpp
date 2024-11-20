/**
 *
 * @file
 *
 * @brief CLI application implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune123/config.h"
#include "apps/zxtune123/console.h"
#include "apps/zxtune123/display.h"
#include "apps/zxtune123/information.h"
#include "apps/zxtune123/sound.h"
#include "apps/zxtune123/source.h"

#include "module/conversion/api.h"
#include "module/conversion/parameters.h"

#include "async/data_receiver.h"
#include "binary/container.h"
#include "binary/container_factories.h"
#include "binary/crc.h"
#include "io/api.h"
#include "io/template.h"
#include "math/fixedpoint.h"
#include "module/attributes.h"
#include "module/holder.h"
#include "module/information.h"
#include "module/renderer.h"
#include "module/state.h"
#include "parameters/container.h"
#include "parameters/merged_accessor.h"
#include "parameters/template.h"
#include "platform/application.h"
#include "platform/version/api.h"
#include "sound/backend.h"
#include "sound/chunk.h"
#include "sound/gain.h"
#include "strings/array.h"
#include "strings/fields.h"
#include "strings/format.h"
#include "strings/template.h"
#include "time/instant.h"
#include "time/timer.h"
#include "tools/data_streaming.h"
#include "tools/progress_callback.h"

#include "contract.h"
#include "error_tools.h"
#include "make_ptr.h"
#include "string_view.h"
#include "types.h"

#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include <algorithm>
#include <exception>
#include <memory>
#include <optional>
#include <ostream>
#include <utility>

namespace
{
  String GetModuleId(const Parameters::Accessor& props)
  {
    return Parameters::GetString(props, Module::ATTR_FULLPATH);
  }

  String GetFilenameTemplate(const Parameters::Accessor& params)
  {
    auto res = Parameters::GetString(params, "filename"sv);
    if (res.empty())
    {
      throw Error(THIS_LINE, "Output filename template is not specified.");
    }
    return res;
  }

  struct HolderAndData
  {
    Module::Holder::Ptr Holder;
    Binary::Data::Ptr Data;

    using Receiver = DataReceiver<HolderAndData>;
  };

  class SaveEndpoint : public HolderAndData::Receiver
  {
  public:
    SaveEndpoint(DisplayComponent& display, const Parameters::Accessor& params)
      : Display(display)
      , Params(params)
      , FileNameTemplate(IO::CreateFilenameTemplate(GetFilenameTemplate(params)))
    {}

    void ApplyData(HolderAndData data) override
    {
      try
      {
        const Parameters::Accessor::Ptr props = data.Holder->GetModuleProperties();
        const auto& id = GetModuleId(*props);
        const auto& filename =
            FileNameTemplate->Instantiate(Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource>(*props));
        const auto stream = IO::CreateStream(filename, Params, Log::ProgressCallback::Stub());
        stream->ApplyData(*data.Data);
        Display.Message("Converted '{0}' => '{1}'", id, filename);
      }
      catch (const Error& e)
      {
        StdOut << e.ToString();
      }
    }

    void Flush() override {}

  private:
    DisplayComponent& Display;
    const Parameters::Accessor& Params;
    const Strings::Template::Ptr FileNameTemplate;
  };

  std::unique_ptr<Module::Conversion::Parameter> CreateConversionParameters(StringView mode,
                                                                            const Parameters::Accessor& modeParams)
  {
    const auto optimization =
        Parameters::GetInteger<uint_t>(modeParams, "optimization"sv, Module::Conversion::DEFAULT_OPTIMIZATION);
    std::unique_ptr<Module::Conversion::Parameter> param;
    if (mode == "psg"sv)
    {
      param = std::make_unique<Module::Conversion::PSGConvertParam>(optimization);
    }
    else if (mode == "zx50"sv)
    {
      param = std::make_unique<Module::Conversion::ZX50ConvertParam>(optimization);
    }
    else if (mode == "txt"sv)
    {
      param = std::make_unique<Module::Conversion::TXTConvertParam>();
    }
    else if (mode == "debugay"sv)
    {
      param = std::make_unique<Module::Conversion::DebugAYConvertParam>(optimization);
    }
    else if (mode == "aydump"sv)
    {
      param = std::make_unique<Module::Conversion::AYDumpConvertParam>(optimization);
    }
    else if (mode == "fym"sv)
    {
      param = std::make_unique<Module::Conversion::FYMConvertParam>(optimization);
    }
    else
    {
      throw Error(THIS_LINE, "Unknown conversion mode specified.");
    }
    return param;
  }

  class TruncateDataEndpoint : public HolderAndData::Receiver
  {
  public:
    explicit TruncateDataEndpoint(Ptr saver)
      : Saver(std::move(saver))
    {}

    void ApplyData(HolderAndData data) override
    {
      const auto availSize = data.Data->Size();
      const auto usedSize =
          Parameters::GetInteger<decltype(availSize)>(*data.Holder->GetModuleProperties(), Module::ATTR_SIZE);
      if (availSize != usedSize)
      {
        Require(availSize > usedSize);
        data.Data = Binary::CreateContainer(std::move(data.Data))->GetSubcontainer(0, usedSize);
      }
      Saver->ApplyData(std::move(data));
    }

    void Flush() override
    {
      Saver->Flush();
    }

  private:
    const Ptr Saver;
  };

  class ConvertEndpoint : public HolderAndData::Receiver
  {
  public:
    ConvertEndpoint(DisplayComponent& display, StringView mode, const Parameters::Accessor& modeParams, Ptr saver)
      : Display(display)
      , ConversionParameter(CreateConversionParameters(mode, modeParams))
      , Saver(std::move(saver))
    {}

    void ApplyData(HolderAndData data) override
    {
      const auto holder = data.Holder;
      const auto props = holder->GetModuleProperties();
      if (const Binary::Data::Ptr result = Module::Convert(*holder, *ConversionParameter, props))
      {
        Saver->ApplyData({holder, result});
      }
      else
      {
        const auto& type = Parameters::GetString(*props, Module::ATTR_TYPE);
        const auto& id = GetModuleId(*props);
        Display.Message("Skipping '{0}' (type '{1}') due to convert impossibility.", id, type);
      }
    }

    void Flush() override
    {
      Saver->Flush();
    }

  private:
    DisplayComponent& Display;
    const std::unique_ptr<Module::Conversion::Parameter> ConversionParameter;
    const Ptr Saver;
  };

  class Convertor : public OnItemCallback
  {
  public:
    Convertor(const Parameters::Accessor& params, DisplayComponent& display)
      : Pipe(HolderAndData::Receiver::CreateStub())
    {
      const auto mode = params.FindString("mode"sv);
      if (!mode)
      {
        throw Error(THIS_LINE, "Conversion mode is not specified.");
      }
      const HolderAndData::Receiver::Ptr saver(new SaveEndpoint(display, params));
      const HolderAndData::Receiver::Ptr target = *mode == "raw"
                                                      ? MakePtr<TruncateDataEndpoint>(saver)
                                                      : MakePtr<ConvertEndpoint>(display, *mode, params, saver);
      Pipe = Async::DataReceiver<HolderAndData>::Create(1, 1000, target);
    }

    ~Convertor() override
    {
      Pipe->Flush();
    }

    void ProcessItem(Binary::Data::Ptr data, Module::Holder::Ptr holder) override
    {
      Pipe->ApplyData({std::move(holder), std::move(data)});
    }

  private:
    HolderAndData::Receiver::Ptr Pipe;
  };

  class Benchmark : public OnItemCallback
  {
  public:
    Benchmark(unsigned iterations, bool dumpUnknownData, SoundComponent& sound, DisplayComponent& display)
      : Iterations(iterations)
      , DumpUnknownData(dumpUnknownData)
      , Sounder(sound)
      , Display(display)
    {}

    void ProcessItem(Binary::Data::Ptr /*data*/, Module::Holder::Ptr holder) override
    {
      const Module::Information::Ptr info = holder->GetModuleInformation();
      const Parameters::Accessor::Ptr props = holder->GetModuleProperties();
      const auto& path = Parameters::GetString(*props, Module::ATTR_FULLPATH);
      const auto& type = Parameters::GetString(*props, Module::ATTR_TYPE);

      try
      {
        const auto total = info->Duration() * Iterations;
        BenchmarkSoundReceiver receiver;
        const auto renderer = holder->CreateRenderer(Sounder.GetSamplerate(), props);
        const auto state = renderer->GetState();
        const Time::Timer timer;
        for (unsigned i = 0; i != Iterations; ++i)
        {
          renderer->SetPosition({});
          while (0 == state->LoopCount())
          {
            auto data = renderer->Render();
            if (data.empty())
            {
              break;
            }
            else
            {
              receiver.ApplyData(std::move(data));
            }
          }
        }
        const auto real = timer.Elapsed<>();
        const auto relSpeed = total.Divide<double>(real);
        Display.Message("x{2:.2f}\t({1})\t{0}\t[0x{3:08x}]\t{{{4}..{5}, {6}}}", path, type, relSpeed,
                        receiver.GetHash(), receiver.GetMinSample(), receiver.GetMaxSample(),
                        receiver.GetTotalSamples());
      }
      catch (const std::exception& e)
      {
        BenchmarkFail(path, type, e.what());
      }
      catch (const Error& e)
      {
        BenchmarkFail(path, type, e.ToString());
      }
      catch (...)
      {
        BenchmarkFail(path, type, "Unknown error");
      }
    }

    void ProcessUnknownData(StringView path, StringView container, Binary::Data::Ptr data) override
    {
      if (DumpUnknownData)
      {
        Display.Message("Unknown\t({1})\t{0}\t[{2}]", path, container, data->Size());
      }
    }

  private:
    void BenchmarkFail(StringView path, StringView type, StringView msg) const
    {
      Display.Message("Fail\t({1})\t{0}\t[{2}]", path, type, msg);
    }

    class BenchmarkSoundReceiver
    {
    public:
      void ApplyData(Sound::Chunk data)
      {
        Crc32 = Binary::Crc32(data, Crc32);
        TotalSamples += data.size();
        for (const auto smp : data)
        {
          MinMax(smp.Left());
          MinMax(smp.Right());
        }
      }

      uint32_t GetHash() const
      {
        return Crc32;
      }

      uint64_t GetTotalSamples() const
      {
        return TotalSamples;
      }

      Sound::Sample::Type GetMinSample() const
      {
        return MinSample;
      }

      Sound::Sample::Type GetMaxSample() const
      {
        return MaxSample;
      }

    private:
      void MinMax(Sound::Sample::Type val)
      {
        MinSample = std::min(MinSample, val);
        MaxSample = std::max(MaxSample, val);
      }

    private:
      uint32_t Crc32 = 0;
      uint64_t TotalSamples = 0;
      Sound::Sample::Type MinSample = Sound::Sample::MAX;
      Sound::Sample::Type MaxSample = Sound::Sample::MIN;
    };

  private:
    const unsigned Iterations;
    const bool DumpUnknownData;
    SoundComponent& Sounder;
    DisplayComponent& Display;
  };

  const auto NO_BENCHMARK = ~0u;

  class CLIApplication
    : public Platform::Application
    , private OnItemCallback
  {
  public:
    CLIApplication()
      : ConfigParams(Parameters::Container::Create())
      , Informer(InformationComponent::Create())
      , Sourcer(SourceComponent::Create(ConfigParams))
      , Sounder(SoundComponent::Create(ConfigParams))
      , Display(DisplayComponent::Create())
      , BenchmarkIterations(NO_BENCHMARK)
    {}

    int Run(Strings::Array args) override
    {
      try
      {
        if (ProcessOptions(std::move(args)) || Informer->Process(*Sounder))
        {
          return 0;
        }

        Sourcer->Initialize();

        if (!ConvertParams.empty())
        {
          const Parameters::Container::Ptr cnvParams = Parameters::Container::Create();
          ParseParametersString("", ConvertParams, *cnvParams);
          const Parameters::Accessor::Ptr mergedParams = Parameters::CreateMergedAccessor(cnvParams, ConfigParams);
          Convertor cnv(*mergedParams, *Display);
          Sourcer->ProcessItems(cnv);
        }
        else if (NO_BENCHMARK != BenchmarkIterations)
        {
          Benchmark benchmark(BenchmarkIterations, DumpUnknownData, *Sounder, *Display);
          Sourcer->ProcessItems(benchmark);
        }
        else
        {
          Sounder->Initialize();
          Sourcer->ProcessItems(*this);
        }
      }
      catch (const CancelError&)
      {}
      return 0;
    }

  private:
    bool ProcessOptions(Strings::Array args)
    {
      try
      {
        using namespace boost::program_options;

        String configFile;
        options_description options(
            Strings::Format("Usage:\n{0} <Informational keys>\n{0} <Options> <files>...\n\nKeys", args[0]));
        const auto HELP_KEY = "help"s;
        const auto VERSION_KEY = "version"s;
        const auto ABOUT_KEY = "about"s;
        const auto CONFIG_KEY = "config"s;
        const auto CONVERT_KEY = "convert"s;
        {
          auto opt = options.add_options();
          opt(HELP_KEY.c_str(), "show this message");
          opt(VERSION_KEY.c_str(), "show version");
          opt(ABOUT_KEY.c_str(), "show information about program");
          opt(CONFIG_KEY.c_str(), value<String>(&configFile), "configuration file");
          opt(CONVERT_KEY.c_str(), value<String>(&ConvertParams),
              "Perform conversion instead of playback.\n"
              "Parameter is map with the next mandatory parameters:\n"
              " mode - specify conversion mode. Currently supported are: raw,psg,zx50,txt,aydump,fym\n"
              " filename - filename template with any module's attributes\n"
              ".");
          opt("benchmark", value<uint_t>(&BenchmarkIterations),
              "Switch on benchmark mode with specified iterations count.\n");
          opt("dump-unknown-data", bool_switch(&DumpUnknownData), "Also report about unprocessed data regions.\n");
        }
        options.add(Informer->GetOptionsDescription());
        options.add(Sourcer->GetOptionsDescription());
        options.add(Sounder->GetOptionsDescription());
        options.add(Display->GetOptionsDescription());
        // add positional parameters for input
        positional_options_description inputPositional;
        inputPositional.add("file", -1);

        // cli options
        options_description cliOptions("Other options");
        cliOptions.add_options()("seekstep", value<uint_t>(&SeekStep), "seeking step in percents");
        options.add(cliOptions);

        variables_map vars;
        // args should not contain program name
        args.erase(args.begin());
        store(command_line_parser(args).options(options).positional(inputPositional).run(), vars);
        notify(vars);
        if (vars.count(HELP_KEY))
        {
          StdOut << options << std::endl;
          return true;
        }
        else if (vars.count(VERSION_KEY))
        {
          StdOut << Platform::Version::GetProgramVersionString() << std::endl;
          return true;
        }
        else if (vars.count(ABOUT_KEY))
        {
          StdOut << "This program is the part of ZXTune project.\n"
                    "Visit https://zxtune.bitbucket.io/ for support or new versions\n"
                    "For additional information contact vitamin.caig@gmail.com\n"
                    "(C) Vitamin/CAIG\n"
                 << std::endl;
          return true;
        }
        ParseConfigFile(configFile, *ConfigParams);
        Sourcer->ParseParameters();
        Sounder->ParseParameters();
        return false;
      }
      catch (const std::exception& e)
      {
        throw MakeFormattedError(THIS_LINE, "Error: {}", e.what());
      }
    }

    void ProcessItem(Binary::Data::Ptr /*data*/, Module::Holder::Ptr holder) override
    {
      const Sound::Backend::Ptr backend = Sounder->CreateBackend(holder);
      const Sound::PlaybackControl::Ptr control = backend->GetPlaybackControl();

      const Module::Information::Ptr info = holder->GetModuleInformation();
      const auto seekStep = Time::Milliseconds(info->Duration().Get() * SeekStep / 100);
      control->Play();

      Display->SetModule(holder, backend);

      const Sound::Gain::Type minVol(0);
      const Sound::Gain::Type maxVol(1);
      const Sound::Gain::Type volStep(5, 100);
      Sound::Gain::Type curVolume;
      const Sound::VolumeControl::Ptr volCtrl = backend->GetVolumeControl();
      if (volCtrl)
      {
        const Sound::Gain allVolume = volCtrl->GetVolume();
        curVolume = (allVolume.Left() + allVolume.Right()) / 2;
      }

      for (;;)
      {
        Sound::PlaybackControl::State state = control->GetCurrentState();

        const auto pos = Display->BeginFrame(state);

        const auto START = Time::AtMillisecond();
        if (const uint_t key = Console::Self().GetPressedKey())
        {
          switch (key)
          {
          case Console::INPUT_KEY_CANCEL:
          case 'Q':
            throw CancelError();
          case Console::INPUT_KEY_LEFT:
            control->SetPosition(pos < (START + seekStep) ? START : Time::AtMillisecond(pos.Get() - seekStep.Get()));
            break;
          case Console::INPUT_KEY_RIGHT:
            control->SetPosition(pos + seekStep);
            break;
          case Console::INPUT_KEY_DOWN:
            if (volCtrl)
            {
              curVolume -= volStep;
              curVolume = std::max(minVol, curVolume);
              const Sound::Gain allVol(curVolume, curVolume);
              volCtrl->SetVolume(allVol);
            }
            break;
          case Console::INPUT_KEY_UP:
            if (volCtrl)
            {
              curVolume += volStep;
              curVolume = std::min(maxVol, curVolume);
              const Sound::Gain allVol(curVolume, curVolume);
              volCtrl->SetVolume(allVol);
            }
            break;
          case Console::INPUT_KEY_ENTER:
            if (Sound::PlaybackControl::STARTED == state)
            {
              control->Pause();
              Console::Self().WaitForKeyRelease();
            }
            else
            {
              Console::Self().WaitForKeyRelease();
              control->Play();
            }
            break;
          case ' ':
            control->Stop();
            state = Sound::PlaybackControl::STOPPED;
            Console::Self().WaitForKeyRelease();
            break;
          }
        }

        if (Sound::PlaybackControl::STOPPED == state)
        {
          break;
        }
      }
    }

  private:
    const Parameters::Container::Ptr ConfigParams;
    String ConvertParams;
    std::unique_ptr<InformationComponent> Informer;
    std::unique_ptr<SourceComponent> Sourcer;
    std::unique_ptr<SoundComponent> Sounder;
    std::unique_ptr<DisplayComponent> Display;
    uint_t SeekStep = 10;
    uint_t BenchmarkIterations;
    bool DumpUnknownData = false;
  };
}  // namespace

namespace Platform
{
  namespace Version
  {
    const StringView PROGRAM_NAME = "zxtune123"sv;
  }

  std::unique_ptr<Application> Application::Create()
  {
    return std::unique_ptr<Application>(new CLIApplication());
  }
}  // namespace Platform
