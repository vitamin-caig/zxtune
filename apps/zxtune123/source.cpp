/**
 *
 * @file
 *
 * @brief Source data provider implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "source.h"
#include "config.h"
#include "console.h"
// common includes
#include <contract.h>
#include <error_tools.h>
#include <progress_callback.h>
// library includes
#include <core/additional_files_resolve.h>
#include <core/core_parameters.h>
#include <core/data_location.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <core/service.h>
#include <io/api.h>
#include <io/providers_parameters.h>
#include <module/properties/path.h>
#include <parameters/merged_container.h>
#include <platform/application.h>
#include <strings/array.h>
#include <time/elapsed.h>
// std includes
#include <iomanip>
#include <iostream>
// boost includes
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>

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
    {}

    void OnProgress(uint_t current) override
    {
      static const String EMPTY;
      OnProgress(current, EMPTY);
    }

    void OnProgress(uint_t current, StringView message) override
    {
      if (ReportTimeout())
      {
        CheckForExit();
        if (const uint_t currentWidth = GetCurrentWidth())
        {
          OutputString(currentWidth, Strings::Format("{} [{}%]", message, current));
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
    {}

    Binary::Container::Ptr Get(StringView name) const override
    {
      return IO::OpenData(Dir + name.to_string(), Params, Log::ProgressCallback::Stub());
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
    {}

    Parameters::Container::Ptr CreateInitialProperties(StringView subpath) const
    {
      auto subId = Id->WithSubpath(subpath);
      auto moduleProperties = Module::CreatePathProperties(std::move(subId));
      return Parameters::CreateMergedContainer(std::move(moduleProperties), Parameters::Container::Create());
    }

    void ProcessModule(const ZXTune::DataLocation& location, const ZXTune::Plugin& /*decoder*/,
                       Module::Holder::Ptr holder) override
    {
      if (location.GetPath()->Empty())
      {
        if (const auto files = dynamic_cast<const Module::AdditionalFiles*>(holder.get()))
        {
          const RealFilesSource source(*Params, *Id);
          Module::ResolveAdditionalFiles(source, *files);
        }
      }
      Callback.ProcessItem(location.GetData(), std::move(holder));
    }

    void ProcessUnknownData(const ZXTune::DataLocation& location) override
    {
      const auto subId = Id->WithSubpath(location.GetPath()->AsString());
      Callback.ProcessUnknownData(subId->Full(), location.GetPluginsChain()->AsString(), location.GetData());
    }

    Log::ProgressCallback* GetProgress() const override
    {
      return ProgressCallback.get();
    }

  private:
    const Parameters::Accessor::Ptr Params;
    const IO::Identifier::Ptr Id;
    OnItemCallback& Callback;
    const std::unique_ptr<Log::ProgressCallback> ProgressCallback;
  };

  class Source : public SourceComponent
  {
  public:
    Source(Parameters::Container::Ptr configParams)
      : Params(std::move(configParams))
      , Service(ZXTune::Service::Create(Params))
      , OptionsDescription("Input options")
      , ShowProgress(false)
      , YM(false)
    {
      using namespace boost::program_options;
      auto opt = OptionsDescription.add_options();
      opt("file", value<Strings::Array>(&Files), "file to process");
      opt("providers-options", value<String>(&ProvidersOptions),
          "options for i/o providers. Implies 'zxtune.io.providers.' prefix to all options in map.");
      opt("progress", bool_switch(&ShowProgress), "show progress while processing input files.");
      opt("core-options", value<String>(&CoreOptions),
          "options for core. Implies 'zxtune.core.' prefix to all options in map.");
      opt("ym", bool_switch(&YM), "use YM chip for playback");
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
        ParseParametersString(Parameters::ZXTune::IO::Providers::PREFIX, ProvidersOptions, *ioParams);
        ioParams->Process(*Params);
      }

      if (!CoreOptions.empty() || YM)
      {
        const Parameters::Container::Ptr coreParams = Parameters::Container::Create();
        if (!CoreOptions.empty())
        {
          ParseParametersString(Parameters::ZXTune::Core::PREFIX, CoreOptions, *coreParams);
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
        throw Error(THIS_LINE, "No files to process.");
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
          Service->DetectModules(std::move(data), detectCallback);
        }
        else
        {
          Service->OpenModule(std::move(data), subpath, detectCallback);
        }
      }
      catch (const Error& e)
      {
        Console::Self().Write(e.ToString());
      }
    }

  private:
    const Parameters::Container::Ptr Params;
    const ZXTune::Service::Ptr Service;
    boost::program_options::options_description OptionsDescription;
    Strings::Array Files;
    String ProvidersOptions;
    bool ShowProgress;
    String CoreOptions;
    bool YM;
  };
}  // namespace

std::unique_ptr<SourceComponent> SourceComponent::Create(Parameters::Container::Ptr configParams)
{
  return std::unique_ptr<SourceComponent>(new Source(configParams));
}
