#include "app.h"
#include "error_codes.h"
#include "information.h"
#include "parsing.h"
#include "sound.h"
#include "source.h"

#include <error_tools.h>
#include <core/module_attrs.h>

#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include <iostream>

#include "messages.h"

#define FILE_TAG 81C76E7D

namespace
{
  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    std::cout << Error::AttributesToString(loc, code, text);
  }

  class CLIApplication : public Application
  {
  public:
    CLIApplication()
      : Informer(InformationComponent::Create())
      , Sourcer(SourceComponent::Create(GlobalParams))
      , Sounder(SoundComponent::Create(GlobalParams))
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
        
        ModuleItemsArray playlist;
        Sourcer->GetItems(playlist);
        std::for_each(playlist.begin(), playlist.end(), boost::bind(&CLIApplication::PlayItem, this, _1));
      }
      catch (const Error& e)
      {
        e.WalkSuberrors(ErrOuter);
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

        options_description options((Formatter(TEXT_USAGE_SECTION) % *argv).str());
        options.add_options()(TEXT_HELP_KEY, TEXT_HELP_DESC);
        options.add(Informer->GetOptionsDescription());
        options.add(Sourcer->GetOptionsDescription());
        options.add(Sounder->GetOptionsDescription());
        //add positional parameters for input
        positional_options_description inputPositional;
        inputPositional.add(TEXT_INPUT_FILE_KEY, -1);
        
        variables_map vars;
        store(command_line_parser(argc, argv).options(options).positional(inputPositional).run(), vars);
        notify(vars);
        if (vars.count(TEXT_HELP_KEY))
        {
          std::cout << options << std::endl;
          return true;
        }
        return false;
      }
      catch (const std::exception& e)
      {
        throw MakeFormattedError(THIS_LINE, UNKNOWN_ERROR, "Error: %1%", e.what());
      }
    }
    
    void PlayItem(const ModuleItem& item)
    {
      //show startup info
      ShowItemInfo(item);
      //calculate and apply parameters
      Parameters::Map perItemParams(GlobalParams);
      perItemParams.insert(item.Params.begin(), item.Params.end());
      ZXTune::Sound::Backend& backend(Sounder->GetBackend());
      ThrowIfError(backend.SetModule(item.Module));
      ThrowIfError(backend.SetParameters(perItemParams));
      
      ThrowIfError(backend.Play());
      while (ZXTune::Module::Player::ConstPtr player = backend.GetPlayer().lock()) 
      {
        if (ZXTune::Sound::Backend::STOP == backend.WaitForEvent(ZXTune::Sound::Backend::STOP, 1000))
        {
          break;
        }
      }
    }
    
    void ShowItemInfo(const ModuleItem& item)
    {
      ZXTune::Module::Information info;
      item.Module->GetModuleInformation(info);
      StringMap strProps;
      Parameters::ConvertMap(info.Properties, strProps);
      std::cout << (Formatter(
        "Playing   %1%\n"
        "Title:    %2%\n"
        "Duration: %3%\n"
        "Channels: %4%\n")
        % item.Id % strProps[ZXTune::Module::ATTR_TITLE]
        % UnparseFrameTime(info.Statistic.Frame, 20000) % info.PhysicalChannels).str();
    }
  private:
    Parameters::Map GlobalParams;
    std::auto_ptr<InformationComponent> Informer;
    std::auto_ptr<SourceComponent> Sourcer;
    std::auto_ptr<SoundComponent> Sounder;
  };
}

std::auto_ptr<Application> Application::Create()
{
  return std::auto_ptr<Application>(new CLIApplication());
}
