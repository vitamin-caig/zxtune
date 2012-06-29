/*
Abstract:
  SharedLibrary implementation for Linux

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
#include <dlfcn.h>
//text includes
#include <tools/text/tools.h>

#define FILE_TAG 4C0042B0

namespace
{
  const unsigned THIS_MODULE = Error::ModuleCode<'L', 'S', 'O'>::Value;

  void CloseLibrary(void* handle)
  {
    if (handle)
    {
      ::dlclose(handle);
    }
  }

  class LinuxSharedLibrary : public SharedLibrary
  {
  public:
    explicit LinuxSharedLibrary(const std::string& fileName)
      : Handle(::dlopen(fileName.c_str(), RTLD_LAZY), std::ptr_fun(&CloseLibrary))
    {
      if (!Handle)
      {
        throw MakeFormattedError(THIS_LINE, THIS_MODULE,
          Text::FAILED_LOAD_DYNAMIC_LIBRARY, FromStdString(fileName), FromStdString(::dlerror()));
      }
    }

    virtual void* GetSymbol(const std::string& name) const
    {
      if (void* res = ::dlsym(Handle.get(), name.c_str()))
      {
        return res;
      }
      throw MakeFormattedError(THIS_LINE, THIS_MODULE, Text::FAILED_FIND_DYNAMIC_LIBRARY_SYMBOL, FromStdString(name));
    }
  private:
    const boost::shared_ptr<void> Handle;
  };
  
  const std::string SUFFIX(".so");
  
  std::string BuildLibraryFilename(const std::string& name)
  {
    return "lib" + name + SUFFIX;
  }
}

SharedLibrary::Ptr SharedLibrary::Load(const std::string& name)
{
  const std::string filename = name.find(SUFFIX) == name.npos
    ? BuildLibraryFilename(name)
    : name;
  return boost::make_shared<LinuxSharedLibrary>(filename);
}
