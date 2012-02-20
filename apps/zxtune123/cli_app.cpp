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
#include <apps/base/error_codes.h>
#include <apps/base/parsing.h>
#include <apps/version/api.h>
//common includes
#include <error_tools.h>
#include <template_parameters.h>
//library includes
#include <async/data_receiver.h>
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <io/fs_tools.h>
#include <io/providers_parameters.h>
#include <sound/sound_parameters.h>
//std includes
#include <algorithm>
#include <cctype>
#include <functional>
#include <fstream>
#include <iostream>
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
  inline void ErrOuter(uint_t /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    StdOut << Error::AttributesToString(loc, code, text);
  }

  String GetModuleId(const Parameters::Accessor& props)
  {
    String res;
    props.FindStringValue(ZXTune::Module::ATTR_FULLPATH, res);
    return res;
  }

  class ModuleFieldsSource : public Parameters::FieldsSourceAdapter<SkipFieldsSource>
  {
  public:
    typedef Parameters::FieldsSourceAdapter<SkipFieldsSource> Parent;
    explicit ModuleFieldsSource(const Parameters::Accessor& params)
      : Parent(params)
    {
    }

    String GetFieldValue(const String& fieldName) const
    {
      return ZXTune::IO::MakePathFromString(Parent::GetFieldValue(fieldName), '_');
    }
  };

  class ConvertEndpoint : public DataReceiver<ZXTune::Module::Holder::Ptr>
  {
  public:
    ConvertEndpoint(DisplayComponent& display, std::auto_ptr<ZXTune::Module::Conversion::Parameter> param, uint_t capMask, StringTemplate::Ptr templ)
      : Display(display)
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
      Dump result;
      ThrowIfError(holder->Convert(*ConversionParameter, props, result));
      //prepare result filename
      const String& filename = FileNameTemplate->Instantiate(ModuleFieldsSource(*props));
      std::ofstream file(filename.c_str(), std::ios::binary);
      file.write(safe_ptr_cast<const char*>(&result[0]), static_cast<std::streamsize>(result.size() * sizeof(result.front())));
      if (!file)
      {
        throw MakeFormattedError(THIS_LINE, CONVERT_PARAMETERS,
          Text::CONVERT_ERROR_WRITE_FILE, filename);
      }
      Display.Message(Strings::Format(Text::CONVERT_DONE, id, filename));
    }

    virtual void Flush()
    {
    }
  private:
    DisplayComponent& Display;
    const std::auto_ptr<ZXTune::Module::Conversion::Parameter> ConversionParameter;
    const uint_t CapabilityMask;
    const StringTemplate::Ptr FileNameTemplate;
  };


  class Convertor
  {
  public:
    Convertor(const Parameters::Accessor& params, DisplayComponent& display)
      : Pipe(DataReceiver<ZXTune::Module::Holder::Ptr>::CreateStub())
    {
      Parameters::StringType mode;
      if (!params.FindStringValue(Text::CONVERSION_PARAM_MODE, mode))
      {
        throw Error(THIS_LINE, CONVERT_PARAMETERS, Text::CONVERT_ERROR_NO_MODE);
      }
      String nameTemplate;
      if (!params.FindStringValue(Text::CONVERSION_PARAM_FILENAME, nameTemplate))
      {
        throw Error(THIS_LINE, CONVERT_PARAMETERS, Text::CONVERT_ERROR_NO_FILENAME);
      }
      std::auto_ptr<ZXTune::Module::Conversion::Parameter> param;
      uint_t mask = 0;
      if (mode == Text::CONVERSION_MODE_RAW)
      {
        param.reset(new ZXTune::Module::Conversion::RawConvertParam());
        mask = ZXTune::CAP_CONV_RAW;
      }
      else if (mode == Text::CONVERSION_MODE_PSG)
      {
        param.reset(new ZXTune::Module::Conversion::PSGConvertParam());
        mask = ZXTune::CAP_CONV_PSG;
      }
      else if (mode == Text::CONVERSION_MODE_ZX50)
      {
        param.reset(new ZXTune::Module::Conversion::ZX50ConvertParam());
        mask = ZXTune::CAP_CONV_ZX50;
      }
      else if (mode == Text::CONVERSION_MODE_TXT)
      {
        param.reset(new ZXTune::Module::Conversion::TXTConvertParam());
        mask = ZXTune::CAP_CONV_TXT;
      }
      else if (mode == Text::CONVERSION_MODE_DEBUGAY)
      {
        param.reset(new ZXTune::Module::Conversion::DebugAYConvertParam());
        mask = ZXTune::CAP_CONV_AYDUMP;
      }
      else if (mode == Text::CONVERSION_MODE_AYDUMP)
      {
        param.reset(new ZXTune::Module::Conversion::AYDumpConvertParam());
        mask = ZXTune::CAP_CONV_AYDUMP;
      }
      else if (mode == Text::CONVERSION_MODE_FYM)
      {
        param.reset(new ZXTune::Module::Conversion::FYMConvertParam());
        mask = ZXTune::CAP_CONV_FYM;
      }
      else
      {
        throw Error(THIS_LINE, CONVERT_PARAMETERS, Text::CONVERT_ERROR_INVALID_MODE);
      }

      const DataReceiver<ZXTune::Module::Holder::Ptr>::Ptr target(new ConvertEndpoint(display, param, mask, StringTemplate::Create(nameTemplate)));
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
          ThrowIfError(ParseParametersString(String(), ConvertParams, *cnvParams));
          Convertor cnv(*cnvParams, *Display);
          Sourcer->ProcessItems(boost::bind(&Convertor::ProcessItem, &cnv, _1));
        }
        else
        {
          Sounder->Initialize();
          Sourcer->ProcessItems(boost::bind(&CLIApplication::PlayItem, this, _1));
        }
      }
      catch (const Error& e)
      {
        if (!e.FindSuberror(ZXTune::Module::ERROR_DETECT_CANCELED))
        {
          e.WalkSuberrors(ErrOuter);
        }
        return -1;
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
          (Text::CONFIG_KEY, boost::program_options::value<String>(&configFile), Text::CONFIG_DESC)
          (Text::CONVERT_KEY, boost::program_options::value<String>(&ConvertParams), Text::CONVERT_DESC)
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
        ThrowIfError(ParseConfigFile(configFile, *ConfigParams));
        Sourcer->ParseParameters();
        Sounder->ParseParameters();
        return false;
      }
      catch (const std::exception& e)
      {
        throw MakeFormattedError(THIS_LINE, UNKNOWN_ERROR, Text::COMMON_ERROR, e.what());
      }
    }

    void PlayItem(ZXTune::Module::Holder::Ptr holder)
    {
      const ZXTune::Sound::Backend::Ptr backend = Sounder->CreateBackend(holder);

      const uint_t frameDuration = Sounder->GetFrameDuration();

      const ZXTune::Module::Information::Ptr info = holder->GetModuleInformation();
      const uint_t seekStepFrames(info->FramesCount() * SeekStep / 100);
      ThrowIfError(backend->Play());

      Display->SetModule(backend, static_cast<uint_t>(frameDuration));

      ZXTune::Sound::Gain curVolume = ZXTune::Sound::Gain();
      ZXTune::Sound::MultiGain allVolume;
      ZXTune::Sound::VolumeControl::Ptr volCtrl(backend->GetVolumeControl());
      const bool noVolume = volCtrl.get() == 0;
      if (!noVolume)
      {
        ThrowIfError(volCtrl->GetVolume(allVolume));
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
            throw Error(THIS_LINE, ZXTune::Module::ERROR_DETECT_CANCELED);
          case Console::INPUT_KEY_LEFT:
            ThrowIfError(backend->SetPosition(curFrame < seekStepFrames ? 0 : curFrame - seekStepFrames));
            break;
          case Console::INPUT_KEY_RIGHT:
            ThrowIfError(backend->SetPosition(curFrame + seekStepFrames));
            break;
          case Console::INPUT_KEY_DOWN:
            if (!noVolume)
            {
              curVolume = std::max(0.0, curVolume - 0.05);
              ZXTune::Sound::MultiGain allVol;
              allVol.assign(curVolume);
              ThrowIfError(volCtrl->SetVolume(allVol));
            }
            break;
          case Console::INPUT_KEY_UP:
            if (!noVolume)
            {
              curVolume = std::min(1.0, curVolume + 0.05);
              ZXTune::Sound::MultiGain allVol;
              allVol.assign(curVolume);
              ThrowIfError(volCtrl->SetVolume(allVol));
            }
            break;
          case Console::INPUT_KEY_ENTER:
            if (ZXTune::Sound::Backend::STARTED == state)
            {
              ThrowIfError(backend->Pause());
              Console::Self().WaitForKeyRelease();
            }
            else
            {
              Console::Self().WaitForKeyRelease();
              ThrowIfError(backend->Play());
            }
            break;
          case ' ':
            ThrowIfError(backend->Stop());
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
  };
}

const std::string THIS_MODULE("zxtune123");

std::auto_ptr<Application> Application::Create()
{
  return std::auto_ptr<Application>(new CLIApplication());
}
