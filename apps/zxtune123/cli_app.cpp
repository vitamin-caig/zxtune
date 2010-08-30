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
//common includes
#include <error_tools.h>
#include <template.h>
//library includes
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

// version definition-related
#ifndef ZXTUNE_VERSION
#define ZXTUNE_VERSION develop
#endif

#define STR(a) #a

#define VERSION_STRING(a) STR(a)

namespace
{
  inline void ErrOuter(uint_t /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    StdOut << Error::AttributesToString(loc, code, text);
  }

  class Convertor
  {
  public:
    Convertor(const String& paramsStr, DisplayComponent& display)
      : Display(display)
    {
      assert(!paramsStr.empty());
      Parameters::Map paramsMap;
      ThrowIfError(ParseParametersString(String(), paramsStr, paramsMap));
      Parameters::StringType mode;
      if (!Parameters::FindByName(paramsMap, Text::CONVERSION_PARAM_MODE, mode))
      {
        throw Error(THIS_LINE, CONVERT_PARAMETERS, Text::CONVERT_ERROR_NO_MODE);
      }
      if (!Parameters::FindByName(paramsMap, Text::CONVERSION_PARAM_FILENAME, NameTemplate))
      {
        throw Error(THIS_LINE, CONVERT_PARAMETERS, Text::CONVERT_ERROR_NO_FILENAME);
      }
      if (mode == Text::CONVERSION_MODE_RAW)
      {
        ConversionParameter.reset(new ZXTune::Module::Conversion::RawConvertParam());
        CapabilityMask = ZXTune::CAP_CONV_RAW;
      }
      else if (mode == Text::CONVERSION_MODE_PSG)
      {
        ConversionParameter.reset(new ZXTune::Module::Conversion::PSGConvertParam());
        CapabilityMask = ZXTune::CAP_CONV_PSG;
      }
      else if (mode == Text::CONVERSION_MODE_ZX50)
      {
        ConversionParameter.reset(new ZXTune::Module::Conversion::ZX50ConvertParam());
        CapabilityMask = ZXTune::CAP_CONV_ZX50;
      }
      else if (mode == Text::CONVERSION_MODE_TXT)
      {
        ConversionParameter.reset(new ZXTune::Module::Conversion::TXTConvertParam());
        CapabilityMask = ZXTune::CAP_CONV_TXT;
      }
      else
      {
        throw Error(THIS_LINE, CONVERT_PARAMETERS, Text::CONVERT_ERROR_INVALID_MODE);
      }
    }
    bool ProcessItem(const ModuleItem& item) const
    {
      {
        const ZXTune::Plugin::Ptr plugin = item.Module->GetPlugin();
        if (!(plugin->Capabilities() & CapabilityMask))
        {
          Display.Message((Formatter(Text::CONVERT_SKIPPED) % item.Id % plugin->Id()).str());
          return true;
        }
      }
      Dump result;
      ThrowIfError(item.Module->Convert(*ConversionParameter, result));
      //prepare result filename
      StringMap fields;
      {
        StringMap origFields;
        Parameters::ConvertMap(item.Information.Properties, origFields);
        std::transform(origFields.begin(), origFields.end(), std::inserter(fields, fields.end()),
          boost::bind(&std::make_pair<String, String>,
            boost::bind<String>(&StringMap::value_type::first, _1),
            boost::bind<String>(&ZXTune::IO::MakePathFromString,
              boost::bind<String>(&StringMap::value_type::second, _1), '_')));
      }
      const String& filename = InstantiateTemplate(NameTemplate, fields, SKIP_NONEXISTING);
      std::ofstream file(filename.c_str(), std::ios::binary);
      file.write(safe_ptr_cast<const char*>(&result[0]), static_cast<std::streamsize>(result.size() * sizeof(result.front())));
      if (!file)
      {
        throw MakeFormattedError(THIS_LINE, CONVERT_PARAMETERS,
          Text::CONVERT_ERROR_WRITE_FILE, filename);
      }
      Display.Message((Formatter(Text::CONVERT_DONE) % item.Id % filename).str());
      return true;
    }
  private:
    DisplayComponent& Display;
    std::auto_ptr<ZXTune::Module::Conversion::Parameter> ConversionParameter;
    uint_t CapabilityMask;
    String NameTemplate;
  };

  class CLIApplication : public Application
  {
  public:
    CLIApplication()
      : Informer(InformationComponent::Create())
      , Sourcer(SourceComponent::Create(GlobalParams))
      , Sounder(SoundComponent::Create(GlobalParams))
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
        Sounder->Initialize();
        
