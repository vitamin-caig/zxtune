/*
Abstract:
  SharedLibrary implementation for Windows

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <contract.h>
#include <error_tools.h>
#include <logging.h>
#include <shared_library.h>
//boost includes
#include <boost/make_shared.hpp>
//platform includes
#include <windows.h>
//text includes
#include <tools/text/tools.h>

#define FILE_TAG 326CACD8

namespace
{
  const std::string THIS_MODULE("Tools");
  const unsigned THIS_MODULE_CODE = Error::ModuleCode<'W', 'S', 'O'>::Value;

  HMODULE OpenLibrary(const std::string& fileName)
  {
    return ::LoadLibrary(fileName.c_str());
  }

  Error CreateLoadError(Error::LocationRef loc, const std::string& fileName)
  {
    //TODO: make text error
    return MakeFormattedError(THIS_LINE, THIS_MODULE_CODE,
      Text::FAILED_LOAD_DYNAMIC_LIBRARY, FromStdString(fileName), ::GetLastError());
  }

  class WindowsSharedLibrary : public SharedLibrary
  {
  public:
    explicit WindowsSharedLibrary(HMODULE handle)
      : Handle(handle)
    {
      Require(Handle != 0);
    }

    virtual ~WindowsSharedLibrary()
    {
      if (Handle)
      {
        ::FreeLibrary(Handle);
      }
    }

    virtual void* GetSymbol(const std::string& name) const
    {
      if (void* res = reinterpret_cast<void*>(::GetProcAddress(Handle, name.c_str())))
      {
        return res;
      }
      throw MakeFormattedError(THIS_LINE, THIS_MODULE_CODE, Text::FAILED_FIND_DYNAMIC_LIBRARY_SYMBOL, FromStdString(name));
    }
  private:
    const HMODULE Handle;
  };
  
  const std::string SUFFIX(".dll");
  
  std::string BuildLibraryFilename(const std::string& name)
  {
    return name + SUFFIX;
  }

  std::string GetLibraryFilename(const std::string& name)
  {
    return name.find(SUFFIX) == name.npos
      ? BuildLibraryFilename(name)
      : name;
  }
}

SharedLibrary::Ptr SharedLibrary::Load(const std::string& name)
{
  const std::string& fileName = GetLibraryFilename(name);
  if (const HMODULE handle = OpenLibrary(fileName))
  {
    Log::Debug(THIS_MODULE, "Loaded '%1%' (as '%2%')", name, fileName);
    return boost::make_shared<WindowsSharedLibrary>(handle);
  }
  throw CreateLoadError(THIS_LINE, fileName);
}

SharedLibrary::Ptr SharedLibrary::Load(const SharedLibrary::Name& name)
{
  const std::string& baseName = name.Base();
  const std::string& baseFileName = GetLibraryFilename(baseName);
  if (const HMODULE handle = OpenLibrary(baseFileName))
  {
    Log::Debug(THIS_MODULE, "Loaded '%1%' (as '%2%')", baseName, baseFileName);
    return boost::make_shared<WindowsSharedLibrary>(handle);
  }
  const Error result = CreateLoadError(THIS_LINE, name.Base());
  const std::vector<std::string>& alternatives = name.WindowsAlternatives();
  for (std::vector<std::string>::const_iterator it = alternatives.begin(), lim = alternatives.end(); it != lim; ++it)
  {
    const std::string altName = *it;
    const std::string altFilename = GetLibraryFilename(altName);
    if (const HMODULE handle = OpenLibrary(altFilename))
    {
      Log::Debug(THIS_MODULE, "Loaded '%1%' (as '%2%')", baseName, altFilename);
      return boost::make_shared<WindowsSharedLibrary>(handle);
    }
  }
  throw result;
}
