/*
Abstract:
  Source data provider implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune123 application based on zxtune library
*/

//local includes
#include "console.h"
#include "source.h"
#include <apps/base/app.h>
#include <apps/base/parsing.h>
#include <apps/base/playitem.h>
//common includes
#include <error_tools.h>
#include <progress_callback.h>
#include <tools.h>
//library includes
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/module_detect.h>
#include <core/module_open.h>
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <io/api.h>
#include <io/providers_parameters.h>
#include <strings/array.h>
//std includes
#include <iomanip>
#include <iostream>
//boost includes
#include <boost/bind.hpp>
#include <boost/unordered_set.hpp>
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
    {
    }

    virtual void OnProgress(uint_t current)
    {
      static const Char EMPTY[] = {0};
      OnProgress(current, EMPTY);
    }

    virtual void OnProgress(uint_t current, const String& message)
    {
      CheckForExit();
      if (const uint_t currentWidth = GetCurrentWidth())
      {
        String text = message;
        text += Strings::Format(Text::PROGRESS_FORMAT, current);
        OutputString(currentWidth, text);
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
  };

  class DetectCallback : public Module::DetectCallback
  {
  public:
    DetectCallback(Parameters::Accessor::Ptr params, IO::Identifier::Ptr id, const OnItemCallback& callback, bool showLogs)
      : Params(params)
      , Id(id)
      , Callback(callback)
      , ProgressCallback(showLogs ? new ProgressCallbackImpl() : 0)
    {
    }

    virtual Parameters::Accessor::Ptr GetPluginsParameters() const
    {
      return Params;
    }

    virtual void ProcessModule(ZXTune::DataLocation::Ptr location, Module::Holder::Ptr holder) const
    {
      const IO::Identifier::Ptr subId = Id->WithSubpath(location->GetPath()->AsString());
      const Parameters::Accessor::Ptr moduleParams = Parameters::CreateMergedAccessor(CreatePathProperties(subId), Params);
      const Module::Holder::Ptr result = Module::CreateMixedPropertiesHolder(holder, moduleParams);
      Callback(result);
    }

    virtual Log::ProgressCallback* GetProgress() const
    {
      return ProgressCallback.get();
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const IO::Identifier::Ptr Id;
    const OnItemCallback& Callback;
    const Log::ProgressCallback::Ptr ProgressCallback;
  };

  class Source : public SourceComponent
  {
  public:
    Source(Parameters::Container::Ptr configParams)
      : Params(configParams)
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

    virtual const boost::program_options::options_description& GetOptionsDescription() const
    {
      return OptionsDescription;
    }

    virtual void ParseParameters()
    {
      if (!ProvidersOptions.empty())
      {
        const Parameters::Container::Ptr ioParams = Parameters::Container::Create();
        ThrowIfError(ParseParametersString(Parameters::ZXTune::IO::Providers::PREFIX,
          ProvidersOptions, *ioParams));
        ioParams->Process(*Params);
      }

      if (!CoreOptions.empty() || YM)
      {
        const Parameters::Container::Ptr coreParams = Parameters::Container::Create();
        if (!CoreOptions.empty())
        {
          ThrowIfError(ParseParametersString(Parameters::ZXTune::Core::PREFIX,
            CoreOptions, *coreParams));
        }
        if (YM)
        {
          coreParams->SetValue(Parameters::ZXTune::Core::AYM::TYPE, 1);
        }
        coreParams->Process(*Params);
      }
    }

    // throw
    virtual void Initialize()
    {
      if (Files.empty())
      {
        throw Error(THIS_LINE, Text::INPUT_ERROR_NO_FILES);
      }
    }

    virtual void ProcessItems(const OnItemCallback& callback)
    {
      assert(callback);

      for (Strings::Array::const_iterator it = Files.begin(), lim = Files.end(); it != lim; ++it)
      {
        ProcessItem(*it, callback);
      }
    }
  private:
    void ProcessItem(const String& uri, const OnItemCallback& callback) const
    {
      try
      {
        const IO::Identifier::Ptr id = IO::ResolveUri(uri);

        const DetectCallback detectCallback(Params, id, callback, ShowProgress);
        Log::ProgressCallback& progress = ShowProgress ? *detectCallback.GetProgress() : Log::ProgressCallback::Stub();
        const Binary::Container::Ptr data = IO::OpenData(id->Path(), *Params, progress);

        const String subpath = id->Subpath();
        if (subpath.empty())
        {
          const ZXTune::DataLocation::Ptr location = ZXTune::CreateLocation(data);
          Module::Detect(location, detectCallback);
        }
        else
        {
          const ZXTune::DataLocation::Ptr location = ZXTune::OpenLocation(Params, data, subpath);
          const Module::Holder::Ptr module = Module::Open(location);
          detectCallback.ProcessModule(location, module);
        }
      }
      catch (const Error& e)
      {
        StdOut << e.ToString();
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

std::auto_ptr<SourceComponent> SourceComponent::Create(Parameters::Container::Ptr configParams)
{
  return std::auto_ptr<SourceComponent>(new Source(configParams));
}
