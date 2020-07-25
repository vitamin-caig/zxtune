/**
* 
* @file
*
* @brief CLI application implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "config.h"
#include "console.h"
#include "display.h"
#include "information.h"
#include "sound.h"
#include "source.h"
//common includes
#include <error_tools.h>
#include <progress_callback.h>
//library includes
#include <async/data_receiver.h>
#include <async/src/event.h>
#include <binary/container_factories.h>
#include <binary/crc.h>
#include <core/core_parameters.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <io/api.h>
#include <io/template.h>
#include <module/attributes.h>
#include <module/conversion/api.h>
#include <module/conversion/types.h>
#include <parameters/merged_accessor.h>
#include <parameters/template.h>
#include <platform/application.h>
#include <platform/version/api.h>
#include <sound/sound_parameters.h>
#include <time/duration.h>
#include <time/timer.h>
//std includes
#include <algorithm>
#include <cctype>
#include <functional>
#include <limits>
#include <numeric>
//boost includes
#include <boost/program_options.hpp>
//text includes
#include "text/text.h"

#define FILE_TAG 81C76E7D

namespace
{
  String GetModuleId(const Parameters::Accessor& props)
  {
    String res;
    props.FindValue(Module::ATTR_FULLPATH, res);
    return res;
  }
  
  String GetFilenameTemplate(const Parameters::Accessor& params)
  {
    String nameTemplate;
    if (!params.FindValue(ToStdString(Text::CONVERSION_PARAM_FILENAME), nameTemplate))
    {
      throw Error(THIS_LINE, Text::CONVERT_ERROR_NO_FILENAME);
    }
    return nameTemplate;
  }
  
  struct HolderAndData
  {
    Module::Holder::Ptr Holder;
    Binary::Data::Ptr Data;
    
    typedef DataReceiver<HolderAndData> Receiver;
  };

  class SaveEndpoint : public HolderAndData::Receiver
  {
  public:
    SaveEndpoint(DisplayComponent& display, const Parameters::Accessor& params)
      : Display(display)
      , Params(params)
      , FileNameTemplate(IO::CreateFilenameTemplate(GetFilenameTemplate(params)))
    {
    }

    void ApplyData(HolderAndData data) override
    {
      try
      {
        const Parameters::Accessor::Ptr props = data.Holder->GetModuleProperties();
        const String& id = GetModuleId(*props);
        const String& filename = FileNameTemplate->Instantiate(Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource>(*props));
        const Binary::OutputStream::Ptr stream = IO::CreateStream(filename, Params, Log::ProgressCallback::Stub());
        stream->ApplyData(*data.Data);
        Display.Message(Strings::Format(Text::CONVERT_DONE, id, filename));
      }
      catch (const Error& e)
      {
        StdOut << e.ToString();
      }
    }

    void Flush() override
    {
    }
  private:
    DisplayComponent& Display;
    const Parameters::Accessor& Params;
    const Strings::Template::Ptr FileNameTemplate;
  };
  
  std::unique_ptr<Module::Conversion::Parameter> CreateConversionParameters(const String& mode, const Parameters::Accessor& modeParams)
  {
    Parameters::IntType optimization = Module::Conversion::DEFAULT_OPTIMIZATION;
    modeParams.FindValue(ToStdString(Text::CONVERSION_PARAM_OPTIMIZATION), optimization);
    std::unique_ptr<Module::Conversion::Parameter> param;
    if (mode == Text::CONVERSION_MODE_PSG)
    {
      param.reset(new Module::Conversion::PSGConvertParam(optimization));
    }
    else if (mode == Text::CONVERSION_MODE_ZX50)
    {
      param.reset(new Module::Conversion::ZX50ConvertParam(optimization));
    }
    else if (mode == Text::CONVERSION_MODE_TXT)
    {
      param.reset(new Module::Conversion::TXTConvertParam());
    }
    else if (mode == Text::CONVERSION_MODE_DEBUGAY)
    {
      param.reset(new Module::Conversion::DebugAYConvertParam(optimization));
    }
    else if (mode == Text::CONVERSION_MODE_AYDUMP)
    {
      param.reset(new Module::Conversion::AYDumpConvertParam(optimization));
    }
    else if (mode == Text::CONVERSION_MODE_FYM)
    {
      param.reset(new Module::Conversion::FYMConvertParam(optimization));
    }
    else
    {
      throw Error(THIS_LINE, Text::CONVERT_ERROR_INVALID_MODE);
    }
    return param;
  }
  
  class TruncateDataEndpoint : public HolderAndData::Receiver
  {
  public:
    explicit TruncateDataEndpoint(Ptr saver)
      : Saver(std::move(saver))
    {
    }

    void ApplyData(HolderAndData data) override
    {
      const Parameters::IntType availSize = data.Data->Size();
      Parameters::IntType usedSize = 0;
      data.Holder->GetModuleProperties()->FindValue(Module::ATTR_SIZE, usedSize);
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
    ConvertEndpoint(DisplayComponent& display, const String& mode, const Parameters::Accessor& modeParams, Ptr saver)
      : Display(display)
      , ConversionParameter(CreateConversionParameters(mode, modeParams))
      , Saver(std::move(saver))
    {
    }
    
    void ApplyData(HolderAndData data) override
    {
      const Module::Holder::Ptr holder = data.Holder;
      const Parameters::Accessor::Ptr props = holder->GetModuleProperties();
      if (const Binary::Data::Ptr result = Module::Convert(*holder, *ConversionParameter, props))
      {
        Saver->ApplyData({holder, result});
      }
      else
      {
        Parameters::StringType type;
        props->FindValue(Module::ATTR_TYPE, type);
        const String& id = GetModuleId(*props);
        Display.Message(Strings::Format(Text::CONVERT_SKIPPED, id, type));
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
      Parameters::StringType mode;
      if (!params.FindValue(ToStdString(Text::CONVERSION_PARAM_MODE), mode))
      {
        throw Error(THIS_LINE, Text::CONVERT_ERROR_NO_MODE);
      }
      const HolderAndData::Receiver::Ptr saver(new SaveEndpoint(display, params));
      const HolderAndData::Receiver::Ptr target = mode == Text::CONVERSION_MODE_RAW
        ? MakePtr<TruncateDataEndpoint>(saver)
        : MakePtr<ConvertEndpoint>(display, mode, params, saver);
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
    Benchmark(unsigned iterations, SoundComponent& sound, DisplayComponent& display)
      : Iterations(iterations)
      , Sounder(sound)
      , Display(display)
    {
    }

    void ProcessItem(Binary::Data::Ptr /*data*/, Module::Holder::Ptr holder) override
    {
      const Module::Information::Ptr info = holder->GetModuleInformation();
      const Parameters::Accessor::Ptr props = holder->GetModuleProperties();
      String path, type;
      props->FindValue(Module::ATTR_FULLPATH, path);
      props->FindValue(Module::ATTR_TYPE, type);

      try
      {
        const auto total = Sounder.GetFrameDuration() * info->FramesCount() * Iterations;
        BenchmarkSoundReceiver receiver;
        const auto renderer = holder->CreateRenderer(holder->GetModuleProperties(), MakeSingletonPointer(receiver));
        const Time::Timer timer;
        for (unsigned i = 0; i != Iterations; ++i)
        {
          renderer->SetPosition(0);
          while (renderer->RenderFrame()) {}
        }
        const auto real = timer.Elapsed<>();
        const auto relSpeed = total.Divide<double>(real);
        Display.Message(Strings::Format(Text::BENCHMARK_RESULT, path, type, relSpeed,
          receiver.GetHash(), receiver.GetMinSample(), receiver.GetMaxSample()));
      }
      catch (const std::exception& e)
      {
        Display.Message(Strings::Format(Text::BENCHMARK_FAIL, path, type, e.what()));
      }
      catch (const Error& e)
      {
        Display.Message(Strings::Format(Text::BENCHMARK_FAIL, path, type, e.ToString()));
      }
      catch (...)
      {
        Display.Message(Strings::Format(Text::BENCHMARK_FAIL, path, type, "Unknown error"));
      }
    }
  private:
    class BenchmarkSoundReceiver : public Sound::Receiver
    {
    public:
      void ApplyData(Sound::Chunk data) override
      {
        Crc32 = Binary::Crc32(data, Crc32);
        for (const auto smp : data)
        {
          MinMax(smp.Left());
          MinMax(smp.Right());
        }
      }

      void Flush() override
      {
      }

      uint32_t GetHash() const
      {
        return Crc32;
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
      Sound::Sample::Type MinSample = Sound::Sample::MAX;
      Sound::Sample::Type MaxSample = Sound::Sample::MIN;
    };
  private:
    const unsigned Iterations;
    SoundComponent& Sounder;
    DisplayComponent& Display;
  };

  class CLIApplication : public Platform::Application
                       , private OnItemCallback
  {
  public:
    CLIApplication()
      : ConfigParams(Parameters::Container::Create())
      , Informer(InformationComponent::Create())
      , Sourcer(SourceComponent::Create(ConfigParams))
      , Sounder(SoundComponent::Create(ConfigParams))
      , Display(DisplayComponent::Create())
      , SeekStep(10)
      , BenchmarkIterations(0)
    {
    }

    int Run(Strings::Array args) override
    {
      try
      {
        if (ProcessOptions(std::move(args)) ||
            Informer->Process(*Sounder))
        {
          return 0;
        }

        Sourcer->Initialize();

        if (!ConvertParams.empty())
        {
          const Parameters::Container::Ptr cnvParams = Parameters::Container::Create();
          ParseParametersString(Parameters::NameType(), ConvertParams, *cnvParams);
          const Parameters::Accessor::Ptr mergedParams = Parameters::CreateMergedAccessor(cnvParams, ConfigParams);
          Convertor cnv(*mergedParams, *Display);
          Sourcer->ProcessItems(cnv);
        }
        else if (0 != BenchmarkIterations)
        {
          Benchmark benchmark(BenchmarkIterations, *Sounder, *Display);
          Sourcer->ProcessItems(benchmark);
        }
        else
        {
          Sounder->Initialize();
          Sourcer->ProcessItems(*this);
        }
      }
      catch (const CancelError&)
      {
      }
      return 0;
    }
  private:
    bool ProcessOptions(Strings::Array args)
    {
      try
      {
        using namespace boost::program_options;

        String configFile;
        options_description options(Strings::Format(Text::USAGE_SECTION, args[0]));
        options.add_options()
          (Text::HELP_KEY, Text::HELP_DESC)
          (Text::VERSION_KEY, Text::VERSION_DESC)
          (Text::ABOUT_KEY, Text::ABOUT_DESC)
          (Text::CONFIG_KEY, boost::program_options::value<String>(&configFile), Text::CONFIG_DESC)
          (Text::CONVERT_KEY, boost::program_options::value<String>(&ConvertParams), Text::CONVERT_DESC)
          (Text::BENCHMARK_KEY, boost::program_options::value<uint_t>(&BenchmarkIterations), Text::BENCHMARK_DESC)
        ;

        options.add(Informer->GetOptionsDescription());
        options.add(Sourcer->GetOptionsDescription());
        options.add(Sounder->GetOptionsDescription());
        options.add(Display->GetOptionsDescription());
        //add positional parameters for input
        positional_options_description inputPositional;
        inputPositional.add(Text::INPUT_FILE_KEY, -1);

        //cli options
        options_description cliOptions(Text::CLI_SECTION);
        cliOptions.add_options()
          (Text::SEEKSTEP_KEY, value<uint_t>(&SeekStep), Text::SEEKSTEP_DESC)
        ;
        options.add(cliOptions);

        variables_map vars;
        //args should not contain program name
        args.erase(args.begin());
        store(command_line_parser(args).options(options).positional(inputPositional).run(), vars);
        notify(vars);
        if (vars.count(Text::HELP_KEY))
        {
          StdOut << options << std::endl;
          return true;
        }
        else if (vars.count(Text::VERSION_KEY))
        {
          StdOut << Platform::Version::GetProgramVersionString() << std::endl;
          return true;
        }
        else if (vars.count(Text::ABOUT_KEY))
        {
          StdOut << Text::ABOUT_SECTION << std::endl;
          return true;
        }
        ParseConfigFile(configFile, *ConfigParams);
        Sourcer->ParseParameters();
        Sounder->ParseParameters();
        return false;
      }
      catch (const std::exception& e)
      {
        throw MakeFormattedError(THIS_LINE, Text::COMMON_ERROR, e.what());
      }
    }

    void ProcessItem(Binary::Data::Ptr /*data*/, Module::Holder::Ptr holder) override
    {
      const Sound::Backend::Ptr backend = Sounder->CreateBackend(holder);
      const Sound::PlaybackControl::Ptr control = backend->GetPlaybackControl();

      const Time::Microseconds frameDuration = Sounder->GetFrameDuration();

      const Module::Information::Ptr info = holder->GetModuleInformation();
      const uint_t seekStepFrames(info->FramesCount() * SeekStep / 100);
      control->Play();

      Display->SetModule(holder, backend, frameDuration);

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

        const uint_t curFrame = Display->BeginFrame(state);

        if (const uint_t key = Console::Self().GetPressedKey())
        {
          switch (key)
          {
          case Console::INPUT_KEY_CANCEL:
          case 'Q':
            throw CancelError();
          case Console::INPUT_KEY_LEFT:
            control->SetPosition(curFrame < seekStepFrames ? 0 : curFrame - seekStepFrames);
            break;
          case Console::INPUT_KEY_RIGHT:
            control->SetPosition(curFrame + seekStepFrames);
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
        Display->EndFrame();
      }
    }
  private:
    const Parameters::Container::Ptr ConfigParams;
    String ConvertParams;
    std::unique_ptr<InformationComponent> Informer;
    std::unique_ptr<SourceComponent> Sourcer;
    std::unique_ptr<SoundComponent> Sounder;
    std::unique_ptr<DisplayComponent> Display;
    uint_t SeekStep;
    uint_t BenchmarkIterations;
  };
}

namespace Platform
{
  std::unique_ptr<Application> Application::Create()
  {
    return std::unique_ptr<Application>(new CLIApplication());
  }
}
