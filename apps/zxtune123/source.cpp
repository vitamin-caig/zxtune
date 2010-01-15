#include "error_codes.h"
#include "source.h"

#include <io/provider.h>

#include <boost/bind.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>

#include "text.h"

#define FILE_TAG 9EDFE3AF

namespace
{
  Error FormModule(const String& path, const String& subpath, const ZXTune::Module::Holder::Ptr& module,
    const OnItemCallback& callback)
  {
    String uri;
    if (const Error& e = ZXTune::IO::CombineUri(path, subpath, uri))
    {
      return e;
    }
    ModuleItem item;
    item.Id = uri;
    item.Module = module;
    callback(item);
    return Error();
  }
  
  class Source : public SourceComponent
  {
  public:
    explicit Source(Parameters::Map& globalParams)
      : GlobalParams(globalParams)
      , OptionsDescription(TEXT_INPUT_SECTION)
    {
      OptionsDescription.add_options()
        (TEXT_INPUT_FILE_KEY, boost::program_options::value<StringArray>(&Files), TEXT_INPUT_FILE_DESC)
      ;
    }

    virtual const boost::program_options::options_description& GetOptionsDescription() const
    {
      return OptionsDescription;
    }
    
    // throw
    virtual void Initialize()
    {
      if (Files.empty())
      {
        throw Error(THIS_LINE, NO_INPUT_FILES, TEXT_INPUT_ERROR_NO_FILES);
      }
    }
    
    virtual void ProcessItems(const OnItemCallback& callback)
    {
      assert(callback);
      ThrowIfError(ProcessModuleItems(Files, GlobalParams, 0, callback));
    }

  private:
    Parameters::Map& GlobalParams;
    boost::program_options::options_description OptionsDescription;
    StringArray Files;
  };
}

Error ProcessModuleItems(const StringArray& files, const Parameters::Map& params, const ZXTune::DetectParameters::FilterFunc& filter,
                    const boost::function<void(const ModuleItem&)>& callback)
{
  //TODO: optimize
  for (StringArray::const_iterator it = files.begin(), lim = files.end(); it != lim; ++it)
  {
    ZXTune::IO::DataContainer::Ptr data;
    String subpath;
    if (const Error& e = ZXTune::IO::OpenData(*it, params, 0, data, subpath))
    {
      return e;
    }
    ZXTune::DetectParameters detectParams;
    detectParams.Filter = filter;
    detectParams.Callback = boost::bind(&FormModule, *it, _1, _2, callback);
    if (const Error& e = ZXTune::DetectModules(params, detectParams, data, subpath))
    {
      return e;
    }
  }
  return Error();
}

std::auto_ptr<SourceComponent> SourceComponent::Create(Parameters::Map& globalParams)
{
  return std::auto_ptr<SourceComponent>(new Source(globalParams));
}