        if (!ConvertParams.empty())
        {
          Convertor cnv(ConvertParams, *Display);
          Sourcer->ProcessItems(boost::bind(&Convertor::ProcessItem, &cnv, _1));
        }
        else
        {
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
        options_description options((Formatter(Text::USAGE_SECTION) % *argv).str());
        options.add_options()
          (Text::HELP_KEY, Text::HELP_DESC)
          (Text::VERSION_KEY, Text::VERSION_DESC)
          (Text::CONFIG_KEY, boost::program_options::value<String>(&configFile), Text::CONFIG_DESC)
          (Text::IO_PROVIDERS_OPTS_KEY, boost::program_options::value<String>(&providersOptions), Text::IO_PROVIDERS_OPTS_DESC)
          (Text::CORE_OPTS_KEY, boost::program_options::value<String>(&coreOptions), Text::CORE_OPTS_DESC)
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
          StdOut << VERSION_STRING(ZXTUNE_VERSION) << std::endl;
          return true;
        }
        if (!providersOptions.empty())
        {
          Parameters::Map ioParams;
          ThrowIfError(ParseParametersString(Parameters::ZXTune::IO::Providers::PREFIX, providersOptions, ioParams));
          GlobalParams.insert(ioParams.begin(), ioParams.end());
        }
        if (!coreOptions.empty())
        {
          Parameters::Map coreParams;
          ThrowIfError(ParseParametersString(Parameters::ZXTune::Core::PREFIX, coreOptions, coreParams));
          GlobalParams.insert(coreParams.begin(), coreParams.end());
        }
        {
          Parameters::Map configParams;
          ThrowIfError(ParseConfigFile(configFile, configParams));
          Parameters::MergeMaps(GlobalParams, configParams, GlobalParams, false);
        }
        return false;
      }
      catch (const std::exception& e)
      {
        throw MakeFormattedError(THIS_LINE, UNKNOWN_ERROR, Text::COMMON_ERROR, e.what());
      }
    }
    
    bool PlayItem(const ModuleItem& item)
    {
      try
      {
        ZXTune::Sound::Backend& backend(Sounder->GetBackend());
        ThrowIfError(backend.SetModule(item.Module));
        ThrowIfError(backend.SetParameters(GlobalParams));

        Parameters::IntType frameDuration = 0;
        if (!Parameters::FindByName(GlobalParams, Parameters::ZXTune::Sound::FRAMEDURATION, frameDuration))
        {
          frameDuration = Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT;
        }

        const uint_t seekStepFrames(item.Information.FramesCount * SeekStep / 100);
        ThrowIfError(backend.Play());

        Display->SetModule(item.Information, backend.GetPlayer(), static_cast<uint_t>(frameDuration));

        ZXTune::Sound::Gain curVolume = ZXTune::Sound::Gain();
        ZXTune::Sound::MultiGain allVolume;
        ZXTune::Sound::VolumeControl::Ptr volCtrl(backend.GetVolumeControl());
        const bool noVolume = volCtrl.get() == 0;
        if (!noVolume)
        {
          ThrowIfError(volCtrl->GetVolume(allVolume));
          curVolume = std::accumulate(allVolume.begin(), allVolume.end(), curVolume) / allVolume.size();
        }

        ZXTune::Sound::Backend::State state = ZXTune::Sound::Backend::NOTOPENED;
        Error stateError;

        for (;;)
        {
          state = backend.GetCurrentState(&stateError);

          if (ZXTune::Sound::Backend::FAILED == state)
          {
            throw stateError;
          }

          const uint_t curFrame = Display->BeginFrame(state);

          if (const uint_t key = Console::Self().GetPressedKey())
          {
            switch (key)
            {
            case Console::INPUT_KEY_CANCEL:
            case 'Q':
              return false;
            case Console::INPUT_KEY_LEFT:
              ThrowIfError(backend.SetPosition(curFrame < seekStepFrames ? 0 : curFrame - seekStepFrames));
              break;
            case Console::INPUT_KEY_RIGHT:
              ThrowIfError(backend.SetPosition(curFrame + seekStepFrames));
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
                ThrowIfError(backend.Pause());
                Console::Self().WaitForKeyRelease();
              }
              else
              {
                Console::Self().WaitForKeyRelease();
                ThrowIfError(backend.Play());
              }
              break;
            case ' ':
              ThrowIfError(backend.Stop());
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
      catch (const Error& e)
      {
        e.WalkSuberrors(ErrOuter);
      }
      return true;
    }
  private:
    Parameters::Map GlobalParams;
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
