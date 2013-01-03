/*
Abstract:
  CLI application implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune123 application based on zxtune library
*/

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
#include <template_parameters.h>
//library includes
#include <async/data_receiver.h>
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <io/api.h>
#include <io/template.h>
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
    props.FindValue(ZXTune::Module::ATTR_FULLPATH, res);
    return res;
  }

  class ConvertEndpoint : public DataReceiver<ZXTune::Module::Holder::Ptr>
  {
  public:
    ConvertEndpoint(DisplayComponent& display, const Parameters::Accessor& params, std::auto_ptr<ZXTune::Module::Conversion::Parameter> param, uint_t capMask, Strings::Template::Ptr templ)
      : Display(display)
      , Params(params)
      , ConversionParameter(param)
      , CapabilityMask(capMask)
      , FileNameTemplate(templ)
    {
    }

    virtual void ApplyData(const ZXTune::Module::Holder::Ptr& holder)
    {
      const Parameters::Accessor::Ptr props = holder->GetModuleProperties();
      const String id = GetModuleId(*props);
      {
        const ZXTune::Plugin::Ptr plugin = holder->GetPlugin();
        if (!(plugin->Capabilities() & CapabilityMask))
        {
          Display.Message(Strings::Format(Text::CONVERT_SKIPPED, id, plugin->Id()));
          return;
        }
      }
      const Binary::Data::Ptr result = holder->Convert(*ConversionParameter, props);
      //prepare result filename
      const String& filename = FileNameTemplate->Instantiate(Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource>(*props));
      const Binary::OutputStream::Ptr stream = IO::CreateStream(filename, Params, Log::ProgressCallback::Stub());
      stream->ApplyData(*result);
      Display.Message(Strings::Format(Text::CONVERT_DONE, id, filename));
    }

    virtual void Flush()
    {
    }
  private:
    DisplayComponent& Display;
    const Parameters::Accessor& Params;
    const std::auto_ptr<ZXTune::Module::Conversion::Parameter> ConversionParameter;
    const uint_t CapabilityMask;
    const Strings::Template::Ptr FileNameTemplate;
  };


  class Convertor
  {
  public:
    Convertor(const Parameters::Accessor& params, DisplayComponent& display)
      : Pipe(DataReceiver<ZXTune::Module::Holder::Ptr>::CreateStub())
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
      Parameters::IntType optimization = ZXTune::Module::Conversion::DEFAULT_OPTIMIZATION;
      params.FindValue(ToStdString(Text::CONVERSION_PARAM_OPTIMIZATION), optimization);
      std::auto_ptr<ZXTune::Module::Conversion::Parameter> param;
      uint_t mask = 0;
      if (mode == Text::CONVERSION_MODE_RAW)
      {
        param.reset(new ZXTune::Module::Conversion::RawConvertParam());
        mask = ZXTune::CAP_CONV_RAW;
      }
      else if (mode == Text::CONVERSION_MODE_PSG)
      {
        param.reset(new ZXTune::Module::Conversion::PSGConvertParam(optimization));
        mask = ZXTune::CAP_CONV_PSG;
      }
      else if (mode == Text::CONVERSION_MODE_ZX50)
      {
        param.reset(new ZXTune::Module::Conversion::ZX50ConvertParam(optimization));
        mask = ZXTune::CAP_CONV_ZX50;
      }
      else if (mode == Text::CONVERSION_MODE_TXT)
      {
        param.reset(new ZXTune::Module::Conversion::TXTConvertParam());
        mask = ZXTune::CAP_CONV_TXT;
      }
      else if (mode == Text::CONVERSION_MODE_DEBUGAY)
      {
        param.reset(new ZXTune::Module::Conversion::DebugAYConvertParam(optimization));
        mask = ZXTune::CAP_CONV_AYDUMP;
      }
      else if (mode == Text::CONVERSION_MODE_AYDUMP)
      {
        param.reset(new ZXTune::Module::Conversion::AYDumpConvertParam(optimization));
        mask = ZXTune::CAP_CONV_AYDUMP;
      }
      else if (mode == Text::CONVERSION_MODE_FYM)
      {
        param.reset(new ZXTune::Module::Conversion::FYMConvertParam(optimization));
        mask = ZXTune::CAP_CONV_FYM;
      }
      else
      {
        throw Error(THIS_LINE, Text::CONVERT_ERROR_INVALID_MODE);
      }
      Strings::Template::Ptr templ = IO::CreateFilenameTemplate(nameTemplate);
      const DataReceiver<ZXTune::Module::Holder::Ptr>::Ptr target(new ConvertEndpoint(display, params, param, mask, templ));
      Pipe = Async::DataReceiver<ZXTune::Module::Holder::Ptr>::Create(1, 1000, target);
    }

    ~Convertor()
    {
      Pipe->Flush();
    }

    void ProcessItem(ZXTune::Module::Holder::Ptr holder) const
    {
      Pipe->ApplyData(holder);
    }
  private:
    DataReceiver<ZXTune::Module::Holder::Ptr>::Ptr Pipe;
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

  class Benchmark
  {
  public:
    Benchmark(unsigned iterations, SoundComponent& sound, DisplayComponent& display)
      : Iterations(iterations)
      , Sounder(sound)
      , Display(display)
    {
    }

    void ProcessItem(ZXTune::Module::Holder::Ptr holder) const
    {
      const ZXTune::Module::Information::Ptr info = holder->GetModuleInformation();
      const Parameters::Accessor::Ptr props = holder->GetModuleProperties();
      String path, type;
      props->FindValue(ZXTune::Module::ATTR_FULLPATH, path);
      props->FindValue(ZXTune::Module::ATTR_TYPE, type);

      Time::Microseconds total(Sounder.GetFrameDuration().Get() * info->FramesCount() * Iterations);

      const ZXTune::Sound::Backend::Ptr backend = Sounder.CreateBackend(holder, "null");
      const Async::Signals::Collector::Ptr signals = backend->CreateSignalsCollector(ZXTune::Sound::Backend::MODULE_FINISH);
      const AutoTimer timer;
      for (unsigned i = 0; i != Iterations; ++i)
      {
        backend->SetPosition(0);
        backend->Play();
        while (0 == signals->WaitForSignals(100)) {}
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
            Informer->Process())
        {
          return 0;
        }

        Sourcer->Initialize();

        if (!ConvertParams.empty())
        {
          const Parameters::Container::Ptr cnvParams = Parameters::Container::Create();
          ThrowIfError(ParseParametersString(Parameters::NameType(), ConvertParams, *cnvParams));
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
        String providersOptions, coreOptions;
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
        ThrowIfError(ParseConfigFile(configFile, *ConfigParams));
        Sourcer->ParseParameters();
        Sounder->ParseParameters();
        return false;
      }
      catch (const std::exception& e)
      {
        throw MakeFormattedError(THIS_LINE, Text::COMMON_ERROR, e.what());
      }
    }

    void PlayItem(ZXTune::Module::Holder::Ptr holder)
    {
      const ZXTune::Sound::Backend::Ptr backend = Sounder->CreateBackend(holder);

      const Time::Microseconds frameDuration = Sounder->GetFrameDuration();

      const ZXTune::Module::Information::Ptr info = holder->GetModuleInformation();
      const uint_t seekStepFrames(info->FramesCount() * SeekStep / 100);
      backend->Play();

      Display->SetModule(holder, backend, frameDuration);

      ZXTune::Sound::Gain curVolume = ZXTune::Sound::Gain();
      const ZXTune::Sound::VolumeControl::Ptr volCtrl = backend->GetVolumeControl();
      if (volCtrl)
      {
        const ZXTune::Sound::MultiGain allVolume = volCtrl->GetVolume();
        curVolume = std::accumulate(allVolume.begin(), allVolume.end(), curVolume) / allVolume.size();
      }

      for (;;)
      {
        ZXTune::Sound::Backend::State state = backend->GetCurrentState();

        const uint_t curFrame = Display->BeginFrame(state);

        if (const uint_t key = Console::Self().GetPressedKey())
        {
          switch (key)
          {
          case Console::INPUT_KEY_CANCEL:
          case 'Q':
            throw CancelError();
          case Console::INPUT_KEY_LEFT:
            backend->SetPosition(curFrame < seekStepFrames ? 0 : curFrame - seekStepFrames);
            break;
          case Console::INPUT_KEY_RIGHT:
            backend->SetPosition(curFrame + seekStepFrames);
            break;
          case Console::INPUT_KEY_DOWN:
            if (volCtrl)
            {
              curVolume = std::max(0.0, curVolume - 0.05);
              ZXTune::Sound::MultiGain allVol;
              allVol.assign(curVolume);
              volCtrl->SetVolume(allVol);
            }
            break;
          case Console::INPUT_KEY_UP:
            if (volCtrl)
            {
              curVolume = std::min(1.0, curVolume + 0.05);
              ZXTune::Sound::MultiGain allVol;
              allVol.assign(curVolume);
              volCtrl->SetVolume(allVol);
            }
            break;
          case Console::INPUT_KEY_ENTER:
            if (ZXTune::Sound::Backend::STARTED == state)
            {
              backend->Pause();
              Console::Self().WaitForKeyRelease();
            }
            else
            {
              Console::Self().WaitForKeyRelease();
              backend->Play();
            }
            break;
          case ' ':
            backend->Stop();
            state = ZXTune::Sound::Backend::STOPPED;
            Console::Self().WaitForKeyRelease();
            break;
          }
        }

        if (ZXTune::Sound::Backend::STOPPED == state)
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
