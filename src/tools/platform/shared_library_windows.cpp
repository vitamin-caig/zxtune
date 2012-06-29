/*
Abstract:
  SharedLibrary implementation for Windows

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <error_tools.h>
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
  const unsigned THIS_MODULE = Error::ModuleCode<'W', 'S', 'O'>::Value;

  class WindowsSharedLibrary : public SharedLibrary
  {
  public:
    explicit WindowsSharedLibrary(const std::string& fileName)
      : Handle(::LoadLibrary(fileName.c_str()))
    {
      if (!Handle)
      {
        //TODO: make text error
        throw MakeFormattedError(THIS_LINE, THIS_MODULE,
          Text::FAILED_LOAD_DYNAMIC_LIBRARY, FromStdString(fileName), ::GetLastError());
      }
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
      throw MakeFormattedError(THIS_LINE, THIS_MODULE, Text::FAILED_FIND_DYNAMIC_LIBRARY_SYMBOL, FromStdString(name));
    }
  private:
    const HMODULE Handle;
  };
  
  const std::string SUFFIX(".dll");
  
  std::string BuildLibraryFilename(const std::string& name)
  {
    return name + SUFFIX;
  }
}

SharedLibrary::Ptr SharedLibrary::Load(const std::string& name)
{
  const std::string filename = name.find(SUFFIX) == name.npos
    ? BuildLibraryFilename(name)
    : name;
  return boost::make_shared<WindowsSharedLibrary>(filename);
}
