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
#include "console.h"
#include "display.h"
#include "information.h"
#include "sound.h"
#include "source.h"
#include <apps/base/app.h>
#include <apps/base/parsing.h>
#include <apps/version/api.h>
//common includes
#include <error_tools.h>
#include <progress_callback.h>
//library includes
#include <async/data_receiver.h>
#include <async/src/event.h>
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <core/conversion/api.h>
#include <io/api.h>
#include <io/template.h>
#include <parameters/merged_accessor.h>
#include <parameters/template.h>
#include <sound/sound_parameters.h>
#include <time/duration.h>
//std includes
#include <algorithm>
#include <cctype>
#include <functional>
#include <limits>
#include <numeric>
//boost includes
#include <boost/bind.hpp>
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

  class ConvertEndpoint : public DataReceiver<Module::Holder::Ptr>
  {
  public:
    ConvertEndpoint(DisplayComponent& display, const Parameters::Accessor& params, std::auto_ptr<Module::Conversion::Parameter> param, Strings::Template::Ptr templ)
      : Display(display)
      , Params(params)
      , ConversionParameter(param)
      , FileNameTemplate(templ)
    {
    }

    virtual void ApplyData(const Module::Holder::Ptr& holder)
    {
      try
      {
        const Parameters::Accessor::Ptr props = holder->GetModuleProperties();
        const String id = GetModuleId(*props);
        if (const Binary::Data::Ptr result = Module::Convert(*holder, *ConversionParameter, props))
        {
          //prepare result filename
          const String& filename = FileNameTemplate->Instantiate(Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource>(*props));
          const Binary::OutputStream::Ptr stream = IO::CreateStream(filename, Params, Log::ProgressCallback::Stub());
          stream->ApplyData(*result);
          Display.Message(Strings::Format(Text::CONVERT_DONE, id, filename));
        }
        else
        {
          Parameters::StringType type;
          props->FindValue(Module::ATTR_TYPE, type);
          Display.Message(Strings::Format(Text::CONVERT_SKIPPED, id, type));
        }
      }
      catch (const Error& e)
      {
        StdOut << e.ToString();
      }
    }

    virtual void Flush()
    {
    }
  private:
    DisplayComponent& Display;
    const Parameters::Accessor& Params;
    const std::auto_ptr<Module::Conversion::Parameter> ConversionParameter;
    const Strings::Template::Ptr FileNameTemplate;
  };


  class Convertor
  {
  public:
    Convertor(const Parameters::Accessor& params, DisplayComponent& display)
      : Pipe(DataReceiver<Module::Holder::Ptr>::CreateStub())
    {
      Parameters::StringType mode;
      if (!params.FindValue(ToStdString(Text::CONVERSION_PARAM_MODE), mode))
      {
        throw Error(THIS_LINE, Text::CONVERT_ERROR_NO_MODE);
      }
      String nameTemplate;
      if (!params.FindValue(ToStdString(Text::CONVERSION_PARAM_FILENAME), nameTemplate))
      {
        throw Error(THIS_LINE, Text::CONVERT_ERROR_NO_FILENAME);
      }
      Parameters::IntType optimization = Module::Conversion::DEFAULT_OPTIMIZATION;
      params.FindValue(ToStdString(Text::CONVERSION_PARAM_OPTIMIZATION), optimization);
      std::auto_ptr<Module::Conversion::Parameter> param;
      if (mode == Text::CONVERSION_MODE_RAW)
      {
        param.reset(new Module::Conversion::RawConvertParam());
      }
      else if (mode == Text::CONVERSION_MODE_PSG)
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
      Strings::Template::Ptr templ = IO::CreateFilenameTemplate(nameTemplate);
      const DataReceiver<Module::Holder::Ptr>::Ptr target(new ConvertEndpoint(display, params, param, templ));
      Pipe = Async::DataReceiver<Module::Holder::Ptr>::Create(1, 1000, target);
    }

    ~Convertor()
    {
      Pipe->Flush();
    }

    void ProcessItem(Module::Holder::Ptr holder) const
    {
      Pipe->ApplyData(holder);
    }
  private:
    DataReceiver<Module::Holder::Ptr>::Ptr Pipe;
  };

  class AutoTimer
  {
  public:
    AutoTimer()
      : Start(std::clock())
    {
    }

    template<class StampType>
    StampType Elapsed() const
    {
      const Time::Stamp<typename StampType::ValueType, CLOCKS_PER_SEC> local(std::clock() - Start);
      return StampType(local);
    }
  private:
    const std::clock_t Start;
  };

  class FinishPlaybackCallback : public Sound::BackendCallback
  {
  public:
    virtual void OnStart()
    {
      Event.Reset();
    }

    virtual void OnFrame(const Module::TrackState& /*state*/)
    {
    }

    virtual void OnStop()
    {
    }

    virtual void OnPause()
    {
    }

    virtual void OnResume()
    {
    }

    virtual void OnFinish()
    {
      Event.Set(1);
    }

    void Wait()
    {
      Event.Wait(1);
    }
  private:
    Async::Event<uint_t> Event;
  };

  class Benchmark
  {
  public:
    Benchmark(unsigned iterations, SoundComponent& sound, DisplayComponent& display)
      : Iterations(iterations)
      , Sounder(sound)
      , Display(display)
    {
    }

    void ProcessItem(Module::Holder::Ptr holder) const
    {
      const Module::Information::Ptr info = holder->GetModuleInformation();
      const Parameters::Accessor::Ptr props = holder->GetModuleProperties();
      String path, type;
      props->FindValue(Module::ATTR_FULLPATH, path);
      props->FindValue(Module::ATTR_TYPE, type);

      Time::Microseconds total(Sounder.GetFrameDuration().Get() * info->FramesCount() * Iterations);

      FinishPlaybackCallback cb;
      const Sound::Backend::Ptr backend = Sounder.CreateBackend(holder, "null", Sound::BackendCallback::Ptr(&cb, NullDeleter<Sound::BackendCallback>()));
      const Sound::PlaybackControl::Ptr control = backend->GetPlaybackControl();
      const AutoTimer timer;
      for (unsigned i = 0; i != Iterations; ++i)
      {
        control->SetPosition(0);
        control->Play();
        cb.Wait();
      }
      const Time::Microseconds real = timer.Elapsed<Time::Microseconds>();
      const double relSpeed = double(total.Get()) / real.Get();
      Display.Message(Strings::Format(Text::BENCHMARK_RESULT, path, type, relSpeed));
    }
  private:
    const unsigned Iterations;
    SoundComponent& Sounder;
    DisplayComponent& Display;
  };

  class CLIApplication : public Application
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

    virtual int Run(int argc, char* argv[])
    {
      try
      {
        if (ProcessOptions(argc, argv) ||
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
          Sourcer->ProcessItems(boost::bind(&Convertor::ProcessItem, &cnv, _1));
        }
        else if (0 != BenchmarkIterations)
        {
          const Benchmark benchmark(BenchmarkIterations, *Sounder, *Display);
          Sourcer->ProcessItems(boost::bind(&Benchmark::ProcessItem, &benchmark, _1));
        }
        else
        {
          Sounder->Initialize();
          Sourcer->ProcessItems(boost::bind(&CLIApplication::PlayItem, this, _1));
        }
      }
      catch (const CancelError&)
      {
      }
      return 0;
    }
  private:
    bool ProcessOptions(int argc, char* argv[])
    {
      try
      {
        using namespace boost::program_options;

        String configFile;
        options_description options(Strings::Format(Text::USAGE_SECTION, *argv));
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
        store(command_line_parser(argc, argv).options(options).positional(inputPositional).run(), vars);
        notify(vars);
        if (vars.count(Text::HELP_KEY))
        {
          StdOut << options << std::endl;
          return true;
        }
        else if (vars.count(Text::VERSION_KEY))
        {
          StdOut << GetProgramVersionString() << std::endl;
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

    void PlayItem(Module::Holder::Ptr holder)
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
    std::auto_ptr<InformationComponent> Informer;
    std::auto_ptr<SourceComponent> Sourcer;
    std::auto_ptr<SoundComponent> Sounder;
    std::auto_ptr<DisplayComponent> Display;
    uint_t SeekStep;
    uint_t BenchmarkIterations;
  };
}

std::auto_ptr<Application> Application::Create()
{
  return std::auto_ptr<Application>(new CLIApplication());
}
