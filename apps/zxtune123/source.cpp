/**
* 
* @file
*
* @brief Source data provider implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "config.h"
#include "console.h"
#include "source.h"
//common includes
#include <contract.h>
#include <error_tools.h>
#include <progress_callback.h>
//library includes
#include <core/additional_files_resolve.h>
#include <core/core_parameters.h>
#include <core/module_detect.h>
#include <core/module_open.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <io/api.h>
#include <io/providers_parameters.h>
#include <module/properties/path.h>
#include <parameters/merged_container.h>
#include <platform/application.h>
#include <strings/array.h>
#include <time/elapsed.h>
//std includes
#include <iomanip>
#include <iostream>
//boost includes
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>
//text includes
#include "text/text.h"

#define FILE_TAG 9EDFE3AF

namespace
{
  const Char DELIMITERS[] = {',', ';', ':', '\0'};

  void OutputString(uint_t width, const String& text)
  {
    const String::size_type curSize = text.size();
    if (curSize < width)
    {
      StdOut << text << String(width - curSize - 1, ' ') << '\r';
    }
    else
    {
      StdOut << text << '\n';
    }
    StdOut << std::flush;
  }

  class ProgressCallbackImpl : public Log::ProgressCallback
  {
  public:
    ProgressCallbackImpl()
      : Cons(Console::Self())
      , ReportTimeout(Time::Milliseconds(1000))
    {
    }

    void OnProgress(uint_t current) override
    {
      static const String EMPTY;
      OnProgress(current, EMPTY);
    }

    void OnProgress(uint_t current, const String& message) override
    {
      if (ReportTimeout())
      {
        CheckForExit();
        if (const uint_t currentWidth = GetCurrentWidth())
        {
          String text = message;
          text += Strings::Format(Text::PROGRESS_FORMAT, current);
          OutputString(currentWidth, text);
        }
      }
    }
  private:
    void CheckForExit() const
    {
      if (Cons.GetPressedKey() == Console::INPUT_KEY_CANCEL)
      {
        Cons.WaitForKeyRelease();
        throw std::exception();
      }
    }

    uint_t GetCurrentWidth() const
    {
      const int_t width = Cons.GetSize().first;
      return width >= 0 ? static_cast<uint_t>(width) : 0;
    }
  private:
    const Console& Cons;
    Time::Elapsed ReportTimeout;
  };
  
  class RealFilesSource : public Module::AdditionalFilesSource
  {
  public:
    RealFilesSource(const Parameters::Accessor& params, const IO::Identifier& id)
      : Params(params)
      , Dir(ExtractDir(id))
    {
    }
    
    Binary::Container::Ptr Get(const String& name) const override
    {
      return IO::OpenData(Dir + name, Params, Log::ProgressCallback::Stub());
    }
  private:
    static String ExtractDir(const IO::Identifier& id)
    {
      const auto& full = id.Full();
      const auto& filename = id.Filename();
      Require(!filename.empty());
      Require(id.Subpath().empty());
      return full.substr(0, full.size() - filename.size());
    }
  private:
    const Parameters::Accessor& Params;
    const String Dir;
  };

  class DetectCallback : public Module::DetectCallback
  {
  public:
    DetectCallback(Parameters::Accessor::Ptr params, IO::Identifier::Ptr id, OnItemCallback& callback, bool showLogs)
      : Params(std::move(params))
      , Id(std::move(id))
      , Callback(callback)
      , ProgressCallback(showLogs ? new ProgressCallbackImpl() : nullptr)
    {
    }

    Parameters::Container::Ptr CreateInitialProperties(const String& subpath) const
    {
      auto subId = Id->WithSubpath(subpath);
      auto moduleProperties = Module::CreatePathProperties(std::move(subId));
      return Parameters::CreateMergedContainer(std::move(moduleProperties), Parameters::Container::Create());
    }

    void ProcessModule(const ZXTune::DataLocation& location, const ZXTune::Plugin& /*decoder*/, Module::Holder::Ptr holder) override
    {
      if (!location.GetPath()->Empty())
      {
        if (const auto files = dynamic_cast<const Module::AdditionalFiles*>(holder.get()))
        {
          const RealFilesSource source(*Params, *Id);
          Module::ResolveAdditionalFiles(source, *files);
        }
      }
      Callback.ProcessItem(location.GetData(), std::move(holder));
    }

    Log::ProgressCallback* GetProgress() const override
    {
      return ProgressCallback.get();
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const IO::Identifier::Ptr Id;
    OnItemCallback& Callback;
    const Log::ProgressCallback::Ptr ProgressCallback;
  };

  class Source : public SourceComponent
  {
  public:
    Source(Parameters::Container::Ptr configParams)
      : Params(std::move(configParams))
      , OptionsDescription(Text::INPUT_SECTION)
      , ShowProgress(false)
      , YM(false)
    {
      OptionsDescription.add_options()
        (Text::INPUT_FILE_KEY, boost::program_options::value<Strings::Array>(&Files), Text::INPUT_FILE_DESC)
        (Text::IO_PROVIDERS_OPTS_KEY, boost::program_options::value<String>(&ProvidersOptions), Text::IO_PROVIDERS_OPTS_DESC)
        (Text::INPUT_PROGRESS_KEY, boost::program_options::bool_switch(&ShowProgress), Text::INPUT_PROGRESS_DESC)
        (Text::CORE_OPTS_KEY, boost::program_options::value<String>(&CoreOptions), Text::CORE_OPTS_DESC)
        (Text::YM_KEY, boost::program_options::bool_switch(&YM), Text::YM_DESC)
      ;
    }

    const boost::program_options::options_description& GetOptionsDescription() const override
    {
      return OptionsDescription;
    }

    void ParseParameters() override
    {
      if (!ProvidersOptions.empty())
      {
        const Parameters::Container::Ptr ioParams = Parameters::Container::Create();
        ParseParametersString(Parameters::ZXTune::IO::Providers::PREFIX,
          ProvidersOptions, *ioParams);
        ioParams->Process(*Params);
      }

      if (!CoreOptions.empty() || YM)
      {
        const Parameters::Container::Ptr coreParams = Parameters::Container::Create();
        if (!CoreOptions.empty())
        {
          ParseParametersString(Parameters::ZXTune::Core::PREFIX,
            CoreOptions, *coreParams);
        }
        if (YM)
        {
          coreParams->SetValue(Parameters::ZXTune::Core::AYM::TYPE, Parameters::ZXTune::Core::AYM::TYPE_YM);
        }
        coreParams->Process(*Params);
      }
    }

    // throw
    void Initialize() override
    {
      if (Files.empty())
      {
        throw Error(THIS_LINE, Text::INPUT_ERROR_NO_FILES);
      }
    }

    void ProcessItems(OnItemCallback& callback) override
    {
      for (Strings::Array::const_iterator it = Files.begin(), lim = Files.end(); it != lim; ++it)
      {
        ProcessItem(*it, callback);
      }
    }
  private:
    void ProcessItem(const String& uri, OnItemCallback& callback) const
    {
      try
      {
        const IO::Identifier::Ptr id = IO::ResolveUri(uri);

        DetectCallback detectCallback(Params, id, callback, ShowProgress);
        auto data = IO::OpenData(id->Path(), *Params, Log::ProgressCallback::Stub());

        const String subpath = id->Subpath();
        if (subpath.empty())
        {
          Module::Detect(*Params, std::move(data), detectCallback);
        }
        else
        {
          Module::Open(*Params, std::move(data), subpath, detectCallback);
        }
      }
      catch (const Error& e)
      {
        Console::Self().Write(e.ToString());
      }
    }
  private:
    const Parameters::Container::Ptr Params;
    boost::program_options::options_description OptionsDescription;
    Strings::Array Files;
    String ProvidersOptions;
    bool ShowProgress;
    String CoreOptions;
    bool YM;
  };
}

std::unique_ptr<SourceComponent> SourceComponent::Create(Parameters::Container::Ptr configParams)
{
  return std::unique_ptr<SourceComponent>(new Source(configParams));
}
